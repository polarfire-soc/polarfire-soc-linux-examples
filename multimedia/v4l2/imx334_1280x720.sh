media-ctl -v -V '"imx334 0-001a":0 [fmt:SRGGB10_1X10/1280x720 field:none colorspace:srgb xfer:none]' -d /dev/media0
media-ctl -v -V '"60001000.csi2rx":1 [fmt:SRGGB8_1X8/1280x720 field:none colorspace:srgb xfer:none]' -d /dev/media0

v4l2-ctl -d /dev/v4l-subdev1 --set-ctrl=analogue_gain=80
v4l2-ctl -d /dev/v4l-subdev1 --set-ctrl=vertical_blanking=1170

v4l2-ctl --device /dev/video0 --set-fmt-video=width=1280,height=720,pixelformat=RGGB --stream-mmap  --stream-count=10 --stream-to=file.raw

