// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import android.content.Context;
import android.media.AudioDeviceCallback;
import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.Range;

import org.qtproject.qt.android.UsedFromNativeCode;

class QtAudioDeviceManager
{
    private static final String TAG = "QtAudioDeviceManager";

    static private AudioManager m_audioManager = null;
    static private final AudioDevicesReceiver m_audioDevicesReceiver = new AudioDevicesReceiver();
    static private Handler handler = new Handler(Looper.getMainLooper());
    static private AudioRecord m_recorder = null;
    static private AudioTrack m_streamPlayer = null;
    static private Thread m_streamingThread = null;
    static private boolean m_isStreaming = false;
    static private boolean m_useSpeaker = false;
    static private final int m_sampleRate = 8000;
    static private final int m_channels = AudioFormat.CHANNEL_CONFIGURATION_MONO;
    static private final int m_audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    static private final int m_bufferSize = AudioRecord.getMinBufferSize(m_sampleRate, m_channels, m_audioFormat);
    static private int m_currentOutputId = -1;

    static native void onAudioInputDevicesUpdated();
    static native void onAudioOutputDevicesUpdated();

    static private void updateDeviceList() {
        if (m_currentOutputId != -1)
            setAudioOutput(m_currentOutputId);
        onAudioInputDevicesUpdated();
        onAudioOutputDevicesUpdated();
    }

    private static class AudioDevicesReceiver extends AudioDeviceCallback {
        @Override
        public void onAudioDevicesAdded(AudioDeviceInfo[] addedDevices) {
            updateDeviceList();
        }

        @Override
        public void onAudioDevicesRemoved(AudioDeviceInfo[] removedDevices) {
            updateDeviceList();
        }
    }


    static void registerAudioHeadsetStateReceiver()
    {
        m_audioManager.registerAudioDeviceCallback(m_audioDevicesReceiver, handler);
    }

    static void unregisterAudioHeadsetStateReceiver()
    {
        m_audioManager.unregisterAudioDeviceCallback(m_audioDevicesReceiver);
    }

    static void setContext(Context context)
    {
        m_audioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
    }

    static AudioDeviceInfo[] getAudioOutputDevices()
    {
        return getAudioDevices(AudioManager.GET_DEVICES_OUTPUTS);
    }

    static AudioDeviceInfo[] getAudioInputDevices()
    {
        return getAudioDevices(AudioManager.GET_DEVICES_INPUTS);
    }

    @UsedFromNativeCode
    static int getDefaultSampleRate()
    {
        String sampleRate = m_audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        return Integer.parseInt(sampleRate);
    }

