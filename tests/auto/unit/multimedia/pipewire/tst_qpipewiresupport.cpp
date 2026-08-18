// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/private/qpipewire_propertydict_p.h>
#include <QtMultimedia/private/qpipewire_registry_support_p.h>
#include <QtMultimedia/private/qpipewire_spa_pod_parser_support_p.h>
#include <QtMultimedia/private/qpipewire_spa_pod_support_p.h>
#include <QtMultimedia/private/qpipewire_support_p.h>
#include <QtMultimedia/qaudioformat.h>

#include <pipewire/extensions/metadata.h>
#include <pipewire/extensions/profiler.h>
#if __has_include(<pipewire/extensions/security-context.h>)
#  include <pipewire/extensions/security-context.h>
#endif

#include <spa/param/format.h>
#if __has_include(<spa/param/audio/raw-utils.h>)
#  include <spa/param/audio/raw-utils.h>
#else
#  include <QtMultimedia/private/qpipewire_spa_compat_p.h>
#endif

#include <cerrno>

#ifndef PW_KEY_DEVICE_SYSFS_PATH
#  define PW_KEY_DEVICE_SYSFS_PATH "device.sysfs.path"
#endif
#ifndef PW_KEY_OBJECT_SERIAL
#  define PW_KEY_OBJECT_SERIAL "object.serial"
#endif

QT_USE_NAMESPACE
using namespace QtPipeWire;

// NOLINTBEGIN(readability-convert-member-functions-to-static)

namespace {

struct PodBuffer
{
    PodBuffer() { builder = SPA_POD_BUILDER_INIT(storage.data(), uint32_t(storage.size())); }

    std::array<uint8_t, 4096> storage;
    spa_pod_builder builder{};
};

} // namespace

class tst_QPipeWireSupport : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // qpipewire_support_p.h
    void strongIdType();
    void errorCode();

    // qpipewire_registry_support_p.h
    void registryType_data();
    void registryType();
    void registryType_unknown();

    // qpipewire_propertydict_p.h
    void toPropertyDict();
    void propertyGetters();
    void propertyGetters_missingKeys();
    void deviceId_invalidNumber();
    void makeProperties_roundtrip();

    // qpipewire_spa_pod_parser_support_p.h
    void parsePropertyScalar();
    void parsePropertyChoice();
    void spaRange_wrongElementCount();
    void spaRange_wrongElementSize();
    void spaEnum_wrongElementSize();

    // qpipewire_spa_pod_support_p.h
    void asSpaAudioInfoRaw_data();
    void asSpaAudioInfoRaw();
    void spaObjectAudioFormat_roundtrip();
    void spaObjectAudioFormat_wrongType();
};

void tst_QPipeWireSupport::initTestCase()
{
    pw_init(nullptr, nullptr);
}

void tst_QPipeWireSupport::cleanupTestCase()
{
    pw_deinit();
}

void tst_QPipeWireSupport::strongIdType()
{
    ObjectId a{ 1 };
    ObjectId b{ 1 };
    ObjectId c{ 2 };

    QCOMPARE(a, b);
    QVERIFY(a != c);
    QVERIFY(a < c);

    ObjectSerial s1{ 1ull };
    ObjectSerial s2{ 2ull };
    QVERIFY(s1 != s2);
    QVERIFY(s1 < s2);

    QString debugText;
    QDebug(&debugText) << a;
    QVERIFY(!debugText.isEmpty());
}

void tst_QPipeWireSupport::errorCode()
{
    std::error_code ec = make_error_code(EINVAL);
    QCOMPARE(ec.value(), EINVAL);
    QCOMPARE(ec.category(), std::generic_category());
    QVERIFY(!ec.message().empty());

    errno = EACCES;
    std::error_code ecDefault = make_error_code();
    QCOMPARE(ecDefault.value(), EACCES);
}

