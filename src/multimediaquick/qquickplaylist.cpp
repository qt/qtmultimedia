// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickplaylist_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype PlaylistItem
    \instantiates QQuickPlaylistItem
    \since 5.6

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup multimedia_video_qml
    \brief Defines an item in a Playlist.
    \internal

    \sa Playlist
*/

/*!
    \qmlproperty url QtMultimedia::PlaylistItem::source

    This property holds the source URL of the item.

    \sa Playlist
*/
QQuickPlaylistItem::QQuickPlaylistItem(QObject *parent)
    : QObject(parent)
{
}

QUrl QQuickPlaylistItem::source() const
{
    return m_source;
}

void QQuickPlaylistItem::setSource(const QUrl &source)
{
    m_source = source;
}

/*!
    \qmltype Playlist
    \instantiates QQuickPlaylist
    \since 5.6
    \brief For specifying a list of media to be played.
    \internal

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup multimedia_video_qml

    The Playlist type provides a way to play a list of media with the MediaPlayer, Audio and Video
    types. It can be used as a data source for view elements (such as ListView) and other elements
    that interact with model data (such as Repeater). When used as a data model, each playlist
    item's source URL can be accessed using the \c source role.

    \qml
    Item {
        width: 400;
        height: 300;

        Audio {
            id: player;
            playlist: Playlist {
                id: playlist
                PlaylistItem { source: "song1.ogg"; }
                PlaylistItem { source: "song2.ogg"; }
                PlaylistItem { source: "song3.ogg"; }
            }
        }

        ListView {
            model: playlist;
            delegate: Text {
                font.pixelSize: 16;
                text: source;
            }
        }

        MouseArea {
            anchors.fill: parent;
            onPressed: {
                if (player.playbackState != Audio.PlayingState) {
                    player.play();
                } else {
                    player.pause();
                }
            }
        }
    }
    \endqml

    \sa MediaPlayer, Audio, Video
*/

void QQuickPlaylist::_q_mediaAboutToBeInserted(int start, int end)
{
    emit itemAboutToBeInserted(start, end);

    beginInsertRows(QModelIndex(), start, end);
}

void QQuickPlaylist::_q_mediaInserted(int start, int end)
{
    endInsertRows();

    emit itemCountChanged();
    emit itemInserted(start, end);
}

void QQuickPlaylist::_q_mediaAboutToBeRemoved(int start, int end)
{
    emit itemAboutToBeRemoved(start, end);

    beginRemoveRows(QModelIndex(), start, end);
}

void QQuickPlaylist::_q_mediaRemoved(int start, int end)
{
    endRemoveRows();

    emit itemCountChanged();
    emit itemRemoved(start, end);
}

void QQuickPlaylist::_q_mediaChanged(int start, int end)
{
    emit dataChanged(createIndex(start, 0), createIndex(end, 0));
    emit itemChanged(start, end);
}

void QQuickPlaylist::_q_loadFailed()
{
    m_error = m_playlist->error();
    m_errorString = m_playlist->errorString();

    emit error(Error(m_error), m_errorString);
    emit errorChanged();
    emit loadFailed();
}

QQuickPlaylist::QQuickPlaylist(QObject *parent)
    : QAbstractListModel(parent)
    , m_playlist(nullptr)
    , m_error(QMediaPlaylist::NoError)
{
}

QQuickPlaylist::~QQuickPlaylist()
{
    delete m_playlist;
}

/*!
    \qmlproperty enumeration QtMultimedia::Playlist::playbackMode

    This property holds the order in which items in the playlist are played.

    \table
    \header \li Value \li Description
    \row \li CurrentItemOnce
        \li The current item is played only once.
    \row \li CurrentItemInLoop
        \li The current item is played repeatedly in a loop.
    \row \li Sequential
        \li Playback starts from the current and moves through each successive item until the last
           is reached and then stops. The next item is a null item when the last one is currently
           playing.
    \row \li Loop
        \li Playback restarts at the first item after the last has finished playing.
    \row \li Random
        \li Play items in random order.
    \endtable
 */
QQuickPlaylist::PlaybackMode QQuickPlaylist::playbackMode() const
{
    return PlaybackMode(m_playlist->playbackMode());
}

void QQuickPlaylist::setPlaybackMode(PlaybackMode mode)
{
    if (playbackMode() == mode)
        return;

    m_playlist->setPlaybackMode(QMediaPlaylist::PlaybackMode(mode));
}

/*!
    \qmlproperty url QtMultimedia::Playlist::currentItemsource

    This property holds the source URL of the current item in the playlist.
 */
QUrl QQuickPlaylist::currentItemSource() const
{
    return m_playlist->currentMedia();
}

/*!
    \qmlproperty int QtMultimedia::Playlist::currentIndex

    This property holds the position of the current item in the playlist.
 */
