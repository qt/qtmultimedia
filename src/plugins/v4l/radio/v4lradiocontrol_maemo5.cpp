/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "v4lradiocontrol_maemo5.h"
#include "v4lradioservice.h"

#include "linux/videodev2.h"
#include <linux/videodev.h>
#include <sys/soundcard.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define HEADPHONE_STATE_FILE "/sys/devices/platform/gpio-switch/headphone/state"
#define HEADPHONE_CONNECTED_STATE "connected"
#define HEADPHONE_DISCONNECTED_STATE "disconnected"

#define FMRXENABLER_DBUS_SERVICE "de.pycage.FMRXEnabler"
#define FMRXENABLER_DBUS_OBJ_PATH "/de/pycage/FMRXEnabler"
#define FMRXENABLER_DBUS_IFACE_NAME "de.pycage.FMRXEnabler"

gboolean
state_file_changed(GIOChannel* source, GIOCondition /*condition*/, gpointer data)
{
    V4LRadioControl* radioControl = (V4LRadioControl*)data;
    gchar* result;

    g_io_channel_seek_position(source, 0, G_SEEK_SET, NULL);
    g_io_channel_read_line(source, &result, NULL, NULL, NULL);
    g_strstrip(result);

    if (g_ascii_strcasecmp(result, HEADPHONE_DISCONNECTED_STATE) == 0) {
        radioControl->enablePipeline(false);
    } else if (g_ascii_strcasecmp(result, HEADPHONE_CONNECTED_STATE) == 0) {
        // Wait 400ms until audio is routed again to headphone to prevent sound coming from speakers
        QTimer::singleShot(400,radioControl,SLOT(enablePipeline()));
    }

#ifdef MULTIMEDIA_MAEMO_DEBUG
    qDebug() << "Headphone is now" << result;
#endif

    g_free(result);
    return true;
}

V4LRadioControl::V4LRadioControl(QObject *parent)
    : QRadioTunerControl(parent)
    , fd(1)
    , m_error(false)
    , muted(false)
    , stereo(false)
    , step(100000)
    , sig(0)
    , scanning(false)
    , currentBand(QRadioTuner::FM)
    , pipeline(0)
{
    if (QDBusConnection::systemBus().isConnected()) {
        FMRXEnablerIFace = new QDBusInterface(FMRXENABLER_DBUS_SERVICE,
                                              FMRXENABLER_DBUS_OBJ_PATH,
                                              FMRXENABLER_DBUS_IFACE_NAME,
                                              QDBusConnection::systemBus());
    }

    createGstPipeline();

    GIOChannel* headphoneStateFile = NULL;
    headphoneStateFile = g_io_channel_new_file(HEADPHONE_STATE_FILE, "r", NULL);
    if (headphoneStateFile != NULL) {
        g_io_add_watch(headphoneStateFile, G_IO_PRI, state_file_changed, this);
    } else {
#ifdef MULTIMEDIA_MAEMO_DEBUG
        qWarning() << QString("File %1 can't be read!").arg(HEADPHONE_STATE_FILE) ;
        qWarning() << "Monitoring headphone state isn't possible!";
#endif
    }

    enableFMRX();
    initRadio();
    setupHeadPhone();

    setMuted(false);

    timer = new QTimer(this);
    timer->setInterval(200);
    connect(timer,SIGNAL(timeout()),this,SLOT(search()));

    tickTimer = new QTimer(this);
    tickTimer->setInterval(10000);
    connect(tickTimer,SIGNAL(timeout()),this,SLOT(enableFMRX()));
    tickTimer->start();
}

V4LRadioControl::~V4LRadioControl()
{
    if (pipeline)
    {
        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref (GST_OBJECT (pipeline));
    }

    if(fd > 0)
        ::close(fd);
}

void V4LRadioControl::enablePipeline(bool enable)
{
    if (enable == true)
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
    else
        gst_element_set_state (pipeline, GST_STATE_NULL);
}

void V4LRadioControl::enableFMRX()
{
    if (FMRXEnablerIFace && FMRXEnablerIFace->isValid()) {
        FMRXEnablerIFace->call("request"); // Return value ignored
    }
}

