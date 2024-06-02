# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(MINGW AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(NO_SIMD_DEFINES PFFFT_SIMD_DISABLE DISABLE_SIMD)
endif()

set(PFFFT_DIR "../3rdparty/pffft/" CACHE PATH "Path to pffft library")
set(PFFFT_INCLUDE_DIR ${PFFFT_DIR})
set(PFFFT_SOURCE
        ${PFFFT_DIR}/pffft.c
        ${PFFFT_DIR}/pffft.h
        )

set(SADIE_HRTFS_DIR "../3rdparty/resonance-audio/third_party/SADIE_hrtf_database/generated/" CACHE PATH "Path to SADIE_hrtf_database library")
set(SADIE_HRTFS_INCLUDE_DIR ${SADIE_HRTFS_DIR})
set(SADIE_HRTFS_SOURCE
        ${SADIE_HRTFS_DIR}/hrtf_assets.cc
        ${SADIE_HRTFS_DIR}/hrtf_assets.h
        )
add_library(SadieHrtfsObj OBJECT ${SADIE_HRTFS_SOURCE})

set(RA_TOPLEVEL_DIR "../3rdparty/resonance-audio" CACHE PATH "Path to resonance audio top level dir")
set(RA_SOURCE_DIR "${RA_TOPLEVEL_DIR}/resonance_audio")

