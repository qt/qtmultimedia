# Testdata notes

## color_matrix_90_deg_clockwise.m2v

GStreamer discovery API detects two video streams in color_matrix_90_deg_clockwise.m2v where one of
the streams has a null stream ID. This file is intended to test that QMediaPlayer filters out such
streams from metadata. It is created from color_matrix_90_deg_clockwise.mp4 using the FFmpeg
command line:

    ffmpeg -i color_matrix_90_deg_clockwise.mp4 -vf "scale=64:-1" -c:v mpeg2video color_matrix_90_deg_clockwise.m2v