// Workaround to capture sound from the PGA line and play it back using gstreamer
// because N900 doesn't output sound directly to speakers
bool V4LRadioControl::createGstPipeline()
{
    GstElement *source, *sink;

    gst_init (NULL, NULL);
    pipeline = gst_pipeline_new("my-pipeline");

    source = gst_element_factory_make ("pulsesrc", "source");
    sink = gst_element_factory_make ("pulsesink", "sink");

    gst_bin_add_many (GST_BIN (pipeline), source, sink, (char *)NULL);

    if (!gst_element_link_many (source, sink, (char *)NULL)) {
        return false;
    }

    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    return true;
}

bool V4LRadioControl::isAvailable() const
{
    return available;
}

QtMultimediaKit::AvailabilityError V4LRadioControl::availabilityError() const
{
    if (fd > 0)
        return QtMultimediaKit::NoError;
    else
        return QtMultimediaKit::ResourceError;
}

QRadioTuner::State V4LRadioControl::state() const
{
    return fd > 0 ? QRadioTuner::ActiveState : QRadioTuner::StoppedState;
}

QRadioTuner::Band V4LRadioControl::band() const
{
    return currentBand;
}

bool V4LRadioControl::isBandSupported(QRadioTuner::Band b) const
{
    QRadioTuner::Band bnd = (QRadioTuner::Band)b;
    switch(bnd) {
        case QRadioTuner::FM:
            if(freqMin <= 87500000 && freqMax >= 108000000)
                return true;
            break;
        case QRadioTuner::LW:
            if(freqMin <= 148500 && freqMax >= 283500)
                return true;
        case QRadioTuner::AM:
            if(freqMin <= 520000 && freqMax >= 1610000)
                return true;
        default:
            if(freqMin <= 1711000 && freqMax >= 30000000)
                return true;
    }

    return false;
}

void V4LRadioControl::setBand(QRadioTuner::Band b)
{
    if(freqMin <= 87500000 && freqMax >= 108000000 && b == QRadioTuner::FM) {
        // FM 87.5 to 108.0 MHz, except Japan 76-90 MHz
        currentBand =  (QRadioTuner::Band)b;
        step = 100000; // 100kHz steps
        emit bandChanged(currentBand);

    } else if(freqMin <= 148500 && freqMax >= 283500 && b == QRadioTuner::LW) {
        // LW 148.5 to 283.5 kHz, 9kHz channel spacing (Europe, Africa, Asia)
        currentBand =  (QRadioTuner::Band)b;
        step = 1000; // 1kHz steps
        emit bandChanged(currentBand);

    } else if(freqMin <= 520000 && freqMax >= 1610000 && b == QRadioTuner::AM) {
        // AM 520 to 1610 kHz, 9 or 10kHz channel spacing, extended 1610 to 1710 kHz
        currentBand =  (QRadioTuner::Band)b;
        step = 1000; // 1kHz steps
        emit bandChanged(currentBand);

    } else if(freqMin <= 1711000 && freqMax >= 30000000 && b == QRadioTuner::SW) {
        // SW 1.711 to 30.0 MHz, divided into 15 bands. 5kHz channel spacing
        currentBand =  (QRadioTuner::Band)b;
        step = 500; // 500Hz steps
        emit bandChanged(currentBand);
    }
}

int V4LRadioControl::frequency() const
{
    return currentFreq;
}

int V4LRadioControl::frequencyStep(QRadioTuner::Band b) const
{
    int step = 0;

    if(b == QRadioTuner::FM)
        step = 100000; // 100kHz steps
    else if(b == QRadioTuner::LW)
        step = 1000; // 1kHz steps
    else if(b == QRadioTuner::AM)
        step = 1000; // 1kHz steps
    else if(b == QRadioTuner::SW)
        step = 500; // 500Hz steps

    return step;
}

QPair<int,int> V4LRadioControl::frequencyRange(QRadioTuner::Band b) const
{
    if(b == QRadioTuner::AM)
        return qMakePair<int,int>(520000,1710000);
    else if(b == QRadioTuner::FM)
        return qMakePair<int,int>(87500000,108000000);
    else if(b == QRadioTuner::SW)
        return qMakePair<int,int>(1711111,30000000);
    else if(b == QRadioTuner::LW)
        return qMakePair<int,int>(148500,283500);

    return qMakePair<int,int>(0,0);
}