void tst_QPipeWireSupport::registryType_data()
{
    QTest::addColumn<QByteArray>("typeName");
    QTest::addColumn<PipewireRegistryType>("expected");

    QTest::newRow("Client") << QByteArray(PW_TYPE_INTERFACE_Client) << PipewireRegistryType::Client;
    QTest::newRow("Core") << QByteArray(PW_TYPE_INTERFACE_Core) << PipewireRegistryType::Core;
    QTest::newRow("Device") << QByteArray(PW_TYPE_INTERFACE_Device) << PipewireRegistryType::Device;
    QTest::newRow("Factory") << QByteArray(PW_TYPE_INTERFACE_Factory) << PipewireRegistryType::Factory;
    QTest::newRow("Link") << QByteArray(PW_TYPE_INTERFACE_Link) << PipewireRegistryType::Link;
    QTest::newRow("Metadata") << QByteArray(PW_TYPE_INTERFACE_Metadata) << PipewireRegistryType::Metadata;
    QTest::newRow("Module") << QByteArray(PW_TYPE_INTERFACE_Module) << PipewireRegistryType::Module;
    QTest::newRow("Node") << QByteArray(PW_TYPE_INTERFACE_Node) << PipewireRegistryType::Node;
    QTest::newRow("Port") << QByteArray(PW_TYPE_INTERFACE_Port) << PipewireRegistryType::Port;
    QTest::newRow("Profiler") << QByteArray(PW_TYPE_INTERFACE_Profiler) << PipewireRegistryType::Profiler;
    QTest::newRow("Registry") << QByteArray(PW_TYPE_INTERFACE_Registry) << PipewireRegistryType::Registry;
#if defined(PW_TYPE_INTERFACE_SecurityContext)
    QTest::newRow("SecurityContext") << QByteArray(PW_TYPE_INTERFACE_SecurityContext)
                                      << PipewireRegistryType::SecurityContext;
#endif
}

void tst_QPipeWireSupport::registryType()
{
    QFETCH(QByteArray, typeName);
    QFETCH(PipewireRegistryType, expected);

    std::optional<PipewireRegistryType> result =
            parsePipewireRegistryType(std::string_view(typeName.constData(), size_t(typeName.size())));
    QVERIFY(result.has_value());
    QCOMPARE(*result, expected);
}

void tst_QPipeWireSupport::registryType_unknown()
{
    QTest::ignoreMessage(QtWarningMsg, "unknown type \"not.a.pipewire.type\"");
    std::optional<PipewireRegistryType> result = parsePipewireRegistryType("not.a.pipewire.type");
    QVERIFY(!result.has_value());
}

void tst_QPipeWireSupport::toPropertyDict()
{
    std::array items{
        spa_dict_item{ "key.one", "value.one" },
        spa_dict_item{ "key.two", "value.two" },
    };
    spa_dict dict = SPA_DICT_INIT(items.data(), uint32_t(items.size()));

    PwPropertyDict result = QtPipeWire::toPropertyDict(dict);
    QCOMPARE(result.size(), size_t(2));
    QCOMPARE(result.at("key.one"), "value.one");
    QCOMPARE(result.at("key.two"), "value.two");
}

