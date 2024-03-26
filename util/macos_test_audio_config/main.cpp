// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <iostream>
#include <vector>
#include <optional>
#include <unordered_map>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>

#include <CoreAudio/AudioHardware.h>

static constexpr std::uint32_t ElementMaster =
        kAudioObjectPropertyElementMaster; // TODO: use kAudioObjectPropertyElementMain for macOS
                                           // versions >= 12.0
static constexpr std::uint32_t LogOffsetSize = 2;

static constexpr std::chrono::milliseconds DefaultTestingTime(1000);
static constexpr std::chrono::milliseconds DefaultTestingInterval(0);

struct Flags
{
    std::uint32_t value = 0;
};

struct Code
{
    std::uint32_t code = 0;
};

template<typename T>
struct Span
{
    const T *data = nullptr;
    size_t size = 0;
};

struct LogOffset
{
    std::uint32_t value = 0;

    LogOffset operator+(std::uint32_t v) const { return LogOffset{ value + v }; }
};

namespace std {
ostream &operator<<(ostream &os, const Flags &flags)
{
    os << '[';
    std::uint32_t index = 0;
    bool first = true;
    for (auto value = flags.value; value; value >>= 1, ++index) {
        if ((value & 1) != 0) {
            if (!first)
                os << ',';
            os << index;
            first = false;
        }
    }
    os << ']';

    return os;
}

ostream &operator<<(ostream &os, const Code &code)
{
    os << '{' << code.code << '|';

    const char *desc = reinterpret_cast<const char *>(&code.code);

    std::copy_n(std::make_reverse_iterator(desc + sizeof(code.code)), sizeof(code.code),
                std::ostream_iterator<char>(os));

    os << '}';

    return os;
}

ostream &operator<<(ostream &os, const LogOffset &offset)
{
    std::fill_n(std::ostream_iterator<char>(os), offset.value * LogOffsetSize, ' ');

    return os;
}

template<typename T>
ostream &operator<<(ostream &os, const Span<T> &span)
{
    os << '[';
    if (span.size)
        os << span.data[0];

    for (size_t i = 1; i < span.size; ++i)
        os << ',' << span.data[i];

    os << ']';

    return os;
}

ostream &operator<<(ostream &os, const CFStringRef &str)
{
    const auto originalBuffer = CFStringGetCStringPtr(str, kCFStringEncodingUTF8);
    if (originalBuffer) {
        os << originalBuffer;
    } else {
        const auto lengthInUtf16 = CFStringGetLength(str);
        const auto maxLengthInUtf8 =
                CFStringGetMaximumSizeForEncoding(lengthInUtf16, kCFStringEncodingUTF8) + 1;
        std::vector<char> localBuffer(maxLengthInUtf8);

        if (CFStringGetCString(str, localBuffer.data(), maxLengthInUtf8, maxLengthInUtf8))
            os << localBuffer.data();
        else
            os << "{empty}";
    }

    return os;
}

} // namespace std

template<typename T = char>
static std::optional<std::vector<T>>
getAudioData(std::ostream &os, const LogOffset &offset, AudioObjectID inObjectID,
             const AudioObjectPropertyAddress &inAddress, size_t minDataSize = 0)
{
    static_assert(std::is_trivial_v<T>, "Trivial type is expected");

    UInt32 propSize = 0;
    const auto res = AudioObjectGetPropertyDataSize(inObjectID, &inAddress, 0, nullptr, &propSize);

    if (res == noErr) {
        if (propSize / sizeof(T) < minDataSize) {
            os << offset << "Data size is too low: actual " << propSize << "B vs expected " << minDataSize * sizeof(T) << "B\n";
            return {};
        }

        std::vector<T> data(propSize / sizeof(T));

        if (data.size() * sizeof(T) != propSize)
            os << offset << "Probably, wrong data size: " << propSize << ", Element size: " << sizeof(T) << '\n';

        const auto res = AudioObjectGetPropertyData(inObjectID, &inAddress, 0, nullptr, &propSize,
                                                    data.data());

        if (res == noErr)
            return { std::move(data) };
        else
            os << offset << "AudioObjectGetPropertyData failed, Err: "
               << Code{ static_cast<std::uint32_t>(res) } << '\n';
    } else {
        os << offset << "AudioObjectGetPropertyDataSize failed, Err: "
           << Code{ static_cast<std::uint32_t>(res) } << '\n';
    }

    return {};
}

template<typename T = char>
static std::optional<T> getAudioObject(std::ostream &os, const LogOffset &offset,
                                       AudioObjectID inObjectID,
                                       const AudioObjectPropertyAddress &inAddress)
{
    if (auto data = getAudioData<T>(os, offset, inObjectID, inAddress, 1)) {
        if (data->size() > 1)
            os << offset << "Warn: unexpected data size: " << data->size() << '\n';

        return { data->front() };
    }

    return {};
}

