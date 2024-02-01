# PolarFire SoC Auto Enhance and OSD Example

auto-enhance-osd is a simple example to the video pipeline controls.
After starting the video pipeline this application will update the camera
analog gain according to the feedback value from the image enhancement driver.

It will also update the H.264 compression ratio from the video capture driver
to OSD (on screen display) driver.

Run the example program, first build it by running make:

```sh
make
```

Once built, it can be run (it will not work if the pipeline is not active):

```sh
./auto-enhance-osd /dev/video0 1 1
```

