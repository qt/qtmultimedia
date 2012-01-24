/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <qtmultimediadefs.h>
#include <QtTest/QtTest>

#include <QtCore/qdir.h>

#include <qmediaimageviewer.h>
#include <private/qmediaimageviewerservice_p.h>
#include <qmediaplaylist.h>
#include <qmediaservice.h>
#include <qvideorenderercontrol.h>

#include <QtCore/qfile.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <qabstractvideosurface.h>
#include <qvideosurfaceformat.h>

QT_USE_NAMESPACE
class QtTestNetworkAccessManager;

class tst_QMediaImageViewer : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void isValid();
    void timeout();
    void setMedia_data();
    void setMedia();
    void setConsecutiveMedia();
    void setInvalidMedia();
    void playlist();
    void multiplePlaylists();
    void invalidPlaylist();
    void elapsedTime();
    void rendererControl();
    void setVideoOutput();
    void debugEnums();

    void mediaChanged_signal();

public:
    tst_QMediaImageViewer() : m_network(0) {}

private:
    QUrl imageUrl(const char *fileName) const {
        return QUrl(QLatin1String("qrc:///images/") + QLatin1String(fileName)); }
    QString imageFileName(const char *fileName) {
        return QLatin1String(":/images/") + QLatin1String(fileName); }

    QtTestNetworkAccessManager *m_network;
    QString m_fileProtocol;
};

class QtTestVideoSurface : public QAbstractVideoSurface
{
public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType) const {
        QList<QVideoFrame::PixelFormat> formats;
        if (handleType == QAbstractVideoBuffer::NoHandle) {
            formats << QVideoFrame::Format_RGB32;
        }
        return formats;
    }

    QVideoFrame frame() const { return m_frame;  }

    bool present(const QVideoFrame &frame) { m_frame = frame; return true; }

private:
    QVideoFrame m_frame;
};

class QtTestNetworkReply : public QNetworkReply
{
public:
    QtTestNetworkReply(
            const QNetworkRequest &request,
            const QByteArray &mimeType,
            QObject *parent)
        : QNetworkReply(parent)
    {
        setRequest(request);
        setOperation(QNetworkAccessManager::HeadOperation);
        setRawHeader("content-type", mimeType);

        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    }

    QtTestNetworkReply(
            const QNetworkRequest &request,
            const QByteArray &mimeType,
            const QString &fileName,
            QObject *parent)
        : QNetworkReply(parent)
        , m_file(fileName)
    {
        setRequest(request);
        setOperation(QNetworkAccessManager::GetOperation);
        setRawHeader("content-type", mimeType);

        if (m_file.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
            setOpenMode(QIODevice::ReadOnly);
        }

        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    }

    void abort() { m_file.close(); }

    bool atEnd () const { return m_file.atEnd(); }
    qint64 bytesAvailable() const { return m_file.bytesAvailable() + QIODevice::bytesAvailable(); }
    void close() { m_file.close(); setOpenMode(QIODevice::NotOpen); }
    bool isSequential() const { return true; }
    bool open(OpenMode) { return false; }
    qint64 pos() const { return 0; }
    bool seek(qint64) { return false; }
    qint64 size() const { return m_file.size(); }
    qint64 readData(char * data, qint64 maxSize) { return m_file.read(data, maxSize); }
    qint64 writeData(const char *, qint64) { return -1; }

protected:
    void customEvent(QEvent *event)
    {
        if (event->type() == QEvent::User) {
            event->accept();
            emit finished();
        }
    }

private:
    QFile m_file;
};

class QtTestNetworkAccessManager : public QNetworkAccessManager
{
public:
    QtTestNetworkAccessManager(QObject *parent = 0)
        : QNetworkAccessManager(parent)
    {
    }

