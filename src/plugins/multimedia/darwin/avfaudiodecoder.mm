// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfaudiodecoder_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/qiodevice.h>
#include <QMimeDatabase>
#include <QThread>
#include "private/qcoreaudioutils_p.h"
#include <QtCore/qloggingcategory.h>

#include <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

static Q_LOGGING_CATEGORY(qLcAVFAudioDecoder, "qt.multimedia.darwin.AVFAudioDecoder")
constexpr static int MAX_BUFFERS_IN_QUEUE = 5;

QAudioBuffer handleNextSampleBuffer(CMSampleBufferRef sampleBuffer)
{
    if (!sampleBuffer)
        return {};

    // Check format
    CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(sampleBuffer);
    if (!formatDescription)
        return {};
    const AudioStreamBasicDescription* const asbd = CMAudioFormatDescriptionGetStreamBasicDescription(formatDescription);
    QAudioFormat qtFormat = CoreAudioUtils::toQAudioFormat(*asbd);
    if (qtFormat.sampleFormat() == QAudioFormat::Unknown && asbd->mBitsPerChannel == 8)
        qtFormat.setSampleFormat(QAudioFormat::UInt8);
    if (!qtFormat.isValid())
        return {};

    // Get the required size to allocate to audioBufferList
    size_t audioBufferListSize = 0;
    OSStatus err = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(sampleBuffer,
                        &audioBufferListSize,
                        NULL,
                        0,
                        NULL,
                        NULL,
                        kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
                        NULL);
    if (err != noErr)
        return {};

    CMBlockBufferRef blockBuffer = NULL;
    AudioBufferList* audioBufferList = (AudioBufferList*) malloc(audioBufferListSize);
    // This ensures the buffers placed in audioBufferList are contiguous
    err = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(sampleBuffer,
                        NULL,
                        audioBufferList,
                        audioBufferListSize,
                        NULL,
                        NULL,
                        kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
                        &blockBuffer);
    if (err != noErr) {
        free(audioBufferList);
        return {};
    }

    QByteArray abuf;
    for (UInt32 i = 0; i < audioBufferList->mNumberBuffers; i++)
    {
        AudioBuffer audioBuffer = audioBufferList->mBuffers[i];
        abuf.push_back(QByteArray((const char*)audioBuffer.mData, audioBuffer.mDataByteSize));
    }

    free(audioBufferList);
    CFRelease(blockBuffer);

    CMTime sampleStartTime = (CMSampleBufferGetPresentationTimeStamp(sampleBuffer));
    float sampleStartTimeSecs = CMTimeGetSeconds(sampleStartTime);

    return QAudioBuffer(abuf, qtFormat, qint64(sampleStartTimeSecs * 1000000));
}

@interface AVFResourceReaderDelegate : NSObject <AVAssetResourceLoaderDelegate> {
    AVFAudioDecoder *m_decoder;
    QMutex m_mutex;
}

- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader
        shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest;

@end

@implementation AVFResourceReaderDelegate

- (id)initWithDecoder:(AVFAudioDecoder *)decoder
{
    if (!(self = [super init]))
        return nil;

    m_decoder = decoder;

    return self;
}

-(BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader
       shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest
{
    Q_UNUSED(resourceLoader);

    if (![loadingRequest.request.URL.scheme isEqualToString:@"iodevice"])
        return NO;

    QMutexLocker locker(&m_mutex);

    QIODevice *device = m_decoder->sourceDevice();
    if (!device)
        return NO;

    device->seek(loadingRequest.dataRequest.requestedOffset);
    if (loadingRequest.contentInformationRequest) {
        loadingRequest.contentInformationRequest.contentLength = device->size();
        loadingRequest.contentInformationRequest.byteRangeAccessSupported = YES;
    }

    if (loadingRequest.dataRequest) {
        NSInteger requestedLength = loadingRequest.dataRequest.requestedLength;
        int maxBytes = qMin(32 * 1024, int(requestedLength));
        char buffer[maxBytes];
        NSInteger submitted = 0;
        while (submitted < requestedLength) {
            qint64 len = device->read(buffer, maxBytes);
            if (len < 1)
                break;

            [loadingRequest.dataRequest respondWithData:[NSData dataWithBytes:buffer length:len]];
            submitted += len;
        }

        // Finish loading even if not all bytes submitted.
        [loadingRequest finishLoading];
    }

    return YES;
}

@end

namespace {

NSDictionary *av_audio_settings_for_format(const QAudioFormat &format)
{
    float sampleRate = format.sampleRate();
    int nChannels = format.channelCount();
    int sampleSize = format.bytesPerSample() * 8;
    BOOL isFloat = format.sampleFormat() == QAudioFormat::Float;

    NSDictionary *audioSettings = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithInt:kAudioFormatLinearPCM], AVFormatIDKey,
        [NSNumber numberWithFloat:sampleRate], AVSampleRateKey,
        [NSNumber numberWithInt:nChannels], AVNumberOfChannelsKey,
        [NSNumber numberWithInt:sampleSize], AVLinearPCMBitDepthKey,
        [NSNumber numberWithBool:isFloat], AVLinearPCMIsFloatKey,
        [NSNumber numberWithBool:NO], AVLinearPCMIsNonInterleaved,
        [NSNumber numberWithBool:NO], AVLinearPCMIsBigEndianKey,
        nil];

    return audioSettings;
}

