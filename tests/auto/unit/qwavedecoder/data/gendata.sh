#!/bin/bash
#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
##
##
## $QT_END_LICENSE$
##
#############################################################################

# Generate some simple test data.  Uses "sox".

endian=""
endian_extn=""

for channel in 1 2; do
    if [ $channel -eq 1 ]; then
        endian="little"
        endian_extn="le"
    fi

    if [ $channel -eq 2 ]; then
        endian="big"
        endian_extn="be"
    fi
    for samplebits in 8 16 32; do
        for samplerate in 44100 8000; do
            if [ $samplebits -ne 8 ]; then
                sox -n --endian "${endian}" -c ${channel} -b ${samplebits} -r ${samplerate} isawav_${channel}_${samplebits}_${samplerate}_${endian_extn}.wav synth 0.25 sine 300-3300
            else
                sox -n -c ${channel} -b ${samplebits} -r ${samplerate} isawav_${channel}_${samplebits}_${samplerate}.wav synth 0.25 sine 300-3300
            fi
        done
     done
done