    void appendDocument(const QUrl &url, const QByteArray &mimeType, const QString &fileName)
    {
        m_documents.append(Document(url, mimeType, fileName));
    }

protected:
    QNetworkReply *createRequest(
            Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0)
    {
        foreach (const Document &document, m_documents) {
            if (document.url == request.url()) {
                if (op == GetOperation) {
                    return new QtTestNetworkReply(
                            request, document.mimeType, document.fileName, this);
                } else if (op == HeadOperation) {
                    return new QtTestNetworkReply(request, document.mimeType, this);
                }
            }
        }
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }

private:
    struct Document
    {
        Document(const QUrl url, const QByteArray mimeType, const QString &fileName)
            : url(url), mimeType(mimeType), fileName(fileName)
        {
        }

        QUrl url;
        QByteArray mimeType;
        QString fileName;
    };

    QList<Document> m_documents;
};

void tst_QMediaImageViewer::initTestCase()
{
    qRegisterMetaType<QMediaImageViewer::State>();
    qRegisterMetaType<QMediaImageViewer::MediaStatus>();

    m_network = new QtTestNetworkAccessManager(this);

    m_network->appendDocument(
            QUrl(QLatin1String("test://image/png?id=1")),
            "image/png",
            imageFileName("image.png"));
    m_network->appendDocument(
            QUrl(QLatin1String("test://image/png?id=2")),
            QByteArray(),
            imageFileName("image.png"));
    m_network->appendDocument(
            QUrl(QLatin1String("test://image/invalid?id=1")),
            "image/png",
            imageFileName("invalid.png"));
    m_network->appendDocument(
            QUrl(QLatin1String("test://image/invalid?id=2")),
            QByteArray(),
            imageFileName("invalid.png"));
#ifdef QTEST_HAVE_JPEG
    m_network->appendDocument(
            QUrl(QLatin1String("test://image/jpeg?id=1")),
            "image/jpeg",
            imageFileName("image.jpg"));
#endif
    m_network->appendDocument(
            QUrl(QLatin1String("test://music/songs/mp3?id=1")),
             "audio/mpeg",
             QString());
    m_network->appendDocument(
            QUrl(QLatin1String("test://music/covers/small?id=1")),
            "image/png",
            imageFileName("coverart.png"));
    m_network->appendDocument(
            QUrl(QLatin1String("test://music/covers/large?id=1")),
            "image/png",
            imageFileName("coverart.png"));
    m_network->appendDocument(
            QUrl(QLatin1String("test://video/movies/mp4?id=1")),
            "video/mp4",
            QString());
    m_network->appendDocument(
            QUrl(QLatin1String("test://video/posters/png?id=1")),
            "image/png",
            imageFileName("poster.png"));
}

void tst_QMediaImageViewer::isValid()
{
    QMediaImageViewer viewer;

    QVERIFY(viewer.service() != 0);
}

void tst_QMediaImageViewer::timeout()
{
    QMediaImageViewer viewer;

    QCOMPARE(viewer.timeout(), 3000);

    viewer.setTimeout(0);
    QCOMPARE(viewer.timeout(), 0);

    viewer.setTimeout(45);
    QCOMPARE(viewer.timeout(), 45);

    viewer.setTimeout(-3000);
    QCOMPARE(viewer.timeout(), 0);
}

void tst_QMediaImageViewer::setMedia_data()
{
    QTest::addColumn<QMediaContent>("media");

    {
        QMediaContent media(imageUrl("image.png"));

        QTest::newRow("file: png image")
                << media;
    } {
        QMediaContent media(QUrl(QLatin1String("test://image/png?id=1")));

        QTest::newRow("network: png image")
                << media;
    } {
        QMediaContent media(QMediaResource(
                QUrl(QLatin1String("test://image/png?id=1")), QLatin1String("image/png")));

        QTest::newRow("network: png image, explicit mime type")
                << media;
    } {
        QMediaContent media(QUrl(QLatin1String("test://image/png?id=2")));

        QTest::newRow("network: png image, no mime type")
                << media;
#ifdef QTEST_HAVE_JPEG
    } {
        QMediaContent media(imageUrl("image.jpg"));

        QTest::newRow("file: jpg image")
                << media;
    } {
        QMediaContent media(QUrl(QLatin1String("test://image/jpeg?id=1")));

        QTest::newRow("network: jpg image")
                << media;
#endif
    }
}

