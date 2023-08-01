# PolarFire SoC Video Kit Frame Capture Examples Scripts

These example scripts show how to capture frames in raw Bayer format and stream them to a file in raw or jpg format.

## Bayer Pipeline

The bayer pipeline consists of the following IP's (exposing the nodes from the corresponding drivers):

`Imx334 camera (SRGGB10 format /dev/v4l-subdev0)` --> `MIPI CSI2 IP (serial data to parallel data /dev/v4l-subdev1)` --> `Video DMA IP (/dev/video0)`

## Prerequisites

- Make sure that the Bayer pipeline design is programmed to the PolarFire SoC Video Kit. A FlashPro Express programming job file can be found in the Video Kit reference design [ref link](https://mi-v-ecosystem.github.io/redirects/releases-video-kit-reference-design).

- The examples expect following nodes to be present at runtime:
  - Video device nodes:
    - /dev/video0,
    - /dev/v4l-subdev0,
    - /dev/v4l-subdev1,
  - Media device nodes:
    - /dev/media0.

Where /dev/video0 is responsible for capturing the images and /dev/media0 for changing the resolution and format.

## Command to load bayer format pipeline

Stop at U-boot prompt and run the below command to load the bayer pipeline

```sh
load mmc 0:1 ${scriptaddr} fitImage;
bootm start ${scriptaddr}#conf-microchip_mpfs-video-kit.dtb#conf-mpfs_video_bayer.dtbo;
bootm loados ${scriptaddr};
bootm ramdisk;
bootm prep;
fdt set /soc/ethernet@20112000 mac-address ${videokit_mac_addr0};
fdt set /soc/ethernet@20110000 mac-address ${videokit_mac_addr0};
bootm go;
```

## Command to capture JPG image with 1280x720 resolution

```sh
/opt/microchip/multimedia/v4l2/fswebcam_1280x720.sh
```

A successful execution should look like:

```text
root@mpfs-video-kit:~# /opt/microchip/multimedia/v4l2/fswebcam_1280x720.sh
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB10_1X10 1280x720 on pad imx334 0-001a/0
Format set: SRGGB10_1X10 1280x720
Setting up format SRGGB10_1X10 1280x720 on pad 60001000.csi2rx/0
Format set: SRGGB10_1X10 1280x720
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB8_1X8 1280x720 on pad 60001000.csi2rx/1
Format set: SRGGB8_1X8 1280x720
--- Opening /dev/video0...
Trying source module v4l2...
/dev/video0 opened.
No input was specified, using the first.
--- Capturing frame...
Captured frame in 0.00 seconds.
--- Processing captured image...
Disabling banner.
Writing JPEG image to '/home/root/2022-04-29_0039.jpg'.
root@mpfs-video-kit:~#

```

## Command to capture JPG image with 1920x1080 resolution

```sh
/opt/microchip/multimedia/v4l2/fswebcam_1920x1080.sh
```

A successful execution should look like:

```text
root@mpfs-video-kit:~# /opt/microchip/multimedia/v4l2/fswebcam_1920x1080.sh
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB10_1X10 1920x1080 on pad imx334 0-001a/0
Format set: SRGGB10_1X10 1920x1080
Setting up format SRGGB10_1X10 1920x1080 on pad 60001000.csi2rx/0
Format set: SRGGB10_1X10 1920x1080
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB10_1X10 1920x1080 on pad 60001000.csi2rx/1
Format set: SRGGB8_1X8 1920x1080
--- Opening /dev/video0...
Trying source module v4l2...
/dev/video0 opened.
No input was specified, using the first.
--- Capturing frame...
Captured frame in 0.00 seconds.
--- Processing captured image...
Disabling banner.
Writing JPEG image to '/home/root/2022-04-29_0041.jpg'.
root@mpfs-video-kit:~#

```

## Save 10 raw frames from the camera with 1280x720 resolution

```sh
/opt/microchip/multimedia/v4l2/imx334_1280x720.sh
```

A successful execution should look like:

```text
root@mpfs-video-kit:~# /opt/microchip/multimedia/v4l2/imx334_1280x720.sh
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB10_1X10 1280x720 on pad imx334 0-001a/0
Format set: SRGGB10_1X10 1280x720
Setting up format SRGGB10_1X10 1280x720 on pad 60001000.csi2rx/0
Format set: SRGGB10_1X10 1280x720
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB8_1X8 1280x720 on pad 60001000.csi2rx/1
Format set: SRGGB8_1X8 1280x720
<<<<<<<<<<<<<<<<<<<<
root@mpfs-video-kit:~#
```

## Save 10 raw frames from the camera with 1920x1080 resolution

```sh
/opt/microchip/multimedia/v4l2/imx334_1920x1080.sh
```

A successful execution should look like:

```text
root@mpfs-video-kit:~# /opt/microchip/multimedia/v4l2/imx334_1920x1080.sh
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB10_1X10 1920x1080 on pad imx334 0-001a/0
Format set: SRGGB10_1X10 1920x1080
Setting up format SRGGB10_1X10 1920x1080 on pad 60001000.csi2rx/0
Format set: SRGGB10_1X10 1920x1080
Opening media device /dev/media0
Enumerating entities
looking up device: 81:0
looking up device: 81:1
looking up device: 81:2
Found 3 entities
Enumerating pads and links
Setting up format SRGGB10_1X10 1920x1080 on pad 60001000.csi2rx/1
Format set: SRGGB8_1X8 1920x1080
<<<<<<<<<<
root@mpfs-video-kit:~#