void V4LRadioControl::setFrequency(int frequency)
{
    qint64 f = frequency;

    v4l2_frequency freq;

    if(frequency < freqMin)
        f = freqMax;
    if(frequency > freqMax)
        f = freqMin;

    if(fd > 0) {
        memset(&freq, 0, sizeof(freq));
        // Use the first tuner
        freq.tuner = 0;
        if (::ioctl(fd, VIDIOC_G_FREQUENCY, &freq) >= 0) {
            if(low) {
                // For low, freq in units of 62.5Hz, so convert from Hz to units.
                freq.frequency = (int)(f/62.5);
            } else {
                // For high, freq in units of 62.5kHz, so convert from Hz to units.
                freq.frequency = (int)(f/62500);
            }
            ::ioctl(fd, VIDIOC_S_FREQUENCY, &freq);
            currentFreq = f;
            emit frequencyChanged(currentFreq);
	    
            int signal = signalStrength();
            if(sig != signal) {
                sig = signal;
                emit signalStrengthChanged(sig);
            }
        }
    }
}

bool V4LRadioControl::isStereo() const
{
    return stereo;
}

QRadioTuner::StereoMode V4LRadioControl::stereoMode() const
{
    return QRadioTuner::Auto;
}

void V4LRadioControl::setStereoMode(QRadioTuner::StereoMode mode)
{
    bool stereo = true;

    if(mode == QRadioTuner::ForceMono)
        stereo = false;

    v4l2_tuner tuner;

    memset(&tuner, 0, sizeof(tuner));

    if (ioctl(fd, VIDIOC_G_TUNER, &tuner) >= 0) {
        if(stereo)
            tuner.audmode = V4L2_TUNER_MODE_STEREO;
        else
            tuner.audmode = V4L2_TUNER_MODE_MONO;

        if (ioctl(fd, VIDIOC_S_TUNER, &tuner) >= 0) {
            emit stereoStatusChanged(stereo);
        }
    }
}

int V4LRadioControl::signalStrength() const
{
    v4l2_tuner tuner;

    tuner.index = 0;
    if (ioctl(fd, VIDIOC_G_TUNER, &tuner) < 0 || tuner.type != V4L2_TUNER_RADIO)
        return 0;
    return tuner.signal*100/65535;
}

int  V4LRadioControl::vol(snd_hctl_elem_t *elem) const
{
    const QString card("hw:0");
    int err;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&control);
    if ((err = snd_hctl_elem_info(elem, info)) < 0) {
        return 0;
    }

    snd_hctl_elem_get_id(elem, id);
    snd_hctl_elem_read(elem, control);

    return snd_ctl_elem_value_get_integer(control, 0);
}

int V4LRadioControl::volume() const
{
    const QString ctlName("Line DAC Playback Volume");
    const QString card("hw:0");

    int volume = 0;
    int err;
    static snd_ctl_t *handle = NULL;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *control;

    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);	/* MIXER */

    snd_ctl_elem_id_set_name(id, ctlName.toAscii());

    if ((err = snd_ctl_open(&handle, card.toAscii(), 0)) < 0) {
        return 0;
    }

    snd_ctl_elem_info_set_id(info, id);
    if ((err = snd_ctl_elem_info(handle, info)) < 0) {
        snd_ctl_close(handle);
        handle = NULL;
        return 0;
    }

    snd_ctl_elem_info_get_id(info, id);	/* FIXME: Remove it when hctl find works ok !!! */
    snd_ctl_elem_value_set_id(control, id);

    snd_ctl_close(handle);
    handle = NULL;

    snd_hctl_t *hctl;
    snd_hctl_elem_t *elem;
    if ((err = snd_hctl_open(&hctl, card.toAscii(), 0)) < 0) {
        return 0;
    }
    if ((err = snd_hctl_load(hctl)) < 0) {
        return 0;
    }
    elem = snd_hctl_find_elem(hctl, id);
    if (elem)
        volume = vol(elem);

    snd_hctl_close(hctl);

    return (volume/118.0) * 100;
}