int QQuickPlaylist::currentIndex() const
{
    return m_playlist->currentIndex();
}

void QQuickPlaylist::setCurrentIndex(int index)
{
    if (currentIndex() == index)
        return;

    m_playlist->setCurrentIndex(index);
}

/*!
    \qmlproperty int QtMultimedia::Playlist::itemCount

    This property holds the number of items in the playlist.
 */
int QQuickPlaylist::itemCount() const
{
    return m_playlist->mediaCount();
}

/*!
    \qmlproperty enumeration QtMultimedia::Playlist::error

    This property holds the error condition of the playlist.

    \table
    \header \li Value \li Description
    \row \li NoError
        \li No errors
    \row \li FormatError
        \li Format error.
    \row \li FormatNotSupportedError
        \li Format not supported.
    \row \li NetworkError
        \li Network error.
    \row \li AccessDeniedError
        \li Access denied error.
    \endtable
 */
QQuickPlaylist::Error QQuickPlaylist::error() const
{
    return Error(m_error);
}

/*!
    \qmlproperty string QtMultimedia::Playlist::errorString

    This property holds a string describing the current error condition of the playlist.
*/
QString QQuickPlaylist::errorString() const
{
    return m_errorString;
}

/*!
    \qmlmethod url QtMultimedia::Playlist::itemSource(index)

    Returns the source URL of the item at the given \a index in the playlist.
*/
QUrl QQuickPlaylist::itemSource(int index)
{
    return m_playlist->media(index);
}

/*!
    \qmlmethod int QtMultimedia::Playlist::nextIndex(steps)

    Returns the index of the item in the playlist which would be current after calling next()
    \a steps times.

    Returned value depends on the size of the playlist, the current position and the playback mode.

    \sa playbackMode, previousIndex()
*/
int QQuickPlaylist::nextIndex(int steps)
{
    return m_playlist->nextIndex(steps);
}

/*!
    \qmlmethod int QtMultimedia::Playlist::previousIndex(steps)

    Returns the index of the item in the playlist which would be current after calling previous()
    \a steps times.

    Returned value depends on the size of the playlist, the current position and the playback mode.

    \sa playbackMode, nextIndex()
*/
int QQuickPlaylist::previousIndex(int steps)
{
    return m_playlist->previousIndex(steps);
}

/*!
    \qmlmethod QtMultimedia::Playlist::next()

    Advances to the next item in the playlist.
*/
void QQuickPlaylist::next()
{
    m_playlist->next();
}

/*!
    \qmlmethod QtMultimedia::Playlist::previous()

    Returns to the previous item in the playlist.
*/
void QQuickPlaylist::previous()
{
    m_playlist->previous();
}

/*!
    \qmlmethod QtMultimedia::Playlist::shuffle()

    Shuffles items in the playlist.
*/
void QQuickPlaylist::shuffle()
{
    m_playlist->shuffle();
}

