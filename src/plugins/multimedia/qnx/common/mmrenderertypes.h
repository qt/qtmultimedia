// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef MMRENDERERTYPES_H
#define MMRENDERERTYPES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <mm/renderer.h>
#include <mm/renderer/types.h>

extern "C" {
// ### replace with proper include: mm/renderer/events.h
typedef enum mmr_state {
    MMR_STATE_DESTROYED,
    MMR_STATE_IDLE,
    MMR_STATE_STOPPED,
    MMR_STATE_PLAYING
} mmr_state_t;

typedef enum mmr_event_type {
    MMR_EVENT_NONE,
    MMR_EVENT_ERROR,
    MMR_EVENT_STATE,
    MMR_EVENT_OVERFLOW,
    MMR_EVENT_WARNING,
    MMR_EVENT_STATUS,
    MMR_EVENT_METADATA,
    MMR_EVENT_PLAYLIST,
    MMR_EVENT_INPUT,
    MMR_EVENT_OUTPUT,
    MMR_EVENT_CTXTPAR,
    MMR_EVENT_TRKPAR,
    MMR_EVENT_OTHER
} mmr_event_type_t;

typedef struct mmr_event {
    mmr_event_type_t type;
    mmr_state_t state;
    int speed;
    union mmr_event_details {

        struct mmr_event_state {
            mmr_state_t oldstate;
            int oldspeed;
        } state;

        struct mmr_event_error {
            mmr_error_info_t info;
        } error;

        struct mmr_event_warning {
            const char *str;
            const strm_string_t *obj;
        } warning;

        struct mmr_event_metadata {
            unsigned index;
        } metadata;

        struct mmr_event_trkparam {
            unsigned index;
        } trkparam;

        struct mmr_event_playlist {
            unsigned start;
            unsigned end;
            unsigned length;
        } playlist;

        struct mmr_event_output {
            unsigned id;
        } output;
    } details;

    const strm_string_t* pos_obj;
    const char* pos_str;
    const strm_dict_t* data;
    const char* objname;
    void* usrdata;
} mmr_event_t;

const mmr_event_t* mmr_event_get(mmr_context_t *ctxt);

}

#endif