void V4LRadioControl::setVolume(int volume)
{
    int vol = (volume / 100.0) * 118; // 118 is a headphone max setting
    callAmixer("Line DAC Playback Volume", QString().setNum(vol)+QString(",")+QString().setNum(vol));
}

bool V4LRadioControl::isMuted() const
{
    return muted;
}

void V4LRadioControl::setMuted(bool muted)
{
    struct v4l2_control vctrl;
    vctrl.id = V4L2_CID_AUDIO_MUTE;
    vctrl.value = muted ? 1 : 0;
    ioctl(fd, VIDIOC_S_CTRL, &vctrl);
}

void V4LRadioControl::setupHeadPhone()
{
    QMap<QString, QString> settings;

    settings["Jack Function"] = "Headset";
    settings["Left DAC_L1 Mixer HP Switch"] = "off";
    settings["Right DAC_R1 Mixer HP Switch"] = "off";
    settings["Line DAC Playback Switch"] = "on";
    settings["Line DAC Playback Volume"] = "118,118"; // Volume is set to 100%
    settings["HPCOM DAC Playback Switch"] = "off";
    settings["Left DAC_L1 Mixer HP Switch"] = "off";
    settings["Left DAC_L1 Mixer Line Switch"] = "on";
    settings["Right DAC_R1 Mixer HP Switch"] = "off";
    settings["Right DAC_R1 Mixer Line Switch"] = "on";
    settings["Speaker Function"] = "Off";

    settings["Input Select"] = "ADC";
    settings["Left PGA Mixer Line1L Switch"] = "off";
    settings["Right PGA Mixer Line1L Switch"] = "off";
    settings["Right PGA Mixer Line1R Switch"] = "off";
    settings["PGA Capture Volume"] = "0,0";

    settings["PGA Capture Switch"] = "on";

    settings["Left PGA Mixer Line2L Switch"] = "on";
    settings["Right PGA Mixer Line2R Switch"] = "on";

    QMapIterator<QString, QString> i(settings);
    while (i.hasNext()) {
        i.next();
        callAmixer(i.key(), i.value());
    }
}

void V4LRadioControl::callAmixer(const QString& target, const QString& value)
{
    int err;
    long tmp;
    unsigned int count;
    static snd_ctl_t *handle = NULL;
    QString card("hw:0");
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_type_t type;

    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);	/* MIXER */

    // in amixer parse func
    // char target[64];
    // e.g. PGA CAPTure Switch
    snd_ctl_elem_id_set_name(id, target.toAscii());

    if (handle == NULL && (err = snd_ctl_open(&handle, card.toAscii(), 0)) < 0)
    {
        return;
    }

    snd_ctl_elem_info_set_id(info, id);
    if ((err = snd_ctl_elem_info(handle, info)) < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return;
    }

    snd_ctl_elem_info_get_id(info, id);	/* FIXME: Remove it when hctl find works ok !!! */
    type = snd_ctl_elem_info_get_type(info);
    count = snd_ctl_elem_info_get_count(info);

    snd_ctl_elem_value_set_id(control, id);

    tmp = 0;
    for (uint idx = 0; idx < count && idx < 128; idx++)
    {
        switch (type)
        {
            case SND_CTL_ELEM_TYPE_BOOLEAN:
#ifdef MULTIMEDIA_MAEMO_DEBUG
                qDebug() << "SND_CTL_ELEM_TYPE_BOOLEAN" << SND_CTL_ELEM_TYPE_BOOLEAN;
#endif
                if ((value == "on") ||(value == "1"))
                {
                    tmp = 1;
                }
                snd_ctl_elem_value_set_boolean(control, idx, tmp);
                break;
            case SND_CTL_ELEM_TYPE_ENUMERATED:
                tmp = getEnumItemIndex(handle, info, value);
                snd_ctl_elem_value_set_enumerated(control, idx, tmp);
                break;
            case SND_CTL_ELEM_TYPE_INTEGER:
#ifdef MULTIMEDIA_MAEMO_DEBUG
                qDebug() << "SND_CTL_ELEM_TYPE_INTEGER" << SND_CTL_ELEM_TYPE_INTEGER;
#endif
                tmp = atoi(value.toAscii());
                if (tmp <  snd_ctl_elem_info_get_min(info))
                    tmp = snd_ctl_elem_info_get_min(info);
                else if (tmp > snd_ctl_elem_info_get_max(info))
                    tmp = snd_ctl_elem_info_get_max(info);
                snd_ctl_elem_value_set_integer(control, idx, tmp);
                break;
            default:
                break;

        }
    }

    if ((err = snd_ctl_elem_write(handle, control)) < 0) {
        snd_ctl_close(handle);
        handle = NULL;
        return;
    }

    snd_ctl_close(handle);
    handle = NULL;
}