# Build Resonance Audio.
set(RA_SOURCES
        ${RA_TOPLEVEL_DIR}/platforms/common/utils.cc
        ${RA_TOPLEVEL_DIR}/platforms/common/utils.h
        ${RA_SOURCE_DIR}/ambisonics/ambisonic_binaural_decoder.h
        ${RA_SOURCE_DIR}/ambisonics/ambisonic_binaural_decoder.cc
        ${RA_SOURCE_DIR}/ambisonics/ambisonic_codec.h
        ${RA_SOURCE_DIR}/ambisonics/ambisonic_codec_impl.h
        ${RA_SOURCE_DIR}/ambisonics/ambisonic_lookup_table.cc
        ${RA_SOURCE_DIR}/ambisonics/ambisonic_lookup_table.h
        ${RA_SOURCE_DIR}/ambisonics/ambisonic_spread_coefficients.h
        ${RA_SOURCE_DIR}/ambisonics/associated_legendre_polynomials_generator.cc
        ${RA_SOURCE_DIR}/ambisonics/associated_legendre_polynomials_generator.h
        ${RA_SOURCE_DIR}/ambisonics/foa_rotator.cc
        ${RA_SOURCE_DIR}/ambisonics/foa_rotator.h
        ${RA_SOURCE_DIR}/ambisonics/hoa_rotator.cc
        ${RA_SOURCE_DIR}/ambisonics/hoa_rotator.h
        ${RA_SOURCE_DIR}/ambisonics/stereo_from_soundfield_converter.cc
        ${RA_SOURCE_DIR}/ambisonics/stereo_from_soundfield_converter.h
        ${RA_SOURCE_DIR}/ambisonics/utils.h
        ${RA_SOURCE_DIR}/api/binaural_surround_renderer.cc
        ${RA_SOURCE_DIR}/api/binaural_surround_renderer.h
        ${RA_SOURCE_DIR}/api/resonance_audio_api.cc
        ${RA_SOURCE_DIR}/api/resonance_audio_api.h
        ${RA_SOURCE_DIR}/base/aligned_allocator.h
        ${RA_SOURCE_DIR}/base/audio_buffer.cc
        ${RA_SOURCE_DIR}/base/audio_buffer.h
        ${RA_SOURCE_DIR}/base/channel_view.cc
        ${RA_SOURCE_DIR}/base/channel_view.h
        ${RA_SOURCE_DIR}/base/constants_and_types.h
        ${RA_SOURCE_DIR}/base/integral_types.h
        ${RA_SOURCE_DIR}/base/logging.h
        ${RA_SOURCE_DIR}/base/misc_math.cc
        ${RA_SOURCE_DIR}/base/misc_math.h
        ${RA_SOURCE_DIR}/base/object_transform.h
        ${RA_SOURCE_DIR}/base/simd_macros.h
        ${RA_SOURCE_DIR}/base/simd_utils.cc
        ${RA_SOURCE_DIR}/base/simd_utils.h
        ${RA_SOURCE_DIR}/base/source_parameters.h
        ${RA_SOURCE_DIR}/base/spherical_angle.cc
        ${RA_SOURCE_DIR}/base/spherical_angle.h
        ${RA_SOURCE_DIR}/base/unique_ptr_wrapper.h
        ${RA_SOURCE_DIR}/config/global_config.h
        ${RA_SOURCE_DIR}/config/source_config.cc
        ${RA_SOURCE_DIR}/config/source_config.h
        ${RA_SOURCE_DIR}/dsp/biquad_filter.cc
        ${RA_SOURCE_DIR}/dsp/biquad_filter.h
        ${RA_SOURCE_DIR}/dsp/channel_converter.cc
        ${RA_SOURCE_DIR}/dsp/channel_converter.h
        ${RA_SOURCE_DIR}/dsp/circular_buffer.cc
        ${RA_SOURCE_DIR}/dsp/circular_buffer.h
        ${RA_SOURCE_DIR}/dsp/delay_filter.cc
        ${RA_SOURCE_DIR}/dsp/delay_filter.h
        ${RA_SOURCE_DIR}/dsp/distance_attenuation.cc
        ${RA_SOURCE_DIR}/dsp/distance_attenuation.h
        ${RA_SOURCE_DIR}/dsp/fft_manager.cc
        ${RA_SOURCE_DIR}/dsp/fft_manager.h
        ${RA_SOURCE_DIR}/dsp/filter_coefficient_generators.cc
        ${RA_SOURCE_DIR}/dsp/filter_coefficient_generators.h
        ${RA_SOURCE_DIR}/dsp/fir_filter.cc
        ${RA_SOURCE_DIR}/dsp/fir_filter.h
        ${RA_SOURCE_DIR}/dsp/gain.cc
        ${RA_SOURCE_DIR}/dsp/gain.h
        ${RA_SOURCE_DIR}/dsp/gain_mixer.cc
        ${RA_SOURCE_DIR}/dsp/gain_mixer.h
        ${RA_SOURCE_DIR}/dsp/gain_processor.cc
        ${RA_SOURCE_DIR}/dsp/gain_processor.h
        ${RA_SOURCE_DIR}/dsp/mixer.cc
        ${RA_SOURCE_DIR}/dsp/mixer.h
        ${RA_SOURCE_DIR}/dsp/mono_pole_filter.cc
        ${RA_SOURCE_DIR}/dsp/mono_pole_filter.h
        ${RA_SOURCE_DIR}/dsp/multi_channel_iir.cc
        ${RA_SOURCE_DIR}/dsp/multi_channel_iir.h
        ${RA_SOURCE_DIR}/dsp/near_field_processor.cc
        ${RA_SOURCE_DIR}/dsp/near_field_processor.h
        ${RA_SOURCE_DIR}/dsp/occlusion_calculator.cc
        ${RA_SOURCE_DIR}/dsp/occlusion_calculator.h
        ${RA_SOURCE_DIR}/dsp/partitioned_fft_filter.cc
        ${RA_SOURCE_DIR}/dsp/partitioned_fft_filter.h
        ${RA_SOURCE_DIR}/dsp/reflection.h
        ${RA_SOURCE_DIR}/dsp/reflections_processor.cc
        ${RA_SOURCE_DIR}/dsp/reflections_processor.h
        ${RA_SOURCE_DIR}/dsp/resampler.cc
        ${RA_SOURCE_DIR}/dsp/resampler.h
        ${RA_SOURCE_DIR}/dsp/reverb_onset_compensator.cc
        ${RA_SOURCE_DIR}/dsp/reverb_onset_compensator.h
        ${RA_SOURCE_DIR}/dsp/reverb_onset_update_processor.cc
        ${RA_SOURCE_DIR}/dsp/reverb_onset_update_processor.h
        ${RA_SOURCE_DIR}/dsp/sh_hrir_creator.cc
        ${RA_SOURCE_DIR}/dsp/sh_hrir_creator.h
        ${RA_SOURCE_DIR}/dsp/shoe_box_room.cc
        ${RA_SOURCE_DIR}/dsp/shoe_box_room.h
        ${RA_SOURCE_DIR}/dsp/spectral_reverb.cc
        ${RA_SOURCE_DIR}/dsp/spectral_reverb.h
        ${RA_SOURCE_DIR}/dsp/spectral_reverb_constants_and_tables.h
        ${RA_SOURCE_DIR}/dsp/stereo_panner.cc
        ${RA_SOURCE_DIR}/dsp/stereo_panner.h
        ${RA_SOURCE_DIR}/dsp/utils.cc
        ${RA_SOURCE_DIR}/dsp/utils.h
        ${RA_SOURCE_DIR}/graph/ambisonic_binaural_decoder_node.cc
        ${RA_SOURCE_DIR}/graph/ambisonic_binaural_decoder_node.h
        ${RA_SOURCE_DIR}/graph/ambisonic_mixing_encoder_node.cc
        ${RA_SOURCE_DIR}/graph/ambisonic_mixing_encoder_node.h
        ${RA_SOURCE_DIR}/graph/binaural_surround_renderer_impl.cc
        ${RA_SOURCE_DIR}/graph/binaural_surround_renderer_impl.h
        ${RA_SOURCE_DIR}/graph/buffered_source_node.cc
        ${RA_SOURCE_DIR}/graph/buffered_source_node.h
        ${RA_SOURCE_DIR}/graph/foa_rotator_node.cc
        ${RA_SOURCE_DIR}/graph/foa_rotator_node.h
        ${RA_SOURCE_DIR}/graph/gain_mixer_node.cc
        ${RA_SOURCE_DIR}/graph/gain_mixer_node.h
        ${RA_SOURCE_DIR}/graph/gain_node.cc
        ${RA_SOURCE_DIR}/graph/gain_node.h
        ${RA_SOURCE_DIR}/graph/graph_manager.cc
        ${RA_SOURCE_DIR}/graph/graph_manager.h
        ${RA_SOURCE_DIR}/graph/graph_manager_config.h
        ${RA_SOURCE_DIR}/graph/hoa_rotator_node.cc
        ${RA_SOURCE_DIR}/graph/hoa_rotator_node.h
        ${RA_SOURCE_DIR}/graph/mixer_node.cc
        ${RA_SOURCE_DIR}/graph/mixer_node.h
        ${RA_SOURCE_DIR}/graph/mono_from_soundfield_node.cc
        ${RA_SOURCE_DIR}/graph/mono_from_soundfield_node.h
        ${RA_SOURCE_DIR}/graph/near_field_effect_node.cc
        ${RA_SOURCE_DIR}/graph/near_field_effect_node.h
        ${RA_SOURCE_DIR}/graph/occlusion_node.cc
        ${RA_SOURCE_DIR}/graph/occlusion_node.h
        ${RA_SOURCE_DIR}/graph/reflections_node.cc
        ${RA_SOURCE_DIR}/graph/reflections_node.h
        ${RA_SOURCE_DIR}/graph/resonance_audio_api_impl.cc
        ${RA_SOURCE_DIR}/graph/resonance_audio_api_impl.h
        ${RA_SOURCE_DIR}/graph/reverb_node.cc
        ${RA_SOURCE_DIR}/graph/reverb_node.h
        ${RA_SOURCE_DIR}/graph/source_graph_config.h
        ${RA_SOURCE_DIR}/graph/source_parameters_manager.cc
        ${RA_SOURCE_DIR}/graph/source_parameters_manager.h
        ${RA_SOURCE_DIR}/graph/stereo_mixing_panner_node.cc
        ${RA_SOURCE_DIR}/graph/stereo_mixing_panner_node.h
        ${RA_SOURCE_DIR}/graph/system_settings.h
        ${RA_SOURCE_DIR}/node/node.h
        ${RA_SOURCE_DIR}/node/processing_node.cc
        ${RA_SOURCE_DIR}/node/processing_node.h
        ${RA_SOURCE_DIR}/node/publisher_node.h
        ${RA_SOURCE_DIR}/node/sink_node.cc
        ${RA_SOURCE_DIR}/node/sink_node.h
        ${RA_SOURCE_DIR}/node/source_node.cc
        ${RA_SOURCE_DIR}/node/source_node.h
        ${RA_SOURCE_DIR}/node/subscriber_node.h
        ${RA_TOPLEVEL_DIR}/platforms/common/room_effects_utils.cc
        ${RA_TOPLEVEL_DIR}/platforms/common/room_effects_utils.h
        ${RA_SOURCE_DIR}/utils/buffer_crossfader.cc
        ${RA_SOURCE_DIR}/utils/buffer_crossfader.h
        ${RA_SOURCE_DIR}/utils/buffer_partitioner.cc
        ${RA_SOURCE_DIR}/utils/buffer_partitioner.h
        ${RA_SOURCE_DIR}/utils/buffer_unpartitioner.cc
        ${RA_SOURCE_DIR}/utils/buffer_unpartitioner.h
        ${RA_SOURCE_DIR}/utils/lockless_task_queue.cc
        ${RA_SOURCE_DIR}/utils/lockless_task_queue.h
        ${RA_SOURCE_DIR}/utils/planar_interleaved_conversion.cc
        ${RA_SOURCE_DIR}/utils/planar_interleaved_conversion.h
        ${RA_SOURCE_DIR}/utils/pseudoinverse.h
        ${RA_SOURCE_DIR}/utils/sample_type_conversion.cc
        ${RA_SOURCE_DIR}/utils/sample_type_conversion.h
        ${RA_SOURCE_DIR}/utils/semi_lockless_fifo.h
        ${RA_SOURCE_DIR}/utils/sum_and_difference_processor.cc
        ${RA_SOURCE_DIR}/utils/sum_and_difference_processor.h
        ${RA_SOURCE_DIR}/utils/threadsafe_fifo.h
        ${RA_SOURCE_DIR}/utils/wav.cc
        ${RA_SOURCE_DIR}/utils/wav.h
        ${RA_SOURCE_DIR}/utils/wav_reader.cc
        ${RA_SOURCE_DIR}/utils/wav_reader.h
)

