/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtMultimedia of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt.android.multimedia;

import java.util.ArrayList;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;

public class QtAudioDeviceManager
{
    static private AudioManager m_audioManager = null;
    static private AudioHeadsetStateReceiver m_audioHeadsetStateReceiver = null;

    public static native void onAudioInputDevicesUpdated();
    public static native void onAudioOutputDevicesUpdated();

    static private class AudioHeadsetStateReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent) {
            onAudioOutputDevicesUpdated();
            onAudioInputDevicesUpdated();
        }
    }

    private static void registerAudioHeadsetStateReceiver(Context context) {
        if (m_audioHeadsetStateReceiver == null) {
            m_audioHeadsetStateReceiver = new AudioHeadsetStateReceiver();
        }
        context.registerReceiver(m_audioHeadsetStateReceiver,
                                 new IntentFilter(AudioManager.ACTION_HEADSET_PLUG));
    }

    static public void setContext(Context context)
    {
        m_audioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
        registerAudioHeadsetStateReceiver(context);
    }

    private static String[] getAudioOutputDevices()
    {
        return getAudioDevices(AudioManager.GET_DEVICES_OUTPUTS);
    }

    private static String[] getAudioInputDevices()
    {
        return getAudioDevices(AudioManager.GET_DEVICES_INPUTS);
    }

    private static String audioDeviceTypeToString(int type)
    {
        switch (type)
        {
            case AudioDeviceInfo.TYPE_AUX_LINE:
                return "AUX Line";
            case AudioDeviceInfo.TYPE_BLUETOOTH_A2DP:
            case AudioDeviceInfo.TYPE_BLUETOOTH_SCO:
                return "Bluetooth";
            case AudioDeviceInfo.TYPE_BUILTIN_EARPIECE:
                return "Built in earpiece";
            case AudioDeviceInfo.TYPE_BUILTIN_MIC:
                return "Built in microphone";
            case AudioDeviceInfo.TYPE_BUILTIN_SPEAKER:
                return "Built in speaker";
            case AudioDeviceInfo.TYPE_DOCK:
                return "Dock";
            case AudioDeviceInfo.TYPE_FM:
                return "FM";
            case AudioDeviceInfo.TYPE_FM_TUNER:
                return "FM TUNER";
            case AudioDeviceInfo.TYPE_HDMI:
                return "HDMI";
            case AudioDeviceInfo.TYPE_HDMI_ARC:
                return "HDMI ARC";
            case AudioDeviceInfo.TYPE_IP:
                return "IP";
            case AudioDeviceInfo.TYPE_LINE_ANALOG:
                return "Line analog";
            case AudioDeviceInfo.TYPE_LINE_DIGITAL:
                return "Line digital";
            case AudioDeviceInfo.TYPE_TV_TUNER:
                return "TV tuner";
            case AudioDeviceInfo.TYPE_USB_ACCESSORY:
                return "USB accessory";
            case AudioDeviceInfo.TYPE_WIRED_HEADPHONES:
                return "Wired headphones";
            case AudioDeviceInfo.TYPE_WIRED_HEADSET:
                return "Wired headset";
            case AudioDeviceInfo.TYPE_TELEPHONY:
            case AudioDeviceInfo.TYPE_UNKNOWN:
            default:
                return "Unknown-Type";
        }
    }

    private static String[] getAudioDevices(int type)
    {
        ArrayList<String> devices = new ArrayList<String>();

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            boolean builtInMicAdded = false;
            boolean bluetoothDeviceAdded = false;
            for (AudioDeviceInfo deviceInfo : m_audioManager.getDevices(type)) {
                String deviceType = audioDeviceTypeToString(deviceInfo.getType());

                if (deviceType == audioDeviceTypeToString(AudioDeviceInfo.TYPE_UNKNOWN)) {
                    // Not supported device type
                    continue;
                }
                else if (deviceType == audioDeviceTypeToString(AudioDeviceInfo.TYPE_BUILTIN_MIC)) {
                    if (builtInMicAdded) {
                        // Built in mic already added. Second built in mic is CAMCORDER, but there is no reliable way of selecting it.
                        // AudioSource.MIC usually means the primary mic. AudioSource.CAMCORDER source might mean the secondary mic, but there's no guarantee.
                        // It depends e.g. on the physical placement of the mics. That's why we will not add built in microphone twice. Should we?
                        continue;
                    }
                    builtInMicAdded = true;
                }
                else if (deviceType == audioDeviceTypeToString(AudioDeviceInfo.TYPE_BLUETOOTH_A2DP)) {
                    if (bluetoothDeviceAdded) {
                        // Bluetooth device already added. Second device is just a different technology profille (like A2DP or SCO).
                        // We should not add the same device twice. Should we?
                        continue;
                    }
                    bluetoothDeviceAdded = true;
                }

                devices.add(deviceInfo.getId() + ":" + deviceType + " ("
                            + deviceInfo.getProductName().toString() +")");
            }
        }

        String[] ret = new String[devices.size()];
        ret = devices.toArray(ret);
        return ret;
    }
}
