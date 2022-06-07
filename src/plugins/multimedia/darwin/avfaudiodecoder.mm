// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfaudiodecoder_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/qiodevice.h>
#include <QMimeDatabase>
#include "private/qcoreaudioutils_p.h"

#include <AVFoundation/AVFoundation.h>

#define MAX_BUFFERS_IN_QUEUE 10

QT_USE_NAMESPACE

@interface AVFResourceReaderDelegate : NSObject <AVAssetResourceLoaderDelegate>
{
    AVFAudioDecoder *m_decoder;
    QMutex m_mutex;
}

-(void)handleNextSampleBuffer:(CMSampleBufferRef)sampleBuffer;

-(BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader
       shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest;

@end

@implementation AVFResourceReaderDelegate

-(id)initWithDecoder: (AVFAudioDecoder *)decoder {
    if (!(self = [super init]))
        return nil;

    m_decoder = decoder;

    return self;
}

-(void)dealloc {
    m_decoder = nil;
    [super dealloc];
}

-(void)handleNextSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    if (!sampleBuffer)
        return;

    // Check format
    CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(sampleBuffer);
    if (!formatDescription)
        return;
    const AudioStreamBasicDescription* const asbd = CMAudioFormatDescriptionGetStreamBasicDescription(formatDescription);
    QAudioFormat qtFormat = CoreAudioUtils::toQAudioFormat(*asbd);
    if (qtFormat.sampleFormat() == QAudioFormat::Unknown && asbd->mBitsPerChannel == 8)
        qtFormat.setSampleFormat(QAudioFormat::UInt8);
    if (!qtFormat.isValid())
        return;

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
        return;

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
        return;
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

    QAudioBuffer audioBuffer;
    audioBuffer = QAudioBuffer(abuf, qtFormat, qint64(sampleStartTimeSecs * 1000000));
    if (!audioBuffer.isValid())
        return;

    emit m_decoder->newAudioBuffer(audioBuffer);
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

AVFAudioDecoder::AVFAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
{
    m_readingQueue = dispatch_queue_create("reader_queue", DISPATCH_QUEUE_SERIAL);
    m_decodingQueue = dispatch_queue_create("decoder_queue", DISPATCH_QUEUE_SERIAL);

    m_readerDelegate = [[AVFResourceReaderDelegate alloc] initWithDecoder:this];

    connect(this, &AVFAudioDecoder::readyToRead, this, &AVFAudioDecoder::startReading);
    connect(this, &AVFAudioDecoder::newAudioBuffer, this, &AVFAudioDecoder::handleNewAudioBuffer);
}

AVFAudioDecoder::~AVFAudioDecoder()
{
    stop();

    [m_readerOutput release];
    m_readerOutput = nil;

    [m_reader release];
    m_reader = nil;

    [m_readerDelegate release];
    m_readerDelegate = nil;

    [m_asset release];
    m_asset = nil;

    if (m_readingQueue)
        dispatch_release(m_readingQueue);
    if (m_decodingQueue)
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

    emit sourceChanged();
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
        [m_asset.resourceLoader setDelegate:m_readerDelegate queue:m_readingQueue];

        m_loadingSource = true;
    }

    emit sourceChanged();
}

void AVFAudioDecoder::start()
{
    Q_ASSERT(!m_buffersAvailable);
    if (isDecoding())
        return;

    if (m_position != -1) {
        m_position = -1;
        emit positionChanged(-1);
    }

    if (m_device && (!m_device->isOpen() || !m_device->isReadable())) {
        processInvalidMedia(QAudioDecoder::ResourceError, tr("Unable to read from specified device"));
        return;
    }

    [m_asset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:
        ^{
            dispatch_async(m_readingQueue,
            ^{
                NSError *error = nil;
                AVKeyValueStatus status = [m_asset statusOfValueForKey:@"tracks" error:&error];
                if (status != AVKeyValueStatusLoaded) {
                    if (status == AVKeyValueStatusFailed) {
                        if (error.domain == NSURLErrorDomain)
                            processInvalidMedia(QAudioDecoder::ResourceError, QString::fromNSString(error.localizedDescription));
                        else
                            processInvalidMedia(QAudioDecoder::FormatError, tr("Could not load media source's tracks"));
                    }
                    return;
                }
                initAssetReader();
            });
        }
    ];

    if (m_device && m_loadingSource) {
        setIsDecoding(true);
        return;
    }
}