static void dumpFormats(std::ostream &os, const LogOffset &offset, AudioDeviceID id,
                        std::uint32_t scope)
{
    os << offset << "Formats:\n";

    const AudioObjectPropertyAddress audioDevicePropertyStreamsAddress{ kAudioDevicePropertyStreams,
                                                                        scope, ElementMaster };

    if (auto streamIDs = getAudioData<AudioStreamID>(os, offset + 1, id,
                                                     audioDevicePropertyStreamsAddress)) {
        for (auto &streamID : *streamIDs) {
            os << offset + 1 << "Stream id: " << streamID << '\n';

            auto dumpCurrentFormat = [&](std::uint32_t selector) {
                const AudioObjectPropertyAddress propertyAddress{ selector, scope, ElementMaster };

                if (auto format = getAudioObject<AudioStreamBasicDescription>(
                            os, offset + 3, streamID, propertyAddress)) {

                    os << offset + 3 << "mFormatID: " << format->mFormatID << '\n';
                    os << offset + 3 << "mSampleRate: " << format->mSampleRate << '\n';
                    os << offset + 3 << "mFormatFlags: " << Flags{ format->mFormatFlags } << '\n';
                }
            };

            auto dumpAvailableFormats = [&](std::uint32_t selector) {
                const AudioObjectPropertyAddress propertyAddress{ selector, scope, ElementMaster };

                if (auto descriptions = getAudioData<AudioStreamRangedDescription>(
                            os, offset + 3, streamID, propertyAddress)) {

                    size_t index = 0;
                    for (const AudioStreamRangedDescription &desc : *descriptions) {

                        const auto &format = desc.mFormat;
                        os << offset + 3 << "mFormatID: " << format.mFormatID << " (Index " << index
                           << ")\n";

                        os << offset + 4 << "mSampleRateRange: [" << desc.mSampleRateRange.mMinimum
                           << "; " << desc.mSampleRateRange.mMaximum << "]\n"
                           << offset + 4 << "mSampleRate: " << format.mSampleRate << '\n'
                           << offset + 4 << "mFormatFlags: " << Flags{ format.mFormatFlags } << '\n'
                           << offset + 4 << "mBytesPerPacket: " << format.mBytesPerPacket << '\n'
                           << offset + 4 << "mFramesPerPacket: " << format.mFramesPerPacket << '\n'
                           << offset + 4 << "mBytesPerFrame: " << format.mBytesPerFrame << '\n'
                           << offset + 4 << "mChannelsPerFrame: " << format.mChannelsPerFrame << '\n'
                           << offset + 4 << "mBitsPerChannel: " << format.mBitsPerChannel << '\n';

                        ++index;
                    }
                }
            };

            os << offset + 2 << "Preffered physical format "
               << Code{ kAudioStreamPropertyPhysicalFormat } << ":\n";
            dumpCurrentFormat(kAudioStreamPropertyPhysicalFormat);

            os << offset + 2 << "Available physical formats "
               << Code{ kAudioStreamPropertyAvailablePhysicalFormats } << ":\n";
            dumpAvailableFormats(kAudioStreamPropertyAvailablePhysicalFormats);

            os << offset + 2 << "Preffered virtual format "
               << Code{ kAudioStreamPropertyVirtualFormat } << ":\n";
            dumpCurrentFormat(kAudioStreamPropertyVirtualFormat);

            os << offset + 2 << "Available virtual formats "
               << Code{ kAudioStreamPropertyAvailableVirtualFormats } << ":\n";
            dumpAvailableFormats(kAudioStreamPropertyAvailableVirtualFormats);
        }
    }
}

static void dumpChannelsLayout(std::ostream &os, const LogOffset &offset, AudioDeviceID id,
                               std::uint32_t scope)
{
    os << offset << "Channels Layout " << Code{ kAudioDevicePropertyPreferredChannelLayout }
       << ":\n";

    const AudioObjectPropertyAddress audioDeviceChannelLayoutPropertyAddress{
        kAudioDevicePropertyPreferredChannelLayout, scope, ElementMaster
    };

    if (auto data = getAudioData(os, offset + 1, id, audioDeviceChannelLayoutPropertyAddress,
                                 sizeof(AudioChannelLayout))) {
        const AudioChannelLayout &layout = *reinterpret_cast<AudioChannelLayout *>(data->data());

        os << offset + 1 << "mChannelLayoutTag: " << layout.mChannelLayoutTag << "\n"
           << offset + 1 << "mChannelBitmap: " << Flags{ layout.mChannelBitmap } << "\n"
           << offset + 1 << "mNumberChannelDescriptions: " << layout.mNumberChannelDescriptions << "\n"
           << offset + 1 << "ChannelDescriptions:\n";

        for (UInt32 i = 0; i < layout.mNumberChannelDescriptions; ++i) {
            const auto &desc = layout.mChannelDescriptions[i];
            os << offset + 2 << "Channel " << i << ":\n";

            os << offset + 3 << "mChannelLabel: " << desc.mChannelLabel;
            if (desc.mChannelLabel == 0xFFFFFFFF)
                os << " (unknown)";
            os << '\n';

            os << offset + 3 << "mChannelFlags: " << Flags{ desc.mChannelFlags } << '\n'
               << offset + 3 << "mCoordinates: " << Span<Float32>{ desc.mCoordinates, 3 } << '\n';
        }
    }
}