void tst_QMediaImageViewer::setMedia()
{
    QFETCH(QMediaContent, media);

    QMediaImageViewer viewer;

    QMediaImageViewerService *service = qobject_cast<QMediaImageViewerService *>(viewer.service());
    service->setNetworkManager(m_network);

    connect(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    viewer.setMedia(media);
    QCOMPARE(viewer.media(), media);

    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    QTestEventLoop::instance().enterLoop(2);

    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
}

void tst_QMediaImageViewer::setConsecutiveMedia()
{
    QMediaContent fileMedia1(imageUrl("image.png"));
    QMediaContent fileMedia2(imageUrl("coverart.png"));
    QMediaContent networkMedia1(QUrl(QLatin1String("test://image/png?id=1")));
    QMediaContent networkMedia2(QUrl(QLatin1String("test://image/png?id=2")));

    QMediaImageViewer viewer;

    connect(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    viewer.setMedia(fileMedia1);
    viewer.setMedia(fileMedia2);

    QCOMPARE(viewer.media(), fileMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.media(), fileMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);

    QMediaImageViewerService *service = qobject_cast<QMediaImageViewerService *>(viewer.service());
    service->setNetworkManager(m_network);

    viewer.setMedia(networkMedia2);
    viewer.setMedia(networkMedia1);

    QCOMPARE(viewer.media(), networkMedia1);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.media(), networkMedia1);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);

    viewer.setMedia(fileMedia1);
    viewer.setMedia(networkMedia2);

    QCOMPARE(viewer.media(), networkMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.media(), networkMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);

    viewer.setMedia(fileMedia1);
    viewer.setMedia(networkMedia2);

    QCOMPARE(viewer.media(), networkMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.media(), networkMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);

    viewer.setMedia(networkMedia1);
    viewer.setMedia(fileMedia2);

    QCOMPARE(viewer.media(), fileMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.media(), fileMedia2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
}

void tst_QMediaImageViewer::setInvalidMedia()
{
    QMediaImageViewer viewer;
    viewer.setTimeout(250);

    QMediaImageViewerService *service = qobject_cast<QMediaImageViewerService *>(viewer.service());
    service->setNetworkManager(m_network);

    connect(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    {
        QMediaContent media(imageUrl("invalid.png"));

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

        QTestEventLoop::instance().enterLoop(2);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    } {
        QMediaContent media(imageUrl("deleted.png"));

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

        QTestEventLoop::instance().enterLoop(2);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    } {
        QMediaResource invalidResource(imageUrl("invalid.png"));
        QMediaResource deletedResource(imageUrl("deleted.png"));
        QMediaContent media(QMediaResourceList() << invalidResource << deletedResource);

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

        QTestEventLoop::instance().enterLoop(2);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    } {
        QMediaResource resource(imageUrl("image.png"), QLatin1String("audio/mpeg"));
        QMediaContent media(resource);

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    } {
        QMediaResource audioResource(imageUrl("image.png"), QLatin1String("audio/mpeg"));
        QMediaResource invalidResource(imageUrl("invalid.png"));
        QMediaContent media(QMediaResourceList() << audioResource << invalidResource);

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

        QTestEventLoop::instance().enterLoop(2);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    } {
        QMediaContent media(QUrl(QLatin1String("test://image/invalid?id=1")));

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

        QTestEventLoop::instance().enterLoop(2);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    } {
        QMediaContent media(QUrl(QLatin1String("test://image/invalid?id=2")));

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

        QTestEventLoop::instance().enterLoop(2);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    } {
        QMediaContent media(QUrl(QLatin1String("test://image/invalid?id=3")));

        viewer.setMedia(media);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

        QTestEventLoop::instance().enterLoop(2);
        QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::InvalidMedia);
        QCOMPARE(viewer.media(), media);
    }
}

void tst_QMediaImageViewer::playlist()
{
    QMediaContent imageMedia(imageUrl("image.png"));
    QMediaContent posterMedia(imageUrl("poster.png"));
    QMediaContent coverArtMedia(imageUrl("coverart.png"));

    QMediaImageViewer viewer;
    viewer.setTimeout(250);

    connect(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    QSignalSpy stateSpy(&viewer, SIGNAL(stateChanged(QMediaImageViewer::State)));

    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);

    // No playlist so can't exit stopped state.
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);
    viewer.pause();
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    QMediaPlaylist playlist;
    viewer.setPlaylist(&playlist);

    // Empty playlist so can't exit stopped state.
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);
    viewer.pause();
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 0);

    playlist.addMedia(imageMedia);
    playlist.addMedia(posterMedia);
    playlist.addMedia(coverArtMedia);

    // Play progresses immediately to the first image and starts loading.
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(qvariant_cast<QMediaImageViewer::State>(stateSpy.last().value(0)),
             QMediaImageViewer::PlayingState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);
    QCOMPARE(playlist.currentIndex(), 0);
    QCOMPARE(viewer.media(), imageMedia);

    // Image is loaded asynchronously.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 0);

    // Time out causes progression to second image, which starts loading.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);
    QCOMPARE(playlist.currentIndex(), 1);
    QCOMPARE(viewer.media(), posterMedia);

    // Image is loaded asynchronously.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 1);

    // Pausing stops progression at current image.
    viewer.pause();
    QCOMPARE(viewer.state(), QMediaImageViewer::PausedState);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(qvariant_cast<QMediaImageViewer::State>(stateSpy.last().value(0)),
             QMediaImageViewer::PausedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 1);

    // No time out.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::PausedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 1);

    // Resuming playback does not immediately progress to the next item
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 1);

    // Time out causes progression to next image, which starts loading.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);
    QCOMPARE(playlist.currentIndex(), 2);
    QCOMPARE(viewer.media(), coverArtMedia);

    // Image is loaded asynchronously.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 2);

    // Time out causes progression to end of list
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 4);
    QCOMPARE(qvariant_cast<QMediaImageViewer::State>(stateSpy.last().value(0)),
             QMediaImageViewer::StoppedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::NoMedia);
    QCOMPARE(playlist.currentIndex(), -1);
    QCOMPARE(viewer.media(), QMediaContent());

    // Stopped, no time out.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::NoMedia);
    QCOMPARE(playlist.currentIndex(), -1);

    // Play progresses immediately to the first image and starts loading.
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(stateSpy.count(), 5);
    QCOMPARE(qvariant_cast<QMediaImageViewer::State>(stateSpy.last().value(0)),
             QMediaImageViewer::PlayingState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);
    QCOMPARE(playlist.currentIndex(), 0);
    QCOMPARE(viewer.media(), imageMedia);

    // Image is loaded asynchronously.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 0);

    // Stop ends progress, but retains current index.
    viewer.stop();
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 6);
    QCOMPARE(qvariant_cast<QMediaImageViewer::State>(stateSpy.last().value(0)),
             QMediaImageViewer::StoppedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);
    QCOMPARE(playlist.currentIndex(), 0);
    QCOMPARE(viewer.media(), imageMedia);

    // Stoppped, No time out.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(playlist.currentIndex(), 0);
    QCOMPARE(viewer.media(), imageMedia);

    // Stop when already stopped doesn't emit additional signals.
    viewer.stop();
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 6);

    viewer.play();
    QCOMPARE(stateSpy.count(), 7);

    // Play when already playing doesn't emit additional signals.
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(stateSpy.count(), 7);

    playlist.next();
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    // Pausing while loading, doesn't stop loading.
    viewer.pause();
    QCOMPARE(viewer.state(), QMediaImageViewer::PausedState);
    QCOMPARE(stateSpy.count(), 8);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);

    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::PausedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadedMedia);

    // Pause while paused doesn't emit additional signals.
    viewer.pause();
    QCOMPARE(viewer.state(), QMediaImageViewer::PausedState);
    QCOMPARE(stateSpy.count(), 8);

    // Calling setMedia stops the playlist.
    viewer.setMedia(imageMedia);
    QCOMPARE(viewer.media(), imageMedia);
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(stateSpy.count(), 9);
    QCOMPARE(qvariant_cast<QMediaImageViewer::State>(stateSpy.last().value(0)),
             QMediaImageViewer::StoppedState);

}

