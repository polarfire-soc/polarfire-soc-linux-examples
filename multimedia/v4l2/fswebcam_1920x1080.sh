
media-ctl -v -V '"imx334 0-001a":0 [fmt:SRGGB10_1X10/1920x1080 field:none colorspace:srgb xfer:none]' -d /dev/media0
media-ctl -v -V '"60001000.csi2rx":1 [fmt:SRGGB10_1X10/1920x1080 field:none colorspace:srgb xfer:none]' -d /dev/media0

v4l2-ctl -d /dev/v4l-subdev1 --set-ctrl=analogue_gain=80
v4l2-ctl -d /dev/v4l-subdev1 --set-ctrl=vertical_blanking=1170

DATE=$(date +"%Y-%m-%d_%H%M")
fswebcam -r 1920x1080 --no-banner ~/$DATE.jpg