static void dumpBasicDescription(std::ostream &os, const LogOffset &offset, AudioDeviceID id,
                                 std::uint32_t scope)
{
    os << offset << "Basic Description " << Code{ kAudioDevicePropertyStreamFormat } << ":\n";

    const AudioObjectPropertyAddress audioDeviceStreamFormatPropertyAddress{
        kAudioDevicePropertyStreamFormat, scope, ElementMaster
    };

    if (auto basicDescr = getAudioObject<AudioStreamBasicDescription>(
                os, offset + 1, id, audioDeviceStreamFormatPropertyAddress)) {
        os << offset + 1 << "mSampleRate: " << basicDescr->mSampleRate << "\n"
           << offset + 1 << "mFormatID: " << basicDescr->mFormatID << "\n"
           << offset + 1 << "mFormatFlags: " << Flags{ basicDescr->mFormatFlags } << "\n"
           << offset + 1 << "mBytesPerPacket: " << basicDescr->mBytesPerPacket << "\n"
           << offset + 1 << "mFramesPerPacket: " << basicDescr->mFramesPerPacket << "\n"
           << offset + 1 << "mBytesPerFrame: " << basicDescr->mBytesPerFrame << "\n"
           << offset + 1 << "mChannelsPerFrame: " << basicDescr->mChannelsPerFrame << "\n"
           << offset + 1 << "mBitsPerChannel: " << basicDescr->mBitsPerChannel << "\n";
    }
}

static void dumpGeneralDeviceInfo(std::ostream &os, const LogOffset &offset, AudioDeviceID id,
                                  std::uint32_t scope)
{
    os << offset << "General Device Info:\n";

    auto dumpString = [&](const char *name, std::uint32_t selector) {
        os << offset + 1 << name << " " << Code{ selector } << ":\n";
        const AudioObjectPropertyAddress propertyAddress{ selector, scope, ElementMaster };

        if (auto str = getAudioObject<CFStringRef>(os, offset + 2, id, propertyAddress)) {
            os << offset + 2 << *str << '\n';
            CFRelease(*str);
        }
    };

    auto dumpBool = [&](const char *name, std::uint32_t selector) {
        os << offset + 1 << name << ' ' << Code{ selector } << ":\n";
        const AudioObjectPropertyAddress propertyAddress{ selector, scope, ElementMaster };

        if (auto value = getAudioObject<UInt32>(os, offset + 2, id, propertyAddress))
            os << offset + 2 << std::boolalpha << static_cast<bool>(*value) << '\n';
    };

    dumpString("Name", kAudioObjectPropertyName);
    dumpString("Manufacturer", kAudioObjectPropertyManufacturer);
    dumpString("Element name", kAudioObjectPropertyElementName);
    dumpString("Model name", kAudioObjectPropertyModelName);

    dumpBool("Device is alive", kAudioDevicePropertyDeviceIsAlive);
    dumpBool("Device is running", kAudioDevicePropertyDeviceIsRunning);
    dumpBool("Can be default device", kAudioDevicePropertyDeviceCanBeDefaultDevice);
    dumpBool("Can be default system device", kAudioDevicePropertyDeviceCanBeDefaultSystemDevice);
    dumpBool("Device property is hidden", kAudioDevicePropertyIsHidden);

    {
        const AudioObjectPropertyAddress propertyAddress{
            kAudioDevicePropertyPreferredChannelsForStereo, scope, ElementMaster
        };

        os << offset + 1 << "Preffered channels for stereo "
           << Code{ kAudioDevicePropertyPreferredChannelsForStereo } << ":\n";
        if (auto data = getAudioData<UInt32>(os, offset + 2, id, propertyAddress, 2))
            os << offset + 2 << Span<UInt32>{ data->data(), 2 } << '\n';
    }
}

