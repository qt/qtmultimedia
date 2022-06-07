// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

int main(int argc, char** argv)
{
    AVPlayer *player = [AVPlayer playerWithURL:[NSURL URLWithString:@"http://doesnotmatter.com"]];
    return 0;
}