void tst_QMediaImageViewer::multiplePlaylists()
{
    QMediaContent imageMedia(imageUrl("image.png"));
    QMediaContent posterMedia(imageUrl("poster.png"));
    QMediaContent coverArtMedia(imageUrl("coverart.png"));

    QMediaImageViewer viewer;

    QMediaPlaylist *playlist1 = new QMediaPlaylist;
    viewer.setPlaylist(playlist1);
    playlist1->addMedia(imageMedia);
    playlist1->addMedia(posterMedia);

    playlist1->setCurrentIndex(0);
    QCOMPARE(viewer.media(), imageMedia);

    QMediaPlaylist *playlist2 = new QMediaPlaylist;

    viewer.setPlaylist(playlist2);
    playlist2->addMedia(coverArtMedia);

    QVERIFY(viewer.media().isNull());

    playlist2->setCurrentIndex(0);
    QCOMPARE(viewer.media(), coverArtMedia);

    delete playlist2;
    QVERIFY(viewer.media().isNull());
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);

    viewer.setPlaylist(playlist1);
    playlist1->setCurrentIndex(0);
    QCOMPARE(viewer.media(), imageMedia);

    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);

    delete playlist1;
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
}


void tst_QMediaImageViewer::invalidPlaylist()
{
    QMediaContent imageMedia(imageUrl("image.png"));
    QMediaContent invalidMedia(imageUrl("invalid.png"));

    QMediaImageViewer viewer;
    viewer.setTimeout(250);

    connect(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    QSignalSpy stateSpy(&viewer, SIGNAL(stateChanged(QMediaImageViewer::State)));
    QSignalSpy statusSpy(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)));

    QMediaPlaylist playlist;
    viewer.setPlaylist(&playlist);
    playlist.addMedia(invalidMedia);
    playlist.addMedia(imageMedia);
    playlist.addMedia(invalidMedia);

    // Test play initially tries to load the first invalid image.
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);
    QCOMPARE(viewer.media(), invalidMedia);
    QCOMPARE(playlist.currentIndex(), 0);
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(qvariant_cast<QMediaImageViewer::MediaStatus>(statusSpy.value(0).value(0)),
             QMediaImageViewer::LoadingMedia);

    // Test status is changed to InvalidMedia, and loading of the next image is started immediately.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::LoadingMedia);
    QCOMPARE(viewer.media(), imageMedia);
    QCOMPARE(playlist.currentIndex(), 1);
    QCOMPARE(statusSpy.count(), 3);
    QCOMPARE(qvariant_cast<QMediaImageViewer::MediaStatus>(statusSpy.value(1).value(0)),
             QMediaImageViewer::InvalidMedia);
    QCOMPARE(qvariant_cast<QMediaImageViewer::MediaStatus>(statusSpy.value(2).value(0)),
             QMediaImageViewer::LoadingMedia);

    // Test if the last image is invalid, the image viewer is stopped.
    playlist.next();
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::NoMedia);
    QCOMPARE(playlist.currentIndex(), -1);
    QCOMPARE(stateSpy.count(), 2);

    playlist.setCurrentIndex(2);
    QTestEventLoop::instance().enterLoop(2);

    // Test play immediately moves to the next item if the current one is invalid, and no state
    // change signals are emitted if the viewer never effectively moves from the StoppedState.
    viewer.play();
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);
    QCOMPARE(viewer.mediaStatus(), QMediaImageViewer::NoMedia);
    QCOMPARE(playlist.currentIndex(), -1);
    QCOMPARE(stateSpy.count(), 2);
}