static void dumpAvailableAudioDevices(std::ostream &os, const LogOffset &offset,
                                      std::uint32_t scope)
{
    os << offset << "Dump devices " << Code{ kAudioHardwarePropertyDevices }
       << ", scope: " << Code{ scope } << "\n";
    const AudioObjectPropertyAddress audioDevicesPropertyAddress{ kAudioHardwarePropertyDevices,
                                                                  scope, ElementMaster };

    if (auto devices = getAudioData<AudioDeviceID>(os, offset + 1, kAudioObjectSystemObject,
                                                   audioDevicesPropertyAddress)) {
        size_t index = 0;
        for (auto id : *devices) {
            os << offset + 2 << "ID: " << id << " (Index " << index << ")\n";

            dumpGeneralDeviceInfo(os, offset + 3, id, scope);

            dumpBasicDescription(os, offset + 3, id, scope);

            dumpChannelsLayout(os, offset + 3, id, scope);

            dumpFormats(os, offset + 3, id, scope);

            ++index;
        }
    }
}

static void dumpAvailableAudioDevices(std::ostream &os, const LogOffset &offset)
{
    for (auto scope : { kAudioObjectPropertyScopeInput, kAudioObjectPropertyScopeOutput }) {
        dumpAvailableAudioDevices(os, offset, scope);
        os << offset << "\n";
    }
}

static void testStability(const std::chrono::milliseconds &time,
                          const std::chrono::milliseconds &interval)
{
    std::cout << "Start testing audio config...\n" << std::endl;

    std::unordered_map<std::string, std::uint32_t> results;
    std::uint32_t counter = 0;

    const auto end = std::chrono::system_clock::now() + time;

    while (true) {
        std::ostringstream stream;

        dumpAvailableAudioDevices(stream, {});

        ++counter;
        ++results[stream.str()];

        if (std::chrono::system_clock::now() + interval >= end)
            break;

        std::this_thread::sleep_for(interval);
    }

    std::cout << "Audio config has been tested for " << time.count() << "ms\n" <<
                 "Set --time %your time% in order to change the testing time.\n" <<
                 "The config has been taken " << counter << " times\n" <<
                 "------------------------------------------------------------\n";

    std::cout << std::endl;

    using Result = decltype(results)::value_type;
    std::vector<std::reference_wrapper<Result>> resultRefs(results.begin(), results.end());
    std::sort(resultRefs.begin(), resultRefs.end(),
              [](const Result &a, const Result &b) { return a.second < b.second; });

    for (size_t i = 0; i < resultRefs.size(); ++i) {
        const Result &res = resultRefs[i];
        std::cout << "Result N" << i + 1 << "; Occurs: " << res.second << '/' << counter << '\n'
                  << res.first;
    }

    if (results.size() == 1)
        std::cout << "The config seems to be stable (" << counter << " times)\n";
    else
        std::cout << "The config is unstable: " << results.size() << " different results\n";

    std::cout << "\nTesting done!" << std::endl;
}

static void printHelp()
{
    // clang-format off
    std::cout << "This utility tests stability of audio configuration on macOS, and prints it.\n"
                 "It reads audio config via AudioObjectGetPropertyData many times, checks if it changes and dumps results.\n"
                 "It might be useful to check the configuration if specific audio devices are installed on your PC\n"
                 "and Qt multimedia handles them incorrectly or writes warnings to the console.\n"
                 "It's a good idea to attach a text file with the utility's output when creating a bug for audio on macOS.\n"
                 "In order to test an unstable config, set some sufficient testing time and just attach/detach a device.\n"
                 "Options:\n"
                 "  --time Common time of testing in ms. Defaults to " << DefaultTestingTime.count() << ".\n"
                 "         Set --time 0 for simple duming of the audio config.\n"
                 "  --interval Interval in ms between reading of audio config. Defaults to " << DefaultTestingInterval.count() << ".\n"
                 "  --help Show help.";
    // clang-format on

    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    std::chrono::milliseconds testingTime = DefaultTestingTime;
    std::chrono::milliseconds testingInterval = DefaultTestingInterval;

    for (int i = 1; i < argc; ++i) {
        auto getIntValue = [&]() -> std::optional<int> {
            if (i + 1 < argc) {
                char *end = argv[i + 1];
                const auto val = strtol(argv[i + 1], &end, 10);
                ++i;
                if (*end == '\0')
                    return { val };
                else
                    std::cout << "Cannot read value " << argv[i + 1] << std::endl;
            } else {
                std::cout << "Cannot read value" << std::endl;
            }

            return {};
        };

        if (strcmp(argv[i], "--time") == 0) {
            if (auto time = getIntValue())
                testingTime = std::chrono::milliseconds(*time);
            else
                return 1;
        } else if (strcmp(argv[i], "--interval") == 0) {
            if (auto interval = getIntValue())
                testingInterval = std::chrono::milliseconds(*interval);
            else
                return 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printHelp();
            return 0;
        } else {
            std::cout << "Wrong option " << argv[i] << std::endl;
            return 1;
        }
    }

    testStability(testingTime, testingInterval);

    return 0;
}