void AVFAudioDecoder::stop()
{
    m_cachedBuffers.clear();

    if (m_reader)
        [m_reader cancelReading];

    if (m_buffersAvailable != 0) {
        m_buffersAvailable = 0;
        emit bufferAvailableChanged(false);
    }
    if (m_position != -1) {
        m_position = -1;
        emit positionChanged(m_position);
    }
    if (m_duration != -1) {
        m_duration = -1;
        emit durationChanged(m_duration);
    }
    setIsDecoding(false);
}

QAudioFormat AVFAudioDecoder::audioFormat() const
{
    return m_format;
}

void AVFAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (m_format != format) {
        m_format = format;
        emit formatChanged(m_format);
    }
}

QAudioBuffer AVFAudioDecoder::read()
{
    if (!m_buffersAvailable)
        return QAudioBuffer();

    Q_ASSERT(m_cachedBuffers.size() > 0);
    QAudioBuffer buffer = m_cachedBuffers.takeFirst();

    m_position = qint64(buffer.startTime() / 1000);
    emit positionChanged(m_position);

    m_buffersAvailable--;
    if (!m_buffersAvailable)
        emit bufferAvailableChanged(false);
    return buffer;
}

bool AVFAudioDecoder::bufferAvailable() const
{
    return m_buffersAvailable > 0;
}

qint64 AVFAudioDecoder::position() const
{
    return m_position;
}

qint64 AVFAudioDecoder::duration() const
{
    return m_duration;
}

void AVFAudioDecoder::processInvalidMedia(QAudioDecoder::Error errorCode, const QString& errorString)
{
    stop();
    emit error(int(errorCode), errorString);
}

void AVFAudioDecoder::initAssetReader()
{
    if (!m_asset)
        return;

    NSArray<AVAssetTrack *> *tracks = [m_asset tracksWithMediaType:AVMediaTypeAudio];
    if (!tracks.count) {
        processInvalidMedia(QAudioDecoder::FormatError, tr("No audio tracks found"));
        return;
    }
    AVAssetTrack *track = [tracks objectAtIndex:0];

    // Set format
    QAudioFormat format;
    if (m_format.isValid()) {
        format = m_format;
    } else {
        format = qt_format_for_audio_track(track);
        if (!format.isValid())
        {
            processInvalidMedia(QAudioDecoder::FormatError, tr("Unsupported source format"));
            return;
        }
    }

    // Set duration
    qint64 duration = CMTimeGetSeconds(track.timeRange.duration) * 1000;
    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged(m_duration);
    }

    // Initialize asset reader and output
    [m_reader release];
    m_reader = nil;
    [m_readerOutput release];
    m_readerOutput = nil;

    NSError *error = nil;
    NSDictionary *audioSettings = av_audio_settings_for_format(format);
    m_readerOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:track outputSettings:audioSettings];
    m_reader = [[AVAssetReader alloc] initWithAsset:m_asset error:&error];
    if (error) {
        processInvalidMedia(QAudioDecoder::ResourceError, QString::fromNSString(error.localizedDescription));
        return;
    }
    if (![m_reader canAddOutput:m_readerOutput]) {
        processInvalidMedia(QAudioDecoder::ResourceError, tr("Failed to add asset reader output"));
        return;
    }
    [m_reader addOutput:m_readerOutput];

    emit readyToRead();
}

void AVFAudioDecoder::startReading()
{
    m_loadingSource = false;

    // Prepares the receiver for obtaining sample buffers from the asset.
    if (!m_reader || ![m_reader startReading]) {
        processInvalidMedia(QAudioDecoder::ResourceError, tr("Could not start reading"));
        return;
    }

    setIsDecoding(true);

    // Since copyNextSampleBuffer is synchronous, submit it to an async dispatch queue
    // to run in a separate thread. Call the handleNextSampleBuffer "callback" on another
    // thread when new audio sample is read.
    dispatch_async(m_readingQueue, ^{
        CMSampleBufferRef sampleBuffer;
        while ((sampleBuffer = [m_readerOutput copyNextSampleBuffer])) {
            dispatch_async(m_decodingQueue, ^{
                if (CMSampleBufferDataIsReady(sampleBuffer))
                    [m_readerDelegate handleNextSampleBuffer:sampleBuffer];
                CFRelease(sampleBuffer);
            });
        }
        if (m_reader.status == AVAssetReaderStatusCompleted)
            emit finished();
    });
}

void AVFAudioDecoder::handleNewAudioBuffer(QAudioBuffer buffer)
{
    Q_ASSERT(m_cachedBuffers.size() <= MAX_BUFFERS_IN_QUEUE);
    m_cachedBuffers.push_back(buffer);

    m_buffersAvailable++;
    Q_ASSERT(m_buffersAvailable <= MAX_BUFFERS_IN_QUEUE);

    emit bufferAvailableChanged(true);
    emit bufferReady();
}
