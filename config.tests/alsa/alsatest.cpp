// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <alsa/asoundlib.h>
#if SND_LIB_VERSION < 0x1000a  // 1.0.10
#error "Alsa version found too old, require >= 1.0.10"
#endif

int main(int argc,char **argv)
{
}