qt_internal_add_3rdparty_library(BundledResonanceAudio
    STATIC
    INSTALL
    DEFINES
        EIGEN_MPL2_ONLY
        ${NO_SIMD_DEFINES}
    SOURCES
        ${RA_SOURCES}
        ${PFFFT_SOURCE}
        ${SADIE_HRTFS_SOURCE}
        resonance_audio.h resonance_audio.cpp
    INCLUDE_DIRECTORIES
        ${RA_TOPLEVEL_DIR}
        ${RA_SOURCE_DIR}
        ${PFFFT_DIR}
        ${SADIE_HRTFS_DIR}
        ../3rdparty/eigen
    ATTRIBUTION_FILE_DIR_PATHS
        ${RA_TOPLEVEL_DIR}
        ${PFFFT_DIR}
        ../3rdparty/eigen
)

# Required by pffft on certain PowerPC archs
qt_internal_extend_target(BundledResonanceAudio CONDITION GCC AND (${CMAKE_SYSTEM_PROCESSOR} MATCHES "(ppc|ppc64)$")
    COMPILE_OPTIONS
        -maltivec
)

# Required by eigen on certain PowerPC archs
qt_internal_extend_target(BundledResonanceAudio CONDITION (${CMAKE_SYSTEM_PROCESSOR} MATCHES "(ppc|ppc64)$")
    COMPILE_OPTIONS
        -mvsx
)

# Use fallback mode if SSE is not available
qt_internal_extend_target(BundledResonanceAudio CONDITION (${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[3-6]86(-AT386)?$")
    COMPILE_OPTIONS
        -DPFFFT_SIMD_DISABLE
)

qt_disable_warnings(BundledResonanceAudio)
qt_set_symbol_visibility_hidden(BundledResonanceAudio)

qt_install_3rdparty_library_wrap_config_extra_file(BundledResonanceAudio)
