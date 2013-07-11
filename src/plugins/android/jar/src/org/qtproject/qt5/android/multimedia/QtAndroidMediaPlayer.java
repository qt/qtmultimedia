/****************************************************************************
 **
 ** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the QtMultimedia module of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and Digia.  For licensing terms and
 ** conditions see http://qt.digia.com/licensing.  For further information
 ** use the contact form at http://qt.digia.com/contact-us.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Digia gives you certain additional
 ** rights.  These rights are described in the Digia Qt LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3.0 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU General Public License version 3.0 requirements will be
 ** met: http://www.gnu.org/copyleft/gpl.html.
 **
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

package org.qtproject.qt5.android.multimedia;

import java.io.IOException;
import java.lang.String;

// API is level is < 9 unless marked otherwise.
import android.app.Activity;
import android.content.Context;
import android.media.MediaPlayer;
import android.net.Uri;
import android.util.Log;
import java.io.FileDescriptor;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;

public class QtAndroidMediaPlayer extends MediaPlayer
{
    // Native callback functions for MediaPlayer
    native public void onErrorNative(int what, int extra, long id);
    native public void onBufferingUpdateNative(int percent, long id);
    native public void onInfoNative(int what, int extra, long id);
    native public void onMediaPlayerInfoNative(int what, int extra, long id);
    native public void onVideoSizeChangedNative(int width, int height, long id);

    private Uri mUri = null;
    private final long mID;
    private boolean mMuted = false;
    private boolean mPreparing = false;
    private boolean mInitialized = false;
    private int mVolume = 100;
    private static final String TAG = "Qt MediaPlayer";
    private static Context mApplicationContext = null;

    final int MEDIA_PLAYER_INVALID_STATE = 1;
    final int MEDIA_PLAYER_PREPARING = 2;
    final int MEDIA_PLAYER_READY = 3;
    final int MEDIA_PLAYER_DURATION = 4;
    final int MEDIA_PLAYER_PROGRESS = 5;
    final int MEDIA_PLAYER_FINISHED = 6;

    // Activity set by Qt on load.
    static public void setActivity(final Activity activity)
    {
        try {
            mApplicationContext = activity.getApplicationContext();
        } catch(final Exception e) {
            Log.d(TAG, "" + e.getMessage());
        }
    }

    private class ProgressWatcher implements Runnable
    {
        @Override
        public void run()
        {
            final int duratation = getDuration();
            int currentPosition = getCurrentPosition();

            try {
                while (duratation >= currentPosition && isPlaying()) {
                    onMediaPlayerInfoNative(MEDIA_PLAYER_PROGRESS, currentPosition, mID);
                    Thread.sleep(1000);
                    currentPosition = getCurrentPosition();
                }
            } catch (final InterruptedException e) {
                Log.d(TAG, "" + e.getMessage());
                return;
            }
        }
    }

    /**
     * MediaPlayer OnErrorListener
     */
    private class MediaPlayerErrorListener
    implements MediaPlayer.OnErrorListener
    {
        @Override
        public boolean onError(final MediaPlayer mp,
                               final int what,
                               final int extra)
        {
            reset();
            onErrorNative(what, extra, mID);
            return true;
        }

    }

    /**
     * MediaPlayer OnBufferingListener
     */
    private class MediaPlayerBufferingListener
    implements MediaPlayer.OnBufferingUpdateListener
    {
        private int mBufferPercent = -1;
        @Override
        public void onBufferingUpdate(final android.media.MediaPlayer mp,
                                      final int percent)
        {
            // Avoid updates when percent is unchanged.
            // E.g., we keep getting updates when percent == 100
            if (mBufferPercent == percent)
                return;

            onBufferingUpdateNative((mBufferPercent = percent), mID);
        }

    }

    /**
     * MediaPlayer OnCompletionListener
     */
    private class MediaPlayerCompletionListener
    implements MediaPlayer.OnCompletionListener
    {
        @Override
        public void onCompletion(final MediaPlayer mp)
        {
            onMediaPlayerInfoNative(MEDIA_PLAYER_FINISHED, 0, mID);
            reset();
        }

    }

    /**
     * MediaPlayer OnInfoListener
     */
    private class MediaPlayerInfoListener
    implements MediaPlayer.OnInfoListener
    {
        @Override
        public boolean onInfo(final MediaPlayer mp,
                              final int what,
                              final int extra)
        {
            onInfoNative(what, extra, mID);
            return true;
        }

    }

    /**
     * MediaPlayer OnPreparedListener
     */
    private class MediaPlayerPreparedListener
    implements MediaPlayer.OnPreparedListener
    {

        @Override
        public void onPrepared(final MediaPlayer mp)
        {
            onMediaPlayerInfoNative(MEDIA_PLAYER_DURATION, getDuration(), mID);
            onMediaPlayerInfoNative(MEDIA_PLAYER_READY, 0, mID);
            mPreparing = false;
        }

    }

    /**
     * MediaPlayer OnSeekCompleteListener
     */
    private class MediaPlayerSeekCompleteListener
    implements MediaPlayer.OnSeekCompleteListener
    {

        @Override
        public void onSeekComplete(final MediaPlayer mp)
        {
            onMediaPlayerInfoNative(MEDIA_PLAYER_PROGRESS, getCurrentPosition(), mID);
        }

    }

    /**
     * MediaPlayer OnVideoSizeChangedListener
     */
    private class MediaPlayerVideoSizeChangedListener
    implements MediaPlayer.OnVideoSizeChangedListener
    {

        @Override
        public void onVideoSizeChanged(final MediaPlayer mp,
                                       final int width,
                                       final int height)
        {
            onVideoSizeChangedNative(width, height, mID);
        }

    }

    public QtAndroidMediaPlayer(final long id)
    {
        super();
        mID = id;
        setOnBufferingUpdateListener(new MediaPlayerBufferingListener());
        setOnCompletionListener(new MediaPlayerCompletionListener());
        setOnInfoListener(new MediaPlayerInfoListener());
        setOnSeekCompleteListener(new MediaPlayerSeekCompleteListener());
        setOnVideoSizeChangedListener(new MediaPlayerVideoSizeChangedListener());
        setOnErrorListener(new MediaPlayerErrorListener());
    }

    @Override
    public void start()
    {
        if (!mInitialized) {
            onMediaPlayerInfoNative(MEDIA_PLAYER_INVALID_STATE, 0, mID);
            return;
        }

        if (mApplicationContext == null)
            return;

        if (mPreparing)
            return;

        if (isPlaying())
            return;

        try {
            super.start();
            Thread progressThread = new Thread(new ProgressWatcher());
            progressThread.start();
        } catch (final IllegalStateException e) {
            reset();
            Log.d(TAG, "" + e.getMessage());
        }
    }

    @Override
    public void pause()
    {
        if (!isPlaying())
            return;

        try {
            super.pause();
        } catch (final IllegalStateException e) {
            reset();
            Log.d(TAG, "" + e.getMessage());
        }
    }

    @Override
    public void stop()
    {
        if (!mInitialized)
            return;

        try {
            super.stop();
        } catch (final IllegalStateException e) {
            Log.d(TAG, "" + e.getMessage());
        } finally {
            reset();
        }
    }

    @Override
    public void seekTo(final int msec)
    {
        if (!mInitialized)
            return;

        try {
            super.seekTo(msec);
            onMediaPlayerInfoNative(MEDIA_PLAYER_PROGRESS, msec, mID);
        } catch (final IllegalStateException e) {
            Log.d(TAG, "" + e.getMessage());
        }
    }

    @Override
    public boolean isPlaying()
    {
        boolean playing = false;

        if (!mInitialized)
            return playing;

        try {
            playing = super.isPlaying();
        } catch (final IllegalStateException e) {
            Log.d(TAG, "" + e.getMessage());
        }

        return playing;
    }

    public void setMediaPath(final String path)
    {
        if (mInitialized)
            reset();

        try {
            mPreparing = true;
            onMediaPlayerInfoNative(MEDIA_PLAYER_PREPARING, 0, mID);
            mUri = Uri.parse(path);
            if (mUri.getScheme().compareTo("assets") == 0) {
                final String asset = mUri.getPath().substring(1 /* Remove first '/' */);
                final AssetManager am = mApplicationContext.getAssets();
                final AssetFileDescriptor afd = am.openFd(asset);
                final long offset = afd.getStartOffset();
                final long length = afd.getLength();
                FileDescriptor fd = afd.getFileDescriptor();
                setDataSource(fd, offset, length);
            } else {
                setDataSource(mApplicationContext, mUri);
            }
            mInitialized = true;
            setOnPreparedListener(new MediaPlayerPreparedListener());
            prepareAsync();
        } catch (final IOException e) {
            mPreparing = false;
            onErrorNative(MEDIA_ERROR_UNKNOWN,
                          /* MEDIA_ERROR_UNSUPPORTED= */ -1010,
                          mID);
        } catch (final IllegalArgumentException e) {
            Log.d(TAG, "" + e.getMessage());
        } catch (final SecurityException e) {
            Log.d(TAG, "" + e.getMessage());
        } catch (final IllegalStateException e) {
            Log.d(TAG, "" + e.getMessage());
        } catch (final NullPointerException e) {
            Log.d(TAG, "" + e.getMessage());
        }
    }

   @Override
   public int getCurrentPosition()
   {
       int currentPosition = 0;

       if (!mInitialized)
           return currentPosition;

       try {
           currentPosition = super.getCurrentPosition();
       } catch (final IllegalStateException e) {
           Log.d(TAG, "" + e.getMessage());
       }

       return currentPosition;
   }

   @Override
   public int getDuration()
   {
       int duration = 0;

       if (!mInitialized)
           return duration;

       try {
           duration = super.getDuration();
       } catch (final IllegalStateException e) {
           Log.d(TAG, "" + e.getMessage());
       }

       return duration;
   }

   private float adjustVolume(final int volume)
   {
       if (volume < 1)
           return 0.0f;

       if (volume > 98)
           return 1.0f;

       return (float) (1-(Math.log(100-volume)/Math.log(100)));
   }

   public void setVolume(int volume)
   {
       if (volume < 0)
           volume = 0;

       if (volume > 100)
           volume = 100;

       float newVolume = adjustVolume(volume);

       try {
           super.setVolume(newVolume, newVolume);
           if (!mMuted)
               mVolume = volume;
       } catch (final IllegalStateException e) {
           Log.d(TAG, "" + e.getMessage());
       }
   }

   public int getVolume()
   {
       return mVolume;
   }

    public void mute(final boolean mute)
    {
        mMuted = mute;
        setVolume(mute ? 0 : mVolume);
    }

    public boolean isMuted()
    {
        return mMuted;
    }

    @Override
    public void reset()
    {
        mInitialized = false;
        super.reset();
    }

}