void tst_QPipeWireSupport::propertyGetters()
{
    std::array items{
        spa_dict_item{ PW_KEY_MEDIA_CLASS, "Audio/Sink" },
        spa_dict_item{ PW_KEY_NODE_NAME, "my-node" },
        spa_dict_item{ PW_KEY_NODE_DESCRIPTION, "My Node" },
        spa_dict_item{ PW_KEY_DEVICE_SYSFS_PATH, "/sys/devices/foo" },
        spa_dict_item{ PW_KEY_DEVICE_NAME, "my-device" },
        spa_dict_item{ PW_KEY_DEVICE_DESCRIPTION, "My Device" },
        spa_dict_item{ PW_KEY_METADATA_NAME, "default" },
        spa_dict_item{ PW_KEY_DEVICE_ID, "42" },
        spa_dict_item{ PW_KEY_OBJECT_SERIAL, "1234567890123" },
    };
    spa_dict dict = SPA_DICT_INIT(items.data(), uint32_t(items.size()));
    PwPropertyDict propertyDict = QtPipeWire::toPropertyDict(dict);

    QCOMPARE(getMediaClass(propertyDict), std::string_view("Audio/Sink"));
    QCOMPARE(getNodeName(propertyDict), std::string_view("my-node"));
    QCOMPARE(getNodeDescription(propertyDict), std::string_view("My Node"));
    QCOMPARE(getDeviceSysfsPath(propertyDict), std::string_view("/sys/devices/foo"));
    QCOMPARE(getDeviceName(propertyDict), std::string_view("my-device"));
    QCOMPARE(getDeviceDescription(propertyDict), std::string_view("My Device"));
    QCOMPARE(getMetadataName(propertyDict), std::string_view("default"));

    std::optional<ObjectId> deviceId = getDeviceId(propertyDict);
    QVERIFY(deviceId.has_value());
    QCOMPARE(deviceId->value, uint32_t(42));

    std::optional<ObjectSerial> serial = getObjectSerial(propertyDict);
    QVERIFY(serial.has_value());
    QCOMPARE(serial->value, uint64_t(1234567890123ull));
}

void tst_QPipeWireSupport::propertyGetters_missingKeys()
{
    PwPropertyDict empty;

    QVERIFY(!getMediaClass(empty).has_value());
    QVERIFY(!getNodeName(empty).has_value());
    QVERIFY(!getNodeDescription(empty).has_value());
    QVERIFY(!getDeviceSysfsPath(empty).has_value());
    QVERIFY(!getDeviceName(empty).has_value());
    QVERIFY(!getDeviceDescription(empty).has_value());
    QVERIFY(!getMetadataName(empty).has_value());
    QVERIFY(!getDeviceId(empty).has_value());
    QVERIFY(!getObjectSerial(empty).has_value());
}

void tst_QPipeWireSupport::deviceId_invalidNumber()
{
    std::array items{
        spa_dict_item{ PW_KEY_DEVICE_ID, "not-a-number" },
    };
    spa_dict dict = SPA_DICT_INIT(items.data(), uint32_t(items.size()));
    PwPropertyDict propertyDict = QtPipeWire::toPropertyDict(dict);

    QVERIFY(!getDeviceId(propertyDict).has_value());
}

void tst_QPipeWireSupport::makeProperties_roundtrip()
{
    std::array items{
        spa_dict_item{ PW_KEY_NODE_NAME, "roundtrip-node" },
        spa_dict_item{ PW_KEY_MEDIA_CLASS, "Audio/Source" },
    };

    PwPropertiesHandle handle = makeProperties(items);
    QVERIFY(handle);

    PwPropertyDict result = QtPipeWire::toPropertyDict(handle->dict);
    QCOMPARE(getNodeName(result), std::string_view("roundtrip-node"));
    QCOMPARE(getMediaClass(result), std::string_view("Audio/Source"));
}

void tst_QPipeWireSupport::parsePropertyScalar()
{
    PodBuffer pod;
    const spa_pod *object = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pod.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
            SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_audio),
            SPA_FORMAT_AUDIO_rate, SPA_POD_Int(44100)));
    QVERIFY(object);

    std::optional<int> rate =
            spaParsePodPropertyScalar<int>(*object, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_rate);
    QVERIFY(rate.has_value());
    QCOMPARE(*rate, 44100);

    std::optional<int> missing = spaParsePodPropertyScalar<int>(*object, SPA_TYPE_OBJECT_Format,
                                                                SPA_FORMAT_AUDIO_channels);
    QVERIFY(!missing.has_value());
}

