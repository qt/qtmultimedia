// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import java.util.ArrayList;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;

public class QtAudioDeviceManager
{
    private static final String TAG = "QtAudioDeviceManager";
    static private AudioManager m_audioManager = null;
    static private final AudioDevicesReceiver m_audioDevicesReceiver = new AudioDevicesReceiver();
    static private AudioRecord m_recorder = null;
    static private AudioTrack m_streamPlayer = null;
    static private Thread m_streamingThread = null;
    static private boolean m_isStreaming = false;
    static private final int m_sampleRate = 8000;
    static private final int m_channels = AudioFormat.CHANNEL_CONFIGURATION_MONO;
    static private final int m_audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    static private final int m_bufferSize = AudioRecord.getMinBufferSize(m_sampleRate, m_channels, m_audioFormat);

    public static native void onAudioInputDevicesUpdated();
    public static native void onAudioOutputDevicesUpdated();

    static private class AudioDevicesReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent) {
             onAudioInputDevicesUpdated();
            onAudioOutputDevicesUpdated();
        }
    }

    public static void registerAudioHeadsetStateReceiver(Context context)
    {
        IntentFilter audioDevicesFilter = new IntentFilter();
        audioDevicesFilter.addAction(AudioManager.ACTION_HEADSET_PLUG);
        audioDevicesFilter.addAction(AudioManager.ACTION_HDMI_AUDIO_PLUG);
        audioDevicesFilter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        audioDevicesFilter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        audioDevicesFilter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECT_REQUESTED);
        audioDevicesFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        audioDevicesFilter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        audioDevicesFilter.addAction(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
        audioDevicesFilter.addAction(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED);
        audioDevicesFilter.addAction(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
        audioDevicesFilter.addAction(BluetoothA2dp.ACTION_PLAYING_STATE_CHANGED);

        context.registerReceiver(m_audioDevicesReceiver, audioDevicesFilter);
    }

    static public void setContext(Context context)
    {
        m_audioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
    }

    private static String[] getAudioOutputDevices()
    {
        return getAudioDevices(AudioManager.GET_DEVICES_OUTPUTS);
    }

    private static String[] getAudioInputDevices()
    {
        return getAudioDevices(AudioManager.GET_DEVICES_INPUTS);
    }

    private static boolean isBluetoothDevice(AudioDeviceInfo deviceInfo)
    {
        switch (deviceInfo.getType()) {
        case AudioDeviceInfo.TYPE_BLUETOOTH_A2DP:
        case AudioDeviceInfo.TYPE_BLUETOOTH_SCO:
            return true;
        default:
            return false;
        }
    }

    private static boolean setAudioInput(MediaRecorder recorder, int id)
    {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.P)
            return false;

        final AudioDeviceInfo[] audioDevices =
                m_audioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);

        for (AudioDeviceInfo deviceInfo : audioDevices) {
            if (deviceInfo.getId() != id)
                continue;

            boolean isPreferred = recorder.setPreferredDevice(deviceInfo);
            if (isPreferred && isBluetoothDevice(deviceInfo)) {
                m_audioManager.startBluetoothSco();
                m_audioManager.setBluetoothScoOn(true);
            }

            return isPreferred;
        }

        return false;
    }

    private static void setInputMuted(boolean mute)
    {
        // This method mutes the microphone across the entire platform
        m_audioManager.setMicrophoneMute(mute);
    }

    private static boolean isMicrophoneMute()
    {
        return m_audioManager.isMicrophoneMute();
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
        ArrayList<String> devices = new ArrayList<>();

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            boolean builtInMicAdded = false;
            boolean bluetoothDeviceAdded = false;
            for (AudioDeviceInfo deviceInfo : m_audioManager.getDevices(type)) {
                String deviceType = audioDeviceTypeToString(deviceInfo.getType());

                if (deviceType.equals(audioDeviceTypeToString(AudioDeviceInfo.TYPE_UNKNOWN))) {
                    // Not supported device type
                    continue;
                } else if (deviceType.equals(audioDeviceTypeToString(AudioDeviceInfo.TYPE_BUILTIN_MIC))) {
                    if (builtInMicAdded) {
                        // Built in mic already added. Second built in mic is CAMCORDER, but there
                        // is no reliable way of selecting it. AudioSource.MIC usually means the
                        // primary mic. AudioSource.CAMCORDER source might mean the secondary mic,
                        // but there's no guarantee. It depends e.g. on the physical placement
                        // of the mics. That's why we will not add built in microphone twice.
                        // Should we?
                        continue;
                    }
                    builtInMicAdded = true;
                } else if (isBluetoothDevice(deviceInfo)) {
                    if (bluetoothDeviceAdded) {
                        // Bluetooth device already added. Second device is just a different
                        // technology profille (like A2DP or SCO). We should not add the same
                        // device twice. Should we?
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

    private static boolean setAudioOutput(int id)
    {
        final AudioDeviceInfo[] audioDevices =
                                        m_audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        for (AudioDeviceInfo deviceInfo : audioDevices) {
           if (deviceInfo.getId() == id) {
               switch (deviceInfo.getType())
               {
                   case AudioDeviceInfo.TYPE_BLUETOOTH_A2DP:
                   case AudioDeviceInfo.TYPE_BLUETOOTH_SCO:
                       setAudioOutput(AudioManager.MODE_IN_COMMUNICATION, true, false);
                       return true;
                   case AudioDeviceInfo.TYPE_BUILTIN_SPEAKER:
                       setAudioOutput(AudioManager.STREAM_MUSIC, false, true);
                       return true;
                   case AudioDeviceInfo.TYPE_WIRED_HEADSET:
                   case AudioDeviceInfo.TYPE_WIRED_HEADPHONES:
                       setAudioOutput(AudioManager.MODE_IN_COMMUNICATION, false, false);
                       return true;
                   case AudioDeviceInfo.TYPE_BUILTIN_EARPIECE:
                       // It doesn't work when WIRED HEADPHONES are connected
                       // Earpiece has the lowest priority and setWiredHeadsetOn(boolean)
                       // method to force it is deprecated
                       Log.w(TAG, "Built in Earpiece may not work when "
                             + "Wired Headphones are connected");
                       setAudioOutput(AudioManager.MODE_IN_CALL, false, false);
                       return true;
                   default:
                       return false;
               }
           }
        }
        return false;
    }

    private static void setAudioOutput(int mode, boolean bluetoothOn, boolean speakerOn)
    {
        m_audioManager.setMode(mode);
        if (bluetoothOn) {
            m_audioManager.startBluetoothSco();
        } else {
            m_audioManager.stopBluetoothSco();
        }
        m_audioManager.setBluetoothScoOn(bluetoothOn);
        m_audioManager.setSpeakerphoneOn(speakerOn);

    }

    private static void streamSound()
    {
        byte data[] = new byte[m_bufferSize];
        while (m_isStreaming) {
            m_recorder.read(data, 0, m_bufferSize);
            m_streamPlayer.play();
            m_streamPlayer.write(data, 0, m_bufferSize);
            m_streamPlayer.stop();
        }
    }

    private static void startSoundStreaming(int inputId, int outputId)
    {
        if (m_isStreaming)
            stopSoundStreaming();

        m_recorder = new AudioRecord(MediaRecorder.AudioSource.DEFAULT, m_sampleRate, m_channels,
                                           m_audioFormat, m_bufferSize);
        m_streamPlayer = new AudioTrack(AudioManager.STREAM_MUSIC, m_sampleRate, m_channels,
                                           m_audioFormat, m_bufferSize, AudioTrack.MODE_STREAM);

        final AudioDeviceInfo[] devices = m_audioManager.getDevices(AudioManager.GET_DEVICES_ALL);
        for (AudioDeviceInfo deviceInfo : devices) {
            if (deviceInfo.getId() == outputId) {
                m_streamPlayer.setPreferredDevice(deviceInfo);
            } else if (deviceInfo.getId() == inputId) {
                m_recorder.setPreferredDevice(deviceInfo);
            }
        }

        m_recorder.startRecording();
        m_isStreaming = true;

        m_streamingThread = new Thread(new Runnable() {
            public void run() {
                streamSound();
            }
        });

        m_streamingThread.start();
    }

    private static void stopSoundStreaming()
    {
        if (!m_isStreaming)
            return;

        m_isStreaming = false;
        try {
            m_streamingThread.join();
            m_streamingThread = null;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        m_recorder.stop();
        m_recorder.release();
        m_streamPlayer.release();
        m_streamPlayer = null;
        m_recorder = null;
    }
}