QAudioFormat qt_format_for_audio_track(AVAssetTrack *track)
{
    QAudioFormat format;
    CMFormatDescriptionRef desc =  (__bridge CMFormatDescriptionRef)track.formatDescriptions[0];
    const AudioStreamBasicDescription* const asbd =
        CMAudioFormatDescriptionGetStreamBasicDescription(desc);
    format = CoreAudioUtils::toQAudioFormat(*asbd);
    // AudioStreamBasicDescription's mBitsPerChannel is 0 for compressed formats
    // In this case set default Int16 sample format
    if (asbd->mBitsPerChannel == 0)
        format.setSampleFormat(QAudioFormat::Int16);
    return format;
}

}

struct AVFAudioDecoder::DecodingContext
{
    AVAssetReader *m_reader = nullptr;
    AVAssetReaderTrackOutput *m_readerOutput = nullptr;

    ~DecodingContext()
    {
        if (m_reader) {
            [m_reader cancelReading];
            [m_reader release];
        }

        [m_readerOutput release];
    }
};

AVFAudioDecoder::AVFAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
{
    m_readingQueue = dispatch_queue_create("reader_queue", DISPATCH_QUEUE_SERIAL);
    m_decodingQueue = dispatch_queue_create("decoder_queue", DISPATCH_QUEUE_SERIAL);

    m_readerDelegate = [[AVFResourceReaderDelegate alloc] initWithDecoder:this];
}

AVFAudioDecoder::~AVFAudioDecoder()
{
    stop();

    [m_readerDelegate release];
    [m_asset release];

    dispatch_release(m_readingQueue);
    dispatch_release(m_decodingQueue);
}

QUrl AVFAudioDecoder::source() const
{
    return m_source;
}

void AVFAudioDecoder::setSource(const QUrl &fileName)
{
    if (!m_device && m_source == fileName)
        return;

    stop();
    m_device = nullptr;
    [m_asset release];
    m_asset = nil;

    m_source = fileName;

    if (!m_source.isEmpty()) {
        NSURL *nsURL = m_source.toNSURL();
        m_asset = [[AVURLAsset alloc] initWithURL:nsURL options:nil];
    }

    sourceChanged();
}

QIODevice *AVFAudioDecoder::sourceDevice() const
{
    return m_device;
}

void AVFAudioDecoder::setSourceDevice(QIODevice *device)
{
    if (m_device == device && m_source.isEmpty())
        return;

    stop();
    m_source.clear();
    [m_asset release];
    m_asset = nil;

    m_device = device;

    if (m_device) {
        const QString ext = QMimeDatabase().mimeTypeForData(m_device).preferredSuffix();
        const QString url = "iodevice:///iodevice." + ext;
        NSString *urlString = url.toNSString();
        NSURL *nsURL = [NSURL URLWithString:urlString];

        m_asset = [[AVURLAsset alloc] initWithURL:nsURL options:nil];

        // use decoding queue instead of reading queue in order to fix random stucks.
        // Anyway, decoding queue is empty in the moment.
        [m_asset.resourceLoader setDelegate:m_readerDelegate queue:m_decodingQueue];
    }

    sourceChanged();
}

