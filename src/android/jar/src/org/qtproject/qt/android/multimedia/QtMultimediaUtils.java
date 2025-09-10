// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.app.Activity;
import android.content.Context;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.view.OrientationEventListener;
import android.webkit.MimeTypeMap;
import android.net.Uri;
import android.content.ContentResolver;
import android.os.Environment;
import android.media.MediaScannerConnection;
import java.lang.String;
import java.io.File;
import android.util.Log;

class QtMultimediaUtils
{
    static private class OrientationListener extends OrientationEventListener
    {
        static int deviceOrientation = 0;

        OrientationListener(Context context)
        {
            super(context);
        }

        @Override
        public void onOrientationChanged(int orientation)
        {
            if (orientation == ORIENTATION_UNKNOWN)
                return;

            deviceOrientation = orientation;
        }
    }

    static private Context m_context = null;
    static private OrientationListener m_orientationListener = null;
    private static final String QtTAG = "Qt QtMultimediaUtils";

    static void setActivity(Activity qtMainActivity, Object qtActivityDelegate)
    {
    }

    static void setContext(Context context)
    {
        m_context = context;
        m_orientationListener = new OrientationListener(context);
    }

    QtMultimediaUtils()
    {
    }

    static void enableOrientationListener(boolean enable)
    {
        if (enable)
            m_orientationListener.enable();
        else
            m_orientationListener.disable();
    }

    static int getDeviceOrientation()
    {
        return OrientationListener.deviceOrientation;
    }

    static String getDefaultMediaDirectory(int type)
    {
        String dirType = new String();
        switch (type) {
            case 0:
                dirType = Environment.DIRECTORY_MUSIC;
                break;
            case 1:
                dirType = Environment.DIRECTORY_MOVIES;
                break;
            case 2:
                dirType = Environment.DIRECTORY_DCIM;
                break;
            default:
                break;
        }

        File path = new File("");
        if (type == 3) {
            // There is no API for knowing the standard location for sounds
            // such as voice recording. Though, it's typically in the 'Sounds'
            // directory at the root of the external storage
            path = new File(Environment.getExternalStorageDirectory().getAbsolutePath()
                            + File.separator + "Sounds");
        } else {
            path = Environment.getExternalStoragePublicDirectory(dirType);
        }

        path.mkdirs(); // make sure the directory exists

        return path.getAbsolutePath();
    }

    static void registerMediaFile(String file)
    {
        MediaScannerConnection.scanFile(m_context, new String[] { file }, null, null);
    }

    static File getCacheDirectory() { return m_context.getCacheDir(); }

    /*
    The array of codecs is in the form:
        c2.qti.vp9.decoder
        c2.android.opus.encoder
        OMX.google.opus.decoder
    */
    private static String[] getMediaCodecs()
    {
        final MediaCodecList codecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        final MediaCodecInfo[] codecInfoArray = codecList.getCodecInfos();
        String[] codecs = new String[codecInfoArray.length];
        for (int i = 0; i < codecInfoArray.length; ++i)
            codecs[i] = codecInfoArray[i].getName();
        return codecs;
    }

    static String getMimeType(Context context, String url)
    {
        Uri parsedUri = Uri.parse(url);
        String type = null;

        try {
            String scheme = parsedUri.getScheme();
            if (scheme != null && scheme.contains("content")) {
                ContentResolver cR = context.getContentResolver();
                type = cR.getType(parsedUri);
            } else {
                String extension = MimeTypeMap.getFileExtensionFromUrl(url);
                if (extension != null)
                type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
            }
        } catch (Exception e) {
            Log.e(QtTAG, "getMimeType(): " + e.toString());
        }
        return type;
    }
}