void tst_QPipeWireSupport::parsePropertyChoice()
{
    PodBuffer pod;
    const spa_pod *object = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pod.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
            SPA_FORMAT_AUDIO_rate, SPA_POD_CHOICE_RANGE_Int(44100, 8000, 192000),
            SPA_FORMAT_AUDIO_channels, SPA_POD_CHOICE_ENUM_Int(3, 2, 1, 6)));
    QVERIFY(object);

    auto rateChoice = spaParsePodPropertyChoice<int, SPA_CHOICE_Range>(*object, SPA_TYPE_OBJECT_Format,
                                                                       SPA_FORMAT_AUDIO_rate);
    QVERIFY(rateChoice.has_value());
    QCOMPARE(rateChoice->defaultValue, 44100);
    QCOMPARE(rateChoice->minValue, 8000);
    QCOMPARE(rateChoice->maxValue, 192000);

    auto channelChoice = spaParsePodPropertyChoice<int, SPA_CHOICE_Enum>(
            *object, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_channels);
    QVERIFY(channelChoice.has_value());
    QCOMPARE(channelChoice->defaultValue(), 2);
    QCOMPARE(channelChoice->values().size(), 2u);
    QCOMPARE(channelChoice->values()[0], 1);
    QCOMPARE(channelChoice->values()[1], 6);
}

void tst_QPipeWireSupport::spaRange_wrongElementCount()
{
    PodBuffer pod;
    spa_pod_frame objectFrame;
    spa_pod_frame choiceFrame;

    spa_pod_builder_push_object(&pod.builder, &objectFrame, SPA_TYPE_OBJECT_Format,
                                SPA_PARAM_EnumFormat);
    spa_pod_builder_prop(&pod.builder, SPA_FORMAT_AUDIO_rate, 0);
    spa_pod_builder_push_choice(&pod.builder, &choiceFrame, SPA_CHOICE_Range, 0);
    spa_pod_builder_int(&pod.builder, 44100);
    spa_pod_builder_int(&pod.builder, 8000); // only 2 values instead of the required 3
    spa_pod_builder_pop(&pod.builder, &choiceFrame);
    const spa_pod *object =
            static_cast<const spa_pod *>(spa_pod_builder_pop(&pod.builder, &objectFrame));
    QVERIFY(object);

    auto rateChoice = spaParsePodPropertyChoice<int, SPA_CHOICE_Range>(*object, SPA_TYPE_OBJECT_Format,
                                                                       SPA_FORMAT_AUDIO_rate);
    QVERIFY(!rateChoice.has_value());
}

void tst_QPipeWireSupport::spaRange_wrongElementSize()
{
    PodBuffer pod;
    const spa_pod *object = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pod.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
            SPA_FORMAT_AUDIO_rate,
            SPA_POD_CHOICE_RANGE_Long(int64_t(44100), int64_t(8000), int64_t(192000))));
    QVERIFY(object);

    // the choice holds 8-byte "long" elements; parsing it as a 4-byte int range must fail
    // rather than reading past the end of the pod
    auto rateChoice = spaParsePodPropertyChoice<int, SPA_CHOICE_Range>(*object, SPA_TYPE_OBJECT_Format,
                                                                       SPA_FORMAT_AUDIO_rate);
    QVERIFY(!rateChoice.has_value());
}

void tst_QPipeWireSupport::spaEnum_wrongElementSize()
{
    PodBuffer pod;
    const spa_pod *object = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pod.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
            SPA_FORMAT_AUDIO_channels, SPA_POD_CHOICE_ENUM_Long(2, int64_t(2), int64_t(6))));
    QVERIFY(object);

    auto channelChoice = spaParsePodPropertyChoice<int, SPA_CHOICE_Enum>(
            *object, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_channels);
    QVERIFY(!channelChoice.has_value());
}