void AVFAudioDecoder::start()
{
    if (m_decodingContext) {
        qCDebug(qLcAVFAudioDecoder()) << "AVFAudioDecoder has been already started";
        return;
    }

    positionChanged(-1);

    if (m_device && (!m_device->isOpen() || !m_device->isReadable())) {
        processInvalidMedia(QAudioDecoder::ResourceError, tr("Unable to read from specified device"));
        return;
    }

    m_decodingContext = std::make_shared<DecodingContext>();
    std::weak_ptr<DecodingContext> weakContext(m_decodingContext);

    auto handleLoadingResult = [=]() {
        NSError *error = nil;
        AVKeyValueStatus status = [m_asset statusOfValueForKey:@"tracks" error:&error];

        if (status == AVKeyValueStatusFailed) {
            if (error.domain == NSURLErrorDomain)
                processInvalidMedia(QAudioDecoder::ResourceError,
                                    QString::fromNSString(error.localizedDescription));
            else
                processInvalidMedia(QAudioDecoder::FormatError,
                                    tr("Could not load media source's tracks"));
        } else if (status != AVKeyValueStatusLoaded) {
            qWarning() << "Unexpected AVKeyValueStatus:" << status;
            stop();
        }
        else {
            initAssetReader();
        }
    };

    [m_asset loadValuesAsynchronouslyForKeys:@[ @"tracks" ]
                           completionHandler:[=]() {
                               invokeWithDecodingContext(weakContext, handleLoadingResult);
                           }];
}

void AVFAudioDecoder::decBuffersCounter(uint val)
{
    if (val) {
        QMutexLocker locker(&m_buffersCounterMutex);
        m_buffersCounter -= val;
    }

    Q_ASSERT(m_buffersCounter >= 0);

    m_buffersCounterCondition.wakeAll();
}

void AVFAudioDecoder::stop()
{
    qCDebug(qLcAVFAudioDecoder()) << "stop decoding";

    m_decodingContext.reset();
    decBuffersCounter(m_cachedBuffers.size());
    m_cachedBuffers.clear();

    bufferAvailableChanged(false);
    positionChanged(-1);
    durationChanged(-1);

    onFinished();
}

QAudioFormat AVFAudioDecoder::audioFormat() const
{
    return m_format;
}

void AVFAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (m_format != format) {
        m_format = format;
        formatChanged(m_format);
    }
}

QAudioBuffer AVFAudioDecoder::read()
{
    if (m_cachedBuffers.empty())
        return QAudioBuffer();

    Q_ASSERT(m_cachedBuffers.size() > 0);
    QAudioBuffer buffer = m_cachedBuffers.dequeue();
    decBuffersCounter(1);

    positionChanged(buffer.startTime() / 1000);
    bufferAvailableChanged(!m_cachedBuffers.empty());
    return buffer;
}

void AVFAudioDecoder::processInvalidMedia(QAudioDecoder::Error errorCode,
                                          const QString &errorString)
{
    qCDebug(qLcAVFAudioDecoder()) << "Invalid media. Error code:" << errorCode
                                  << "Description:" << errorString;

    Q_ASSERT(QThread::currentThread() == thread());

    error(int(errorCode), errorString);

    // TODO: may be check if decodingCondext was changed by
    // user's action (restart) from the emitted error.
    // We should handle it somehow (don't run stop, print warning or etc...)

    stop();
}

void AVFAudioDecoder::onFinished()
{
    m_decodingContext.reset();

    if (isDecoding())
        finished();
}

void AVFAudioDecoder::initAssetReader()
{
    qCDebug(qLcAVFAudioDecoder()) << "Init asset reader";

    Q_ASSERT(m_asset);
    Q_ASSERT(QThread::currentThread() == thread());

    NSArray<AVAssetTrack *> *tracks = [m_asset tracksWithMediaType:AVMediaTypeAudio];
    if (!tracks.count) {
        processInvalidMedia(QAudioDecoder::FormatError, tr("No audio tracks found"));
        return;
    }

    AVAssetTrack *track = [tracks objectAtIndex:0];
    QAudioFormat format = m_format.isValid() ? m_format : qt_format_for_audio_track(track);
    if (!format.isValid()) {
        processInvalidMedia(QAudioDecoder::FormatError, tr("Unsupported source format"));
        return;
    }

    durationChanged(CMTimeGetSeconds(track.timeRange.duration) * 1000);

    NSError *error = nil;
    NSDictionary *audioSettings = av_audio_settings_for_format(format);

    AVAssetReaderTrackOutput *readerOutput =
            [[AVAssetReaderTrackOutput alloc] initWithTrack:track outputSettings:audioSettings];
    AVAssetReader *reader = [[AVAssetReader alloc] initWithAsset:m_asset error:&error];
    if (error) {
        processInvalidMedia(QAudioDecoder::ResourceError, QString::fromNSString(error.localizedDescription));
        return;
    }
    if (![reader canAddOutput:readerOutput]) {
        processInvalidMedia(QAudioDecoder::ResourceError, tr("Failed to add asset reader output"));
        return;
    }

    [reader addOutput:readerOutput];

    Q_ASSERT(m_decodingContext);
    m_decodingContext->m_reader = reader;
    m_decodingContext->m_readerOutput = readerOutput;

    startReading();
}

