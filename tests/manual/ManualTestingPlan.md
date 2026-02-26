# QtMultimedia: Manual testing plan

Some parts of QtMultimedia, most notably tests that need external hardware, cannot easily be automated. This page documents a list of manual tests to be performed manually, ideally on every supported platform.

## Audio subsystem

To test the audio subsystem, ideally some external audio devices are utilized: This may include (a) USB webcams, (b) USB audio devices / headsets and (c) Bluetooth headset. Ideally these tests should be done on a debug build of Qt (or at least a build which has assertions enabled)

### Audio device enumeration and hotplugging

Run the [Audio Devices](https://doc.qt.io/qt-6/qtmultimedia-audiodevices-example.html) example:

#### Device enumeration
**Steps to perform:**
* Select each input device
* Select each output device

**Things to look out for:**
* Entries in the properties should be “reasonable”

#### Device hotplugging
**Steps to perform:**
* Start application
* Insert external audio device(s)
* Remove external audio device(s)

**Things to look out for:**
* The “device” list should update
* If the currently selected device is removed, the “default” device should be selected
* The “default” device should be labeled as “(default)”

---

### Audio Sink

Run the [Audio Output](https://doc.qt.io/qt-6/qtmultimedia-audiooutput-example.html) example:

#### Test audio output
**Steps to perform:**
* Select each output device

**Things to look out for:**
* A clean sine wave should be played on the selected device

#### Test QAudioSink modes and formats
**Steps to perform:**
* Select every “mode” (Push/Pull/Callback)
* Select every sample format (with a fixed sampling rate / channel count)
* Select every sampling rate (with a fixed sample format / channel count)
* Select every channel count (with a fixed sampling rate / sample format)

**Things to look out for:**
* A clean sine wave should be played on the selected device
* On Stereo devices, playing a single-channel stream should play on both channels
* No perceived distortions (except for Uint8)

#### Test volume
**Steps to perform:**
* While sound is played to a specific device, change the “volume” slider
* Repeat for every “mode” (Push/Pull/Callback)

**Things to look out for:**
* The volume change should be audible

#### Test suspend/resume
**Steps to perform:**
* While sound is played to a specific device, “pause” and “resume”
* Repeat for every “mode” (Push/Pull/Callback)

**Things to look out for:**
* “Suspend” should stop the sine wave playback

#### Test device removal
**Steps to perform:**
* While sound is played to a specific device, physically remove the device.
* Repeat for every “mode” (Push/Pull/Callback)

**Things to look out for:**
* Playback continues on the “default” device
* A warning popup should be shown when the device is removed

---

### Audio Source

Run the [Audio Source](https://doc.qt.io/qt-6/qtmultimedia-audiosource-example.html) example:

#### Test audio input signal
**Steps to perform:**
* Select each input device
* Provide sound to the corresponding audio input

**Things to look out for:**
* Visualizer shows changes in volume

#### Test QAudioSource modes and formats
**Steps to perform:**
* Select every "mode" (Push/Pull/Callback)
* Select every sample format (with a fixed sampling rate / channel count)
* Select every sampling rate (with a fixed sample format / channel count)
* Select every channel count (with a fixed sampling rate / sample format)
* For every format, also record an audio file while providing sound to the audio input

**Things to look out for:**
* Visualizer shows signal amplitude changes
* Recordings have correct format
* Recordings sound correct and have no distortions

#### Test volume
**Steps to perform:**
* While providing sound to a specific device, change the "volume" slider
* Repeat for every “mode” (Push/Pull/Callback)

**Things to look out for:**
* The visualizer shows the volume change

#### Test suspend/resume
**Steps to perform:**
* While providing sound to a specific device, "suspend" and "resume"
* Repeat for every “mode” (Push/Pull/Callback)

**Things to look out for:**
* The visualizer freezes when suspended, and resumes in sync when resumed

#### Test device removal
**Steps to perform:**
* While providing sound to a specific device, physically remove the device