void tst_QMediaImageViewer::elapsedTime()
{
    QMediaContent imageMedia(imageUrl("image.png"));

    QMediaImageViewer viewer;
    viewer.setTimeout(250);
    viewer.setNotifyInterval(150);

    QSignalSpy spy(&viewer, SIGNAL(elapsedTimeChanged(int)));

    connect(&viewer, SIGNAL(elapsedTimeChanged(int)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));


    QMediaPlaylist playlist;
    viewer.setPlaylist(&playlist);
    playlist.addMedia(imageMedia);

    QCOMPARE(viewer.elapsedTime(), 0);

    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(spy.count(), 0);

    viewer.play();
    QCOMPARE(viewer.elapsedTime(), 0);

    // Emits an initial elapsed time at 0 milliseconds signal when the image is loaded.
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().value(0).toInt(), 0);

    // Emits a scheduled signal after the notify interval is up. The exact time will be a little
    // fuzzy.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(spy.count(), 2);
    QVERIFY(spy.last().value(0).toInt() != 0);

    // Pausing will emit a signal with the elapsed time when paused.
    viewer.pause();
    QCOMPARE(spy.count(), 3);
    QCOMPARE(viewer.elapsedTime(), spy.last().value(0).toInt());

    // No elapsed time signals will be emitted while paused.
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(spy.count(), 3);

    // Stopping a paused viewer resets the elapsed time to 0 with signals emitted.
    viewer.stop();
    QCOMPARE(viewer.elapsedTime(), 0);
    QCOMPARE(spy.count(), 4);
    QCOMPARE(spy.last().value(0).toInt(), 0);

    disconnect(&viewer, SIGNAL(elapsedTimeChanged(int)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    connect(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
        &QTestEventLoop::instance(), SLOT(exitLoop()));

    // Play until end.
    viewer.play();
    QTestEventLoop::instance().enterLoop(2);

    // Verify at least two more signals are emitted.
    // The second to last at the instant the timeout expired, and the last as it's reset when the
    // current media is cleared.
    QVERIFY(spy.count() >= 5);
    QCOMPARE(spy.value(spy.count() - 2).value(0).toInt(), 250);
    QCOMPARE(spy.value(spy.count() - 1).value(0).toInt(), 0);

    viewer.play();
    QTestEventLoop::instance().enterLoop(2);

    // Test extending the timeout applies to an already loaded image.
    viewer.setTimeout(10000);
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::PlayingState);

    // Test reducing the timeout applies to an already loaded image.
    viewer.setTimeout(1000);
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(viewer.state(), QMediaImageViewer::StoppedState);

}