void AVFAudioDecoder::startReading()
{
    Q_ASSERT(m_decodingContext);
    Q_ASSERT(m_decodingContext->m_reader);
    Q_ASSERT(QThread::currentThread() == thread());

    // Prepares the receiver for obtaining sample buffers from the asset.
    if (![m_decodingContext->m_reader startReading]) {
        processInvalidMedia(QAudioDecoder::ResourceError, tr("Could not start reading"));
        return;
    }

    setIsDecoding(true);

    std::weak_ptr<DecodingContext> weakContext = m_decodingContext;

    // Since copyNextSampleBuffer is synchronous, submit it to an async dispatch queue
    // to run in a separate thread. Call the handleNextSampleBuffer "callback" on another
    // thread when new audio sample is read.
    auto copyNextSampleBuffer = [=]() {
        auto decodingContext = weakContext.lock();
        if (!decodingContext)
            return false;

        CMSampleBufferRef sampleBuffer = [decodingContext->m_readerOutput copyNextSampleBuffer];
        if (!sampleBuffer)
            return false;

        dispatch_async(m_decodingQueue, [=]() {
            if (!weakContext.expired() && CMSampleBufferDataIsReady(sampleBuffer)) {
                auto audioBuffer = handleNextSampleBuffer(sampleBuffer);

                if (audioBuffer.isValid())
                    invokeWithDecodingContext(weakContext,
                                              [=]() { handleNewAudioBuffer(audioBuffer); });
            }

            CFRelease(sampleBuffer);
        });

        return true;
    };

    dispatch_async(m_readingQueue, [=]() {
        qCDebug(qLcAVFAudioDecoder()) << "start reading thread";

        do {
            // Note, waiting here doesn't ensure strong contol of the counter.
            // However, it doesn't affect the logic: the reading flow works fine
            // even if the counter is time-to-time more than max value
            waitUntilBuffersCounterLessMax();
        } while (copyNextSampleBuffer());

        // TODO: check m_reader.status == AVAssetReaderStatusFailed
        invokeWithDecodingContext(weakContext, [this]() { onFinished(); });
    });
}

void AVFAudioDecoder::waitUntilBuffersCounterLessMax()
{
    if (m_buffersCounter >= MAX_BUFFERS_IN_QUEUE) {
        // the check avoids extra mutex lock.

        QMutexLocker locker(&m_buffersCounterMutex);

        while (m_buffersCounter >= MAX_BUFFERS_IN_QUEUE)
            m_buffersCounterCondition.wait(&m_buffersCounterMutex);
    }
}

void AVFAudioDecoder::handleNewAudioBuffer(QAudioBuffer buffer)
{
    m_cachedBuffers.enqueue(buffer);
    ++m_buffersCounter;

    Q_ASSERT(m_cachedBuffers.size() == m_buffersCounter);

    bufferAvailableChanged(true);
    bufferReady();
}

/*
 * The method calls the passed functor in the thread of AVFAudioDecoder and guarantees that
 * the passed decoding context is not expired. In other words, it helps avoiding all callbacks
 * after stopping of the decoder.
 */
template<typename F>
void AVFAudioDecoder::invokeWithDecodingContext(std::weak_ptr<DecodingContext> weakContext, F &&f)
{
    if (!weakContext.expired())
        QMetaObject::invokeMethod(this, [=]() {
            // strong check: compare with actual decoding context.
            // Otherwise, the context can be temporary locked by one of dispatch queues.
            if (auto context = weakContext.lock(); context && context == m_decodingContext)
                f();
        });
}

#include "moc_avfaudiodecoder_p.cpp"
