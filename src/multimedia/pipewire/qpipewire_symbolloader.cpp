// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_symbolloader_p.h"

#include <pipewire/pipewire.h>

extern "C" {
#if !PW_CHECK_VERSION(0, 3, 75)
bool pw_check_library_version(int major, int minor, int micro);
#endif
#if !PW_CHECK_VERSION(0, 3, 50)
int pw_stream_get_time_n(struct pw_stream *stream, struct pw_time *time, size_t size);
#endif
}

//BEGIN_INIT_FUNCS("pipewire-0.3", "0")
BEGIN_INIT_FUNCS("pipewire-" PW_API_VERSION, "0")

INIT_FUNC(pw_init);
INIT_FUNC(pw_deinit);
INIT_OPT_FUNC(pw_check_library_version);
INIT_FUNC(pw_context_new);
INIT_FUNC(pw_context_destroy);
INIT_FUNC(pw_context_connect);
INIT_FUNC(pw_context_connect_fd);
INIT_FUNC(pw_core_disconnect);
INIT_FUNC(pw_thread_loop_new);
INIT_FUNC(pw_thread_loop_destroy);
INIT_FUNC(pw_thread_loop_get_loop);
INIT_FUNC(pw_thread_loop_start);
INIT_FUNC(pw_thread_loop_stop);
INIT_FUNC(pw_thread_loop_lock);
INIT_FUNC(pw_thread_loop_unlock);
INIT_FUNC(pw_thread_loop_timed_wait);
INIT_FUNC(pw_thread_loop_signal);
INIT_FUNC(pw_thread_loop_in_thread);
INIT_FUNC(pw_properties_new_dict);
INIT_FUNC(pw_properties_free);
INIT_FUNC(pw_stream_new);
INIT_FUNC(pw_stream_new_simple);
INIT_FUNC(pw_stream_destroy);
INIT_FUNC(pw_stream_add_listener);
INIT_FUNC(pw_stream_connect);
INIT_FUNC(pw_stream_disconnect);
INIT_FUNC(pw_stream_set_active);
INIT_FUNC(pw_stream_dequeue_buffer);
INIT_FUNC(pw_stream_queue_buffer);
INIT_OPT_FUNC(pw_stream_get_time_n);
INIT_FUNC(pw_proxy_destroy);
INIT_FUNC(pw_get_library_version);

END_INIT_FUNCS()

DEFINE_FUNC(pw_init, 2);
DEFINE_FUNC(pw_deinit, 0);
DEFINE_FUNC(pw_check_library_version, 3);
DEFINE_FUNC(pw_context_new, 3);
DEFINE_FUNC(pw_context_destroy, 1);
DEFINE_FUNC(pw_context_connect, 3);
DEFINE_FUNC(pw_context_connect_fd, 4);
DEFINE_FUNC(pw_core_disconnect, 1);
DEFINE_FUNC(pw_thread_loop_new, 2);
DEFINE_FUNC(pw_thread_loop_destroy, 1);
DEFINE_FUNC(pw_thread_loop_get_loop, 1);
DEFINE_FUNC(pw_thread_loop_start, 1);
DEFINE_FUNC(pw_thread_loop_stop, 1);
DEFINE_FUNC(pw_thread_loop_lock, 1);
DEFINE_FUNC(pw_thread_loop_unlock, 1);
DEFINE_FUNC(pw_thread_loop_timed_wait, 2);
DEFINE_FUNC(pw_thread_loop_signal, 2);
DEFINE_FUNC(pw_thread_loop_in_thread, 1);
DEFINE_FUNC(pw_properties_new_dict, 1);
DEFINE_FUNC(pw_properties_free, 1);
DEFINE_FUNC(pw_stream_new, 3);
DEFINE_FUNC(pw_stream_new_simple, 5);
DEFINE_FUNC(pw_stream_destroy, 1);
DEFINE_FUNC(pw_stream_add_listener, 4);
DEFINE_FUNC(pw_stream_connect, 6);
DEFINE_FUNC(pw_stream_disconnect, 1);
DEFINE_FUNC(pw_stream_set_active, 2);
DEFINE_FUNC(pw_stream_dequeue_buffer, 1);
DEFINE_FUNC(pw_stream_queue_buffer, 2);
DEFINE_FUNC(pw_stream_get_time_n, 3, -1);
DEFINE_FUNC(pw_proxy_destroy, 1);
DEFINE_FUNC(pw_get_library_version, 0);

DEFINE_IS_LOADED_CHECKER(qPipewireIsLoaded)