void tst_QMediaImageViewer::rendererControl()
{
    QtTestVideoSurface surfaceA;
    QtTestVideoSurface surfaceB;
    QAbstractVideoSurface *nullSurface = 0;

    QMediaImageViewer viewer;

    QMediaService *service = viewer.service();
    if (service == 0)
        QSKIP("Image viewer object has no service.");

    QMediaControl *mediaControl = service->requestControl(QVideoRendererControl_iid);
    QVERIFY(mediaControl != 0);

    QVideoRendererControl *rendererControl = qobject_cast<QVideoRendererControl *>(mediaControl);
    QVERIFY(rendererControl != 0);

    rendererControl->setSurface(&surfaceA);
    QCOMPARE(rendererControl->surface(), (QAbstractVideoSurface *)&surfaceA);

    // Load an image so the viewer has some dimensions to work with.
    viewer.setMedia(QMediaContent(imageUrl("image.png")));

    connect(&viewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(2);

    if (viewer.mediaStatus() != QMediaImageViewer::LoadedMedia)
        QSKIP("failed to load test image");

    QCOMPARE(surfaceA.isActive(), true);

    {
        QVideoSurfaceFormat format = surfaceA.surfaceFormat();
        QCOMPARE(format.handleType(), QAbstractVideoBuffer::NoHandle);
        QCOMPARE(format.pixelFormat(), QVideoFrame::Format_RGB32);
        QCOMPARE(format.frameSize(), QSize(75, 50));

        QVideoFrame frame = surfaceA.frame();
        QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoFrame::Format_RGB32);
        QCOMPARE(frame.size(), QSize(75, 50));
    }
    // Test clearing the output stops the video surface.
    service->releaseControl(rendererControl);
    QCOMPARE(surfaceA.isActive(), false);

    // Test reseting the output restarts it.
    mediaControl = service->requestControl(QVideoRendererControl_iid);
    QVERIFY(mediaControl != 0);

    rendererControl = qobject_cast<QVideoRendererControl *>(mediaControl);
    rendererControl->setSurface(&surfaceA);
    QVERIFY(rendererControl != 0);
    {
        QVideoSurfaceFormat format = surfaceA.surfaceFormat();
        QCOMPARE(format.handleType(), QAbstractVideoBuffer::NoHandle);
        QCOMPARE(format.pixelFormat(), QVideoFrame::Format_RGB32);
        QCOMPARE(format.frameSize(), QSize(75, 50));

        QVideoFrame frame = surfaceA.frame();
        QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoFrame::Format_RGB32);
        QCOMPARE(frame.size(), QSize(75, 50));
    }

    // Test changing the surface while viewing an image stops the old surface and starts
    // the new one and presents the image.
    rendererControl->setSurface(&surfaceB);
    QCOMPARE(rendererControl->surface(), (QAbstractVideoSurface*)&surfaceB);

    QCOMPARE(surfaceA.isActive(), false);
    QCOMPARE(surfaceB.isActive(), true);

    QVideoSurfaceFormat format = surfaceB.surfaceFormat();
    QCOMPARE(format.handleType(), QAbstractVideoBuffer::NoHandle);
    QCOMPARE(format.pixelFormat(), QVideoFrame::Format_RGB32);
    QCOMPARE(format.frameSize(), QSize(75, 50));

    QVideoFrame frame = surfaceB.frame();
    QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
    QCOMPARE(frame.pixelFormat(), QVideoFrame::Format_RGB32);
    QCOMPARE(frame.size(), QSize(75, 50));

    // Test setting null media stops the surface.
    viewer.setMedia(QMediaContent());
    QCOMPARE(surfaceB.isActive(), false);

    // Test the renderer control accepts a null surface.
    rendererControl->setSurface(0);
    QCOMPARE(rendererControl->surface(), nullSurface);
}

