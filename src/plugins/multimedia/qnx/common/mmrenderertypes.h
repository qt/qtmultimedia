/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
