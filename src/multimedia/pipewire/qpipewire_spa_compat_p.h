// Copyright (C) 2018 Wim Taymans
// SPDX-License-Identifier: MIT

#ifndef QPIPEWIRE_SPA_COMPAT_P_H
#define QPIPEWIRE_SPA_COMPAT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// clang-format off

// old distros may miss functinality from newer upstream.
// we collect them here for the time being

#if !__has_include(<spa/param/audio/raw-utils.h>)
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup spa_param
 * \{
 */

#include <spa/pod/parser.h>
#include <spa/pod/builder.h>
#include <spa/param/audio/format.h>
#include <spa/param/format-utils.h>

static inline int
spa_format_audio_raw_parse(const struct spa_pod *format, struct spa_audio_info_raw *info)
{
	struct spa_pod *position = NULL;
	int res;
	info->flags = 0;
	res = spa_pod_parse_object(format,
			SPA_TYPE_OBJECT_Format, NULL,
			SPA_FORMAT_AUDIO_format,	SPA_POD_OPT_Id(&info->format),
			SPA_FORMAT_AUDIO_rate,		SPA_POD_OPT_Int(&info->rate),
			SPA_FORMAT_AUDIO_channels,	SPA_POD_OPT_Int(&info->channels),
			SPA_FORMAT_AUDIO_position,	SPA_POD_OPT_Pod(&position));
	if (position == NULL ||
	    !spa_pod_copy_array(position, SPA_TYPE_Id, info->position, SPA_AUDIO_MAX_CHANNELS))
		SPA_FLAG_SET(info->flags, SPA_AUDIO_FLAG_UNPOSITIONED);

	return res;
}

static inline struct spa_pod *
spa_format_audio_raw_build(struct spa_pod_builder *builder, uint32_t id,
			   const struct spa_audio_info_raw *info)
{
	struct spa_pod_frame f;
	spa_pod_builder_push_object(builder, &f, SPA_TYPE_OBJECT_Format, id);
	spa_pod_builder_add(builder,
			SPA_FORMAT_mediaType,		SPA_POD_Id(SPA_MEDIA_TYPE_audio),
			SPA_FORMAT_mediaSubtype,	SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
			0);
	if (info->format != SPA_AUDIO_FORMAT_UNKNOWN)
		spa_pod_builder_add(builder,
			SPA_FORMAT_AUDIO_format,	SPA_POD_Id(info->format), 0);
	if (info->rate != 0)
		spa_pod_builder_add(builder,
			SPA_FORMAT_AUDIO_rate,		SPA_POD_Int(info->rate), 0);
	if (info->channels != 0) {
		spa_pod_builder_add(builder,
			SPA_FORMAT_AUDIO_channels,	SPA_POD_Int(info->channels), 0);
		if (!SPA_FLAG_IS_SET(info->flags, SPA_AUDIO_FLAG_UNPOSITIONED)) {
			spa_pod_builder_add(builder, SPA_FORMAT_AUDIO_position,
				SPA_POD_Array(sizeof(uint32_t), SPA_TYPE_Id,
					info->channels, info->position), 0);
		}
	}
	return (struct spa_pod*)spa_pod_builder_pop(builder, &f);
}

/**
 * \}
 */

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // !__has_include(<spa/param/audio/raw-utils.h>)

#if !__has_include(<spa/param/audio/iec958.h>)

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup spa_param
 * \{
 */
enum spa_audio_iec958_codec {
	SPA_AUDIO_IEC958_CODEC_UNKNOWN,

	SPA_AUDIO_IEC958_CODEC_PCM,
	SPA_AUDIO_IEC958_CODEC_DTS,
	SPA_AUDIO_IEC958_CODEC_AC3,
	SPA_AUDIO_IEC958_CODEC_MPEG,		/**< MPEG-1 or MPEG-2 (Part 3, not AAC) */
	SPA_AUDIO_IEC958_CODEC_MPEG2_AAC,	/**< MPEG-2 AAC */

	SPA_AUDIO_IEC958_CODEC_EAC3,

	SPA_AUDIO_IEC958_CODEC_TRUEHD,		/**< Dolby TrueHD */
	SPA_AUDIO_IEC958_CODEC_DTSHD,		/**< DTS-HD Master Audio */
};

struct spa_audio_info_iec958 {
	enum spa_audio_iec958_codec codec;	/*< format, one of the DSP formats in enum spa_audio_format_dsp */
	uint32_t flags;				/*< extra flags */
	uint32_t rate;				/*< sample rate */
};

#define SPA_AUDIO_INFO_IEC958_INIT(...)		((struct spa_audio_info_iec958) { __VA_ARGS__ })

/**
 * \}
 */

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // !__has_include(<spa/param/audio/iec958.h>)

#if !__has_include(<spa/param/format.h>)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup spa_param
 * \{
 */

/** media type for SPA_TYPE_OBJECT_Format */
enum spa_media_type {
	SPA_MEDIA_TYPE_unknown,
	SPA_MEDIA_TYPE_audio,
	SPA_MEDIA_TYPE_video,
	SPA_MEDIA_TYPE_image,
	SPA_MEDIA_TYPE_binary,
	SPA_MEDIA_TYPE_stream,
	SPA_MEDIA_TYPE_application,
};

/** media subtype for SPA_TYPE_OBJECT_Format */
enum spa_media_subtype {
	SPA_MEDIA_SUBTYPE_unknown,
	SPA_MEDIA_SUBTYPE_raw,
	SPA_MEDIA_SUBTYPE_dsp,
	SPA_MEDIA_SUBTYPE_iec958,	/** S/PDIF */
	SPA_MEDIA_SUBTYPE_dsd,

	SPA_MEDIA_SUBTYPE_START_Audio	= 0x10000,
	SPA_MEDIA_SUBTYPE_mp3,
	SPA_MEDIA_SUBTYPE_aac,
	SPA_MEDIA_SUBTYPE_vorbis,
	SPA_MEDIA_SUBTYPE_wma,
	SPA_MEDIA_SUBTYPE_ra,
	SPA_MEDIA_SUBTYPE_sbc,
	SPA_MEDIA_SUBTYPE_adpcm,
	SPA_MEDIA_SUBTYPE_g723,
	SPA_MEDIA_SUBTYPE_g726,
	SPA_MEDIA_SUBTYPE_g729,
	SPA_MEDIA_SUBTYPE_amr,
	SPA_MEDIA_SUBTYPE_gsm,
	SPA_MEDIA_SUBTYPE_alac,		/** since 0.3.65 */
	SPA_MEDIA_SUBTYPE_flac,		/** since 0.3.65 */
	SPA_MEDIA_SUBTYPE_ape,		/** since 0.3.65 */
	SPA_MEDIA_SUBTYPE_opus,		/** since 0.3.68 */
	SPA_MEDIA_SUBTYPE_ac3,		/** since 1.5.1 */
	SPA_MEDIA_SUBTYPE_eac3,		/** since 1.5.1 */
	SPA_MEDIA_SUBTYPE_truehd,	/** since 1.5.1 */
	SPA_MEDIA_SUBTYPE_dts,		/** since 1.5.1 */
	SPA_MEDIA_SUBTYPE_mpegh,	/** since 1.5.1 */

	SPA_MEDIA_SUBTYPE_START_Video	= 0x20000,
	SPA_MEDIA_SUBTYPE_h264,
	SPA_MEDIA_SUBTYPE_mjpg,
	SPA_MEDIA_SUBTYPE_dv,
	SPA_MEDIA_SUBTYPE_mpegts,
	SPA_MEDIA_SUBTYPE_h263,
	SPA_MEDIA_SUBTYPE_mpeg1,
	SPA_MEDIA_SUBTYPE_mpeg2,
	SPA_MEDIA_SUBTYPE_mpeg4,
	SPA_MEDIA_SUBTYPE_xvid,
	SPA_MEDIA_SUBTYPE_vc1,
	SPA_MEDIA_SUBTYPE_vp8,
	SPA_MEDIA_SUBTYPE_vp9,
	SPA_MEDIA_SUBTYPE_bayer,
	SPA_MEDIA_SUBTYPE_h265,

	SPA_MEDIA_SUBTYPE_START_Image	= 0x30000,
	SPA_MEDIA_SUBTYPE_jpeg,

	SPA_MEDIA_SUBTYPE_START_Binary	= 0x40000,

	SPA_MEDIA_SUBTYPE_START_Stream	= 0x50000,
	SPA_MEDIA_SUBTYPE_midi,

	SPA_MEDIA_SUBTYPE_START_Application	= 0x60000,
	SPA_MEDIA_SUBTYPE_control,		/**< control stream, data contains
						  *  spa_pod_sequence with control info. */
};

/** properties for audio SPA_TYPE_OBJECT_Format */
enum spa_format {
	SPA_FORMAT_START,

	SPA_FORMAT_mediaType,		/**< media type (Id enum spa_media_type) */
	SPA_FORMAT_mediaSubtype,	/**< media subtype (Id enum spa_media_subtype) */

	/* Audio format keys */
	SPA_FORMAT_START_Audio = 0x10000,
	SPA_FORMAT_AUDIO_format,		/**< audio format, (Id enum spa_audio_format) */
	SPA_FORMAT_AUDIO_flags,			/**< optional flags (Int) */
	SPA_FORMAT_AUDIO_rate,			/**< sample rate (Int) */
	SPA_FORMAT_AUDIO_channels,		/**< number of audio channels (Int) */
	SPA_FORMAT_AUDIO_position,		/**< channel positions (Id enum spa_audio_position) */

	SPA_FORMAT_AUDIO_iec958Codec,		/**< codec used (IEC958) (Id enum spa_audio_iec958_codec) */

	SPA_FORMAT_AUDIO_bitorder,		/**< bit order (Id enum spa_param_bitorder) */
	SPA_FORMAT_AUDIO_interleave,		/**< Interleave bytes (Int) */
	SPA_FORMAT_AUDIO_bitrate,		/**< bit rate (Int) */
	SPA_FORMAT_AUDIO_blockAlign,    	/**< audio data block alignment (Int) */

	SPA_FORMAT_AUDIO_AAC_streamFormat,	/**< AAC stream format, (Id enum spa_audio_aac_stream_format) */

	SPA_FORMAT_AUDIO_WMA_profile,		/**< WMA profile (Id enum spa_audio_wma_profile) */

	SPA_FORMAT_AUDIO_AMR_bandMode,		/**< AMR band mode (Id enum spa_audio_amr_band_mode) */

	SPA_FORMAT_AUDIO_MP3_channelMode,	/**< MP3 channel mode, (Id enum spa_audio_mp3_channel_mode) */

	SPA_FORMAT_AUDIO_DTS_extType,		/**< DTS extension type (Id enum spa_audio_dts_ext_type) */


	/* Video Format keys */
	SPA_FORMAT_START_Video = 0x20000,
	SPA_FORMAT_VIDEO_format,		/**< video format (Id enum spa_video_format) */
	SPA_FORMAT_VIDEO_modifier,		/**< format modifier (Long)
						  * use only with DMA-BUF and omit for other buffer types */
	SPA_FORMAT_VIDEO_size,			/**< size (Rectangle) */
	SPA_FORMAT_VIDEO_framerate,		/**< frame rate (Fraction) */
	SPA_FORMAT_VIDEO_maxFramerate,		/**< maximum frame rate (Fraction) */
	SPA_FORMAT_VIDEO_views,			/**< number of views (Int) */
	SPA_FORMAT_VIDEO_interlaceMode,		/**< (Id enum spa_video_interlace_mode) */
	SPA_FORMAT_VIDEO_pixelAspectRatio,	/**< (Rectangle) */
	SPA_FORMAT_VIDEO_multiviewMode,		/**< (Id enum spa_video_multiview_mode) */
	SPA_FORMAT_VIDEO_multiviewFlags,	/**< (Id enum spa_video_multiview_flags) */
	SPA_FORMAT_VIDEO_chromaSite,		/**< /Id enum spa_video_chroma_site) */
	SPA_FORMAT_VIDEO_colorRange,		/**< /Id enum spa_video_color_range) */
	SPA_FORMAT_VIDEO_colorMatrix,		/**< /Id enum spa_video_color_matrix) */
	SPA_FORMAT_VIDEO_transferFunction,	/**< /Id enum spa_video_transfer_function) */
	SPA_FORMAT_VIDEO_colorPrimaries,	/**< /Id enum spa_video_color_primaries) */
	SPA_FORMAT_VIDEO_profile,		/**< (Int) */
	SPA_FORMAT_VIDEO_level,			/**< (Int) */
	SPA_FORMAT_VIDEO_H264_streamFormat,	/**< (Id enum spa_h264_stream_format) */
	SPA_FORMAT_VIDEO_H264_alignment,	/**< (Id enum spa_h264_alignment) */
	SPA_FORMAT_VIDEO_H265_streamFormat,	/**< (Id enum spa_h265_stream_format) */
	SPA_FORMAT_VIDEO_H265_alignment,	/**< (Id enum spa_h265_alignment) */
	SPA_FORMAT_VIDEO_deviceId,	        /**< dev_t identifier (Bytes) */

	/* Image Format keys */
	SPA_FORMAT_START_Image = 0x30000,
	/* Binary Format keys */
	SPA_FORMAT_START_Binary = 0x40000,
	/* Stream Format keys */
	SPA_FORMAT_START_Stream = 0x50000,
	/* Application Format keys */
	SPA_FORMAT_START_Application = 0x60000,
	SPA_FORMAT_CONTROL_types,		/**< possible control types (flags choice Int,
						  *  mask of enum spa_control_type) */
};

#define SPA_KEY_FORMAT_DSP		"format.dsp"		/**< a predefined DSP format,
								  *  Ex. "32 bit float mono audio" */

/**
 * \}
 */

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // !__has_include(<spa/param/format.h>)

#endif // QPIPEWIRE_SPA_COMPAT_P_H
