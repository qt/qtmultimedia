// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include <alsa/asoundlib.h>
#if SND_LIB_VERSION < 0x1000a  // 1.0.10
#error "Alsa version found too old, require >= 1.0.10"
#endif

int main(int argc,char **argv)
{
}