void tst_QMediaImageViewer::setVideoOutput()
{
    QMediaImageViewer imageViewer;
    imageViewer.setMedia(QMediaContent(imageUrl("image.png")));

    connect(&imageViewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(2);

    if (imageViewer.mediaStatus() != QMediaImageViewer::LoadedMedia)
        QSKIP("failed to load test image");

    QtTestVideoSurface surface;

    imageViewer.setVideoOutput(reinterpret_cast<QVideoWidget *>(0));

    imageViewer.setVideoOutput(reinterpret_cast<QGraphicsVideoItem *>(0));

    imageViewer.setVideoOutput(&surface);
    QVERIFY(surface.isActive());

    imageViewer.setVideoOutput(reinterpret_cast<QAbstractVideoSurface *>(0));
    QVERIFY(!surface.isActive());

    imageViewer.setVideoOutput(&surface);
    QVERIFY(surface.isActive());

    imageViewer.setVideoOutput(reinterpret_cast<QVideoWidget *>(0));
    QVERIFY(!surface.isActive());

    imageViewer.setVideoOutput(&surface);
    QVERIFY(surface.isActive());
}

void tst_QMediaImageViewer::debugEnums()
{
    QTest::ignoreMessage(QtDebugMsg, "QMediaImageViewer::PlayingState ");
    qDebug() << QMediaImageViewer::PlayingState;
    QTest::ignoreMessage(QtDebugMsg, "QMediaImageViewer::NoMedia ");
    qDebug() << QMediaImageViewer::NoMedia;
}

void tst_QMediaImageViewer::mediaChanged_signal()
{
    QMediaContent imageMedia(imageUrl("image.png"));
    QMediaImageViewer viewer;
    viewer.setTimeout(250);
    viewer.setNotifyInterval(150);

    QSignalSpy spy(&viewer, SIGNAL(mediaChanged(QMediaContent)));
    QVERIFY(spy.size() == 0);

    viewer.setMedia(imageMedia);
    QVERIFY(spy.size() == 1);
}

QTEST_MAIN(tst_QMediaImageViewer)

#include "tst_qmediaimageviewer.moc"