int V4LRadioControl::getEnumItemIndex(snd_ctl_t *handle, snd_ctl_elem_info_t *info,
       const QString &value)
{
    int items, i;

    items = snd_ctl_elem_info_get_items(info);
    if (items <= 0)
        return -1;

    for (i = 0; i < items; i++)
    {
        snd_ctl_elem_info_set_item(info, i);
        if (snd_ctl_elem_info(handle, info) < 0)
            return -1;
        QString name = snd_ctl_elem_info_get_item_name(info);
        if(name == value)
        {
            return i;
        }
    }
    return -1;
}

bool V4LRadioControl::isSearching() const
{
    return scanning;
}

void V4LRadioControl::cancelSearch()
{
    scanning = false;
    timer->stop();
}

void V4LRadioControl::searchForward()
{
    // Scan up
    if(scanning) {
        cancelSearch();
        return;
    }
    scanning = true;
    forward  = true;
    timer->start();
}

void V4LRadioControl::searchBackward()
{
    // Scan down
    if(scanning) {
        cancelSearch();
        return;
    }
    scanning = true;
    forward  = false;
    timer->start();
}

void V4LRadioControl::start()
{
}

void V4LRadioControl::stop()
{
}

QRadioTuner::Error V4LRadioControl::error() const
{
    if(m_error)
        return QRadioTuner::OpenError;

    return QRadioTuner::NoError;
}

QString V4LRadioControl::errorString() const
{
    return QString();
}

void V4LRadioControl::search()
{
    if(!scanning) return;

    if(forward) {
        setFrequency(currentFreq+step);
    } else {
        setFrequency(currentFreq-step);
    }
    
    int signal = signalStrength();
    if(sig != signal) {
        sig = signal;
        emit signalStrengthChanged(sig);
    }
    
    if (signal > 25) {
        cancelSearch();
        return;
    }
}

bool V4LRadioControl::initRadio()
{
    v4l2_tuner tuner;
    v4l2_frequency freq;
    v4l2_capability cap;

    low = false;
    available = false;
    freqMin = freqMax = currentFreq = 0;

    fd = ::open("/dev/radio1", O_RDWR);

    if(fd != -1) {
        // Capabilities
        memset(&cap, 0, sizeof(cap));
        if(::ioctl(fd, VIDIOC_QUERYCAP, &cap ) >= 0) {
            available = true;
        }

        tuner.index = 0;

        if (ioctl(fd, VIDIOC_G_TUNER, &tuner) < 0) {
            return false;
        }

        if (tuner.type != V4L2_TUNER_RADIO)
            return false;

        if ((tuner.capability & V4L2_TUNER_CAP_LOW) != 0) {
            // Units are 1/16th of a kHz.
            low = true;
        }

        if(low) {
            freqMin = tuner.rangelow * 62.5;
            freqMax = tuner.rangehigh * 62.5;
        } else {
            freqMin = tuner.rangelow * 62500;
            freqMax = tuner.rangehigh * 62500;
        }

        // frequency
        memset(&freq, 0, sizeof(freq));

        if(::ioctl(fd, VIDIOC_G_FREQUENCY, &freq) >= 0) {
            if (((int)freq.frequency) != -1) {    // -1 means not set.
                if(low)
                    currentFreq = freq.frequency * 62.5;
                else
                    currentFreq = freq.frequency * 62500;
            }
        }

        // stereo
        bool stereo = false;
        memset(&tuner, 0, sizeof(tuner));
        if (ioctl(fd, VIDIOC_G_TUNER, &tuner) >= 0) {
            if((tuner.rxsubchans & V4L2_TUNER_SUB_STEREO) != 0)
                stereo = true;
        }

        return true;
    }

    m_error = true;
    emit error();

    return false;
}
