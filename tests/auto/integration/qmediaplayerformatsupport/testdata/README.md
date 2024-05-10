Sample video files are created using FFmpeg, for example:

:: Supported container formats
ffmpeg -ss 2 -i flipable.gif -t 1 -r 5 containers/supported/container.avi
ffmpeg -ss 2 -i flipable.gif -t 1 -r 5 containers/supported/container.mkv
ffmpeg -ss 2 -i flipable.gif -t 1 -r 5 containers/supported/container.mp4
ffmpeg -ss 2 -i flipable.gif -t 1 -r 25 containers/supported/container.mpeg
ffmpeg -ss 2 -i flipable.gif -t 1 -r 25 containers/supported/container.wmv

:: Unsupported container formats
ffmpeg -ss 2 -i flipable.gif -t 1 -r 5 containers/unsupported/container.webm

:: Supported pixel formats h264
ffmpeg -i flipable.gif -pix_fmt yuv420p -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv420p.mp4
ffmpeg -i flipable.gif -pix_fmt yuv420p10 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv420p10.mp4
ffmpeg -i flipable.gif -pix_fmt yuv422p -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv422p.mp4
ffmpeg -i flipable.gif -pix_fmt yuv422p10 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv422p10.mp4
ffmpeg -i flipable.gif -pix_fmt yuv444p -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv444p.mp4
ffmpeg -i flipable.gif -pix_fmt yuvj420p -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuvj420p.mp4
ffmpeg -i flipable.gif -pix_fmt yuvj422p -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuvj422p.mp4
ffmpeg -i flipable.gif -pix_fmt yuvj444p -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuvj444p.mp4
ffmpeg -i flipable.gif -pix_fmt nv12 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_nv12.mp4
ffmpeg -i flipable.gif -pix_fmt nv16 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_nv16.mp4
ffmpeg -i flipable.gif -pix_fmt nv21 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_nv21.mp4
ffmpeg -i flipable.gif -pix_fmt yuv420p10le -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv420p10le.mp4
ffmpeg -i flipable.gif -pix_fmt yuv444p10 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv444p10.mp4
ffmpeg -i flipable.gif -pix_fmt yuv422p10le -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_yuv422p10le.mp4
ffmpeg -i flipable.gif -pix_fmt gray -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_gray.mp4
ffmpeg -i flipable.gif -pix_fmt gray10le -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264 -r 5 pixel_formats/supported/h264_gray10le.mp4

ffmpeg -i flipable.gif -pix_fmt bgr0 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264rgb -r 5 pixel_formats/supported/h264_bgr0.mp4
ffmpeg -i flipable.gif -pix_fmt bgr24 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264rgb -r 5 pixel_formats/supported/h264_bgr24.mp4
ffmpeg -i flipable.gif -pix_fmt rgb24 -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv -vcodec libx264rgb -r 5 pixel_formats/supported/h264_rgb24.mp4