void tst_QPipeWireSupport::asSpaAudioInfoRaw_data()
{
    QTest::addColumn<QAudioFormat::ChannelConfig>("channelConfig");
    QTest::addColumn<int>("channelCount");
    QTest::addColumn<QList<int>>("expectedPositions");

    auto toList = [](auto span) {
        QList<int> result;
        for (auto value : span)
            result.push_back(int(value));
        return result;
    };

    QTest::newRow("mono") << QAudioFormat::ChannelConfigMono << 1
                          << toList(QtPipeWire::channelPositionsMono);
    QTest::newRow("stereo") << QAudioFormat::ChannelConfigStereo << 2
                            << toList(QtPipeWire::channelPositionsStereo);
    QTest::newRow("5.1") << QAudioFormat::ChannelConfigSurround5Dot1 << 6
                         << toList(QtPipeWire::channelPositions5Dot1);
    QTest::newRow("7.1") << QAudioFormat::ChannelConfigSurround7Dot1 << 8
                         << toList(QtPipeWire::channelPositions7Dot1);
}

void tst_QPipeWireSupport::asSpaAudioInfoRaw()
{
    QFETCH(QAudioFormat::ChannelConfig, channelConfig);
    QFETCH(int, channelCount);
    QFETCH(QList<int>, expectedPositions);

    QAudioFormat fmt;
    fmt.setSampleFormat(QAudioFormat::Int16);
    fmt.setSampleRate(48000);
    // setChannelConfig() derives the channel count from the config; setting an explicit
    // channel count afterwards would reset the config back to ChannelConfigUnknown
    fmt.setChannelConfig(channelConfig);
    QCOMPARE(fmt.channelCount(), channelCount);

    spa_audio_info_raw info = QtPipeWire::asSpaAudioInfoRaw(fmt);

    QCOMPARE(info.format, SPA_AUDIO_FORMAT_S16);
    QCOMPARE(int(info.rate), 48000);
    QCOMPARE(int(info.channels), channelCount);

    for (int i = 0; i < channelCount; ++i)
        QCOMPARE(int(info.position[i]), expectedPositions[i]);
}

void tst_QPipeWireSupport::spaObjectAudioFormat_roundtrip()
{
    QAudioFormat fmt;
    fmt.setSampleFormat(QAudioFormat::Int16);
    fmt.setSampleRate(48000);
    fmt.setChannelConfig(QAudioFormat::ChannelConfigStereo);

    spa_audio_info_raw info = QtPipeWire::asSpaAudioInfoRaw(fmt);

    PodBuffer pod;
    const spa_pod *builtPod =
            spa_format_audio_raw_build(&pod.builder, SPA_PARAM_EnumFormat, &info);
    QVERIFY(builtPod);

    std::optional<SpaObjectAudioFormat> parsed = SpaObjectAudioFormat::parse(builtPod);
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->channelCount, 2);
    QVERIFY(std::holds_alternative<spa_audio_format>(parsed->sampleTypes));
    QCOMPARE(std::get<spa_audio_format>(parsed->sampleTypes), SPA_AUDIO_FORMAT_S16);

    QVERIFY(parsed->rates.has_value());
    QVERIFY(std::holds_alternative<int>(*parsed->rates));
    QCOMPARE(std::get<int>(*parsed->rates), 48000);

    QVERIFY(parsed->channelPositions.has_value());
    QCOMPARE(parsed->channelPositions->size(), 2);
    QCOMPARE((*parsed->channelPositions)[0], SPA_AUDIO_CHANNEL_FL);
    QCOMPARE((*parsed->channelPositions)[1], SPA_AUDIO_CHANNEL_FR);
}

void tst_QPipeWireSupport::spaObjectAudioFormat_wrongType()
{
    PodBuffer pod;
    const spa_pod *object = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pod.builder, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
            SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_audio)));
    QVERIFY(object);

    std::optional<SpaObjectAudioFormat> parsed = SpaObjectAudioFormat::parse(object);
    QVERIFY(!parsed.has_value());
}

QTEST_APPLESS_MAIN(tst_QPipeWireSupport)

#include "tst_qpipewiresupport.moc"

// NOLINTEND(readability-convert-member-functions-to-static)
