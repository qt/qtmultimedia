qt_internal_add_module(SpatialAudio
    SOURCES
        qambisonicdecoder.cpp qambisonicdecoder_p.h qambisonicdecoderdata_p.h
        qaudioengine.cpp qaudioengine.h qaudioengine_p.h
        qaudiolistener.cpp qaudiolistener.h
        qaudioroom.cpp qaudioroom.h qaudioroom_p.h
        qspatialsound.cpp qspatialsound.h qspatialsound.h
        qambientsound.cpp qambientsound.h
        qtspatialaudioglobal.h qtspatialaudioglobal_p.h
    INCLUDE_DIRECTORIES
        "../3rdparty/resonance-audio/resonance_audio"
        "../3rdparty/resonance-audio"
        "../resonance-audio"
        "../3rdparty/eigen"
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::BundledResonanceAudio
    PUBLIC_LIBRARIES
        Qt::Multimedia
    GENERATE_CPP_EXPORTS
)


qt_internal_add_docs(SpatialAudio
    doc/qtspatialaudio.qdocconf
)