/*!
    \qmlmethod QtMultimedia::Playlist::load(location, format)

    Loads a playlist from the given \a location. If \a format is specified, it is used, otherwise
    the format is guessed from the location name and the data.

    New items are appended to the playlist.

    \c onloaded() is emitted if the playlist loads successfully, otherwise \c onLoadFailed() is
    emitted with \l error and \l errorString defined accordingly.
*/
void QQuickPlaylist::load(const QUrl &location, const QString &format)
{
    m_error = QMediaPlaylist::NoError;
    m_errorString = QString();
    emit errorChanged();
    m_playlist->load(location, format.toLatin1().constData());
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::save(location, format)

    Saves the playlist to the given \a location. If \a format is specified, it is used, otherwise
    the format is guessed from the location name.

    Returns true if the playlist is saved successfully.
*/
bool QQuickPlaylist::save(const QUrl &location, const QString &format)
{
    return m_playlist->save(location, format.toLatin1().constData());
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::addItem(source)

    Appends the \a source URL to the playlist.

    Returns true if the \a source is added successfully.
*/
void QQuickPlaylist::addItem(const QUrl &source)
{
    m_playlist->addMedia(QUrl(source));
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::addItems(sources)

    Appends the list of URLs in \a sources to the playlist.

    Returns true if the \a sources are added successfully.

    \since 5.7
*/
void QQuickPlaylist::addItems(const QList<QUrl> &sources)
{
    if (sources.isEmpty())
        return;

    QList<QUrl> contents;
    QList<QUrl>::const_iterator it = sources.constBegin();
    while (it != sources.constEnd()) {
        contents.push_back(QUrl(*it));
        ++it;
    }
    m_playlist->addMedia(contents);
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::insertItem(index, source)

    Inserts the \a source URL to the playlist at the given \a index.

    Returns true if the \a source is added successfully.
*/
bool QQuickPlaylist::insertItem(int index, const QUrl &source)
{
    return m_playlist->insertMedia(index, QUrl(source));
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::insertItems(index, sources)

    Inserts the list of URLs in \a sources to the playlist at the given \a index.

    Returns true if the \a sources are added successfully.

    \since 5.7
*/
bool QQuickPlaylist::insertItems(int index, const QList<QUrl> &sources)
{
    if (sources.empty())
        return false;

    QList<QUrl> contents;
    QList<QUrl>::const_iterator it = sources.constBegin();
    while (it != sources.constEnd()) {
        contents.push_back(QUrl(*it));
        ++it;
    }
    return m_playlist->insertMedia(index, contents);
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::moveItem(from, to)

    Moves the item at index position \a from to index position \a to.

    Returns \c true if the item is moved successfully.

    \since 5.7
*/
bool QQuickPlaylist::moveItem(int from, int to)
{
    return m_playlist->moveMedia(from, to);
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::removeItem(index)

    Removes the item at the given \a index from the playlist.

    Returns \c true if the item is removed successfully.
*/
bool QQuickPlaylist::removeItem(int index)
{
    return m_playlist->removeMedia(index);
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::removeItems(int start, int end)

    Removes items in the playlist from \a start to \a end inclusive.

    Returns \c true if the items are removed successfully.

    \since 5.7
*/
bool QQuickPlaylist::removeItems(int start, int end)
{
    return m_playlist->removeMedia(start, end);
}

/*!
    \qmlmethod bool QtMultimedia::Playlist::clear()

    Removes all the items from the playlist.

    Returns \c true if the operation is successful.
*/
void QQuickPlaylist::clear()
{
    m_playlist->clear();
}

int QQuickPlaylist::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_playlist->mediaCount();
}

QVariant QQuickPlaylist::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role);

    if (!index.isValid())
        return QVariant();

    return m_playlist->media(index.row());
}

QHash<int, QByteArray> QQuickPlaylist::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[SourceRole] = "source";
    return roleNames;
}

void QQuickPlaylist::classBegin()
{
    m_playlist = new QMediaPlaylist(this);

    connect(m_playlist, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(currentIndexChanged()));
    connect(m_playlist, SIGNAL(playbackModeChanged(QMediaPlaylist::PlaybackMode)),
            this, SIGNAL(playbackModeChanged()));
    connect(m_playlist, SIGNAL(currentMediaChanged(QUrl)),
            this, SIGNAL(currentItemSourceChanged()));
    connect(m_playlist, SIGNAL(mediaAboutToBeInserted(int,int)),
            this, SLOT(_q_mediaAboutToBeInserted(int,int)));
    connect(m_playlist, SIGNAL(mediaInserted(int,int)),
            this, SLOT(_q_mediaInserted(int,int)));
    connect(m_playlist, SIGNAL(mediaAboutToBeRemoved(int,int)),
            this, SLOT(_q_mediaAboutToBeRemoved(int,int)));
    connect(m_playlist, SIGNAL(mediaRemoved(int,int)),
            this, SLOT(_q_mediaRemoved(int,int)));
    connect(m_playlist, SIGNAL(mediaChanged(int,int)),
            this, SLOT(_q_mediaChanged(int,int)));
    connect(m_playlist, SIGNAL(loaded()),
            this, SIGNAL(loaded()));
    connect(m_playlist, SIGNAL(loadFailed()),
            this, SLOT(_q_loadFailed()));
}

void QQuickPlaylist::componentComplete()
{
}

/*!
    \qmlsignal QtMultimedia::Playlist::itemAboutToBeInserted(start, end)

    This signal is emitted when items are to be inserted into the playlist at \a start and ending at
    \a end.
*/

/*!
    \qmlsignal QtMultimedia::Playlist::itemInserted(start, end)

    This signal is emitted after items have been inserted into the playlist. The new items are those
    between \a start and \a end inclusive.
*/

/*!
    \qmlsignal QtMultimedia::Playlist::itemAboutToBeRemoved(start, end)

    This signal emitted when items are to be deleted from the playlist at \a start and ending at
    \a end.
*/

/*!
    \qmlsignal QtMultimedia::Playlist::itemRemoved(start, end)

    This signal is emitted after items have been removed from the playlist. The removed items are
    those between \a start and \a end inclusive.
*/

/*!
    \qmlsignal QtMultimedia::Playlist::itemChanged(start, end)

    This signal is emitted after items have been changed in the playlist between \a start and
    \a end positions inclusive.
*/

/*!
    \qmlsignal QtMultimedia::Playlist::loaded()

    This signal is emitted when the playlist loading succeeded.
*/

/*!
    \qmlsignal QtMultimedia::Playlist::loadFailed()

    This signal is emitted when the playlist loading failed. \l error and \l errorString can be
    checked for more information on the failure.
*/

QT_END_NAMESPACE

#include "moc_qquickplaylist_p.cpp"
