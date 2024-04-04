// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: BSD-3-Clause

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

int main(int argc, char** argv)
{
    AVPlayer *player = [AVPlayer playerWithURL:[NSURL URLWithString:@"http://doesnotmatter.com"]];
    return 0;
}