    @UsedFromNativeCode
    static boolean isBluetoothDevice(AudioDeviceInfo deviceInfo)
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
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
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
        // API <= 23 types
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
        }

        // API 24
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            if (type == AudioDeviceInfo.TYPE_BUS)
                return "Bus";
        }

        return "Unknown-Type";

    }

    private static final HashMap<Integer, Integer> priorityMap = new HashMap<Integer, Integer>() {{
        put(AudioDeviceInfo.TYPE_WIRED_HEADSET, 1);
        put(AudioDeviceInfo.TYPE_WIRED_HEADPHONES, 1);
        put(AudioDeviceInfo.TYPE_BLUETOOTH_A2DP, 2);
        put(AudioDeviceInfo.TYPE_BLUETOOTH_SCO, 2);
        put(AudioDeviceInfo.TYPE_BUILTIN_SPEAKER, 3);
    }};
    private static final int DEFAULT_PRIORITY = 4;

    private static void sortAudioDevices(AudioDeviceInfo[] devices) {
        Comparator<AudioDeviceInfo> deviceTypeComparator = new Comparator<AudioDeviceInfo>() {
            @Override
            public int compare(AudioDeviceInfo device1, AudioDeviceInfo device2) {
                return getPriority(device1) - getPriority(device2);
            }

            private int getPriority(AudioDeviceInfo device) {
                return priorityMap.getOrDefault(device.getType(), DEFAULT_PRIORITY);
            }
        };

       Arrays.sort(devices, deviceTypeComparator);
    }

    private static AudioDeviceInfo[] getAudioDevices(int type)
    {
        ArrayList<AudioDeviceInfo> filteredDevices = new ArrayList<>();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            boolean builtInMicAdded = false;
            boolean bluetoothDeviceAdded = false;
            AudioDeviceInfo[] audioDevices = m_audioManager.getDevices(type);
            sortAudioDevices(audioDevices);

            for (AudioDeviceInfo deviceInfo : audioDevices) {
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

                filteredDevices.add(deviceInfo);
            }
        }

        return filteredDevices.toArray(new AudioDeviceInfo[filteredDevices.size()]);
    }

    final private static int [] bluetoothTypes = {
            AudioDeviceInfo.TYPE_BLUETOOTH_A2DP,
            AudioDeviceInfo.TYPE_BLUETOOTH_SCO
    };
    final private static int [] wiredTypes = {
            AudioDeviceInfo.TYPE_WIRED_HEADSET,
            AudioDeviceInfo.TYPE_WIRED_HEADPHONES
    };

    private static boolean containsAnyOfType(AudioDeviceInfo[] devices, int[]... types) {

    return Arrays.stream(devices)
                 .anyMatch(device -> Arrays.stream(types)
                                           .flatMapToInt(Arrays::stream)
                                           .anyMatch(type -> device.getType() == type));
    }

    private static int getCorrectModeIfContainsAnyOfType(AudioDeviceInfo[] devices, int[]... types) {
        return containsAnyOfType(devices, types) ?
                AudioManager.MODE_IN_COMMUNICATION : AudioManager.MODE_NORMAL;
    }

    private static int getModeForWired(AudioDeviceInfo[] audioDevices)
    {
        return getCorrectModeIfContainsAnyOfType(audioDevices, bluetoothTypes);
    }

    private static int getModeForBluotooth(AudioDeviceInfo[] audioDevices)
    {
        return getCorrectModeIfContainsAnyOfType(audioDevices, wiredTypes);
    }

    private static int getModeForSpeaker(AudioDeviceInfo[] audioDevices)
    {
        return getCorrectModeIfContainsAnyOfType(audioDevices, bluetoothTypes, wiredTypes);
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
                       setAudioOutput(deviceInfo, getModeForBluotooth(audioDevices), true, false);
                       return true;
                   case AudioDeviceInfo.TYPE_BUILTIN_SPEAKER:
                       setAudioOutput(deviceInfo, getModeForSpeaker(audioDevices), false, true);
                       return true;
                   case AudioDeviceInfo.TYPE_WIRED_HEADSET:
                   case AudioDeviceInfo.TYPE_WIRED_HEADPHONES:
                       setAudioOutput(deviceInfo, getModeForWired(audioDevices), false, false);
                       return true;
                   case AudioDeviceInfo.TYPE_BUILTIN_EARPIECE:
                       // It doesn't work when WIRED HEADPHONES are connected
                       // Earpiece has the lowest priority and setWiredHeadsetOn(boolean)
                       // method to force it is deprecated
                       Log.w(TAG, "Built in Earpiece may not work when "
                             + "Wired Headphones are connected");
                       setAudioOutput(deviceInfo, AudioManager.MODE_IN_CALL, false, false);
                       return true;
                   case AudioDeviceInfo.TYPE_HDMI:
                   case AudioDeviceInfo.TYPE_HDMI_ARC:
                   case AudioDeviceInfo.TYPE_HDMI_EARC:
                       setAudioOutput(deviceInfo, AudioManager.MODE_NORMAL, false, false);
                       return true;
                   default:
                       return false;
               }
           }
        }
        return false;
    }

    private static void setAudioOutput(AudioDeviceInfo deviceInfo, int mode, boolean bluetoothOn,
                                       boolean speakerOn)
    {
        m_audioManager.setMode(mode);

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            if (bluetoothOn)
                m_audioManager.startBluetoothSco();
            else
                m_audioManager.stopBluetoothSco();
            m_audioManager.setSpeakerphoneOn(speakerOn);

        } else if (mode == AudioManager.MODE_IN_COMMUNICATION) {
            m_audioManager.setCommunicationDevice(deviceInfo);
        }

        m_audioManager.setBluetoothScoOn(bluetoothOn);

        m_currentOutputId = deviceInfo.getId();
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
            @Override
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
