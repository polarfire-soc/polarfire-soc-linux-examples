// SPDX-License-Identifier: MIT
/*
 *
 * Camera auto gain control and osd example application
 *
 *	Copyright (c) 2023 Microchip Inc.
 *
 */

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include  <linux/v4l2-controls.h>

#define MCHP_CID_OSD_NUM                (V4L2_CID_USER_BASE | 0x100B)
#define MCHP_CID_COMPRESSION_RATIO      (V4L2_CID_USER_BASE | 0x100A)
#define MCHP_CID_RGB_SUM                (V4L2_CID_USER_BASE | 0x100C)

#define MCHP_MAX_WIDTH			1920
#define MCHP_MAX_HEIGHT			1080

#define MCHP_GAIN_AVERAGE		125
#define MCHP_GAIN_MIN			5
#define MCHP_HYSTERESIS_GAIN		4

#define IOCTL_TRIES			3

static int xioctl(int fd, unsigned long request, void *arg)
{
	int r;
	int tries = IOCTL_TRIES;

	do {
		r = ioctl(fd, request, arg);
	} while (--tries > 0 && r == -1 && EINTR == errno);

	return r;
}

int set_camera_gain(int fd, int gain)
{
	struct v4l2_control ctrl = {0};

	ctrl.id = V4L2_CID_ANALOGUE_GAIN;
	ctrl.value = gain;
	if (-1 == xioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		perror("setting V4L2_CID_ANALOGUE_GAIN");
		return -1;
	}

	return gain;
}

int get_camera_gain(int fd)
{
	struct v4l2_control ctrl = {0};

	ctrl.id = V4L2_CID_ANALOGUE_GAIN;
	if (-1 == xioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		perror("setting V4L2_CID_ANALOGUE_GAIN");
		return -1;
	}

	return ctrl.value;
}

int get_intensity_average(int fd)
{
	struct v4l2_control ctrl = {0};

	ctrl.id = MCHP_CID_RGB_SUM;
	if (-1 == xioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		perror("getting V4L2_CID_ANALOGUE_GAIN");
		return -1;
	}
	if (-1 == xioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		perror("getting V4L2_CID_ANALOGUE_GAIN");
		return -1;
	}
	return ctrl.value;
}

int set_osd_compression_ratio(int fd, int compression_ratio)
{
	struct v4l2_control ctrl = {0};

	ctrl.id = MCHP_CID_OSD_NUM;
	ctrl.value = compression_ratio;
	if (-1 == xioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		perror("setting MCHP_CID_OSD_NUM");
		return -1;
	}

	return compression_ratio;
}

int get_frame_compression_ratio(int fd)
{
	struct v4l2_control ctrl = {0};

	ctrl.id = MCHP_CID_COMPRESSION_RATIO;
	if (-1 == xioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		perror("getting MCHP_CID_COMPRESSION_RATIO");
		return -1;
	}
	return ctrl.value;
}


static void mchp_dscmi_gain_cal(int fd, uint32_t total_sum)
{
	int div = MCHP_MAX_WIDTH * MCHP_MAX_HEIGHT * 2;
	uint32_t total_average;
	const uint16_t hs_threshold_high = (MCHP_GAIN_AVERAGE + MCHP_HYSTERESIS_GAIN);
	const uint16_t hs_threshold_low = (MCHP_GAIN_AVERAGE - MCHP_HYSTERESIS_GAIN);
	static uint16_t last_step;
	uint16_t step;
	uint16_t in_gain;

	total_average = total_sum / div;

	/*
	 * The total_average feed back value from fabric is less than threshold
	 * value then the gain will be step up by one, if the value is more than
	 * the threshold value then the gain will be step down by one.
	 */

	if (total_average < hs_threshold_low)
		step = 1;
	else
		if (total_average > hs_threshold_high)
			step = -1;
		else
			step = 0;

	if (!step)
		return;

	in_gain = get_camera_gain(fd);
	in_gain = in_gain + step;

	if (in_gain < MCHP_GAIN_MIN)
		in_gain = MCHP_GAIN_MIN;

	if (in_gain >= MCHP_GAIN_AVERAGE)
		in_gain = MCHP_GAIN_AVERAGE;

	if (last_step != step && step != 0)
		printf("average=%d in_gain=%d step=%d\n", total_average, in_gain, step);

	last_step = step;
	if (step != 0)
		set_camera_gain(fd, in_gain);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int video0, compression_ratio_loop, auto_gain_loop;
	uint32_t compression_ratio_osd;
	char *devname;

	if (argc != 4) {
		perror("Incorrect parameters passed:\n");
		perror("auto-enhance-osd /dev/video0 <auto_gain_loop> <compression_ratio_loop>\n");
		perror("ex: ./auto-enhance-osd /dev/video0 1 1\n");
		ret = -1;
		goto end;
	}

	devname = argv[1];
	auto_gain_loop = atoi(argv[2]);
	compression_ratio_loop = atoi(argv[3]);

	if (auto_gain_loop != 0 && auto_gain_loop != 1) {
		perror("auto_gain_loop 0 or 1\n");
		ret = -1;
		goto end;
	}

	if (compression_ratio_loop != 0 && compression_ratio_loop != 1) {
		perror("compression_ratio_loop 0 or 1\n");
		ret = -1;
		goto end;
	}

	if (compression_ratio_loop != 1 && auto_gain_loop != 1) {
		ret = -1;
		goto end;
	}

	video0 = open(devname, O_RDWR);
	if (video0 < 0) {
		fprintf(stderr, "cannot open %s: %s\n", devname, strerror(errno));
		ret = -1;
		goto end;
	} else {
		printf("opened %s (r,w)\n", devname);
	}

	while (1) {
		if (compression_ratio_loop) {
			ret = get_frame_compression_ratio(video0);
			compression_ratio_osd = ((ret % 10) |
						((ret / 10 % 10) << 4) |
						((ret / 100) << 8));

			ret = set_osd_compression_ratio(video0, compression_ratio_osd);
			if (ret < 0)
				return ret;
		}

		if (auto_gain_loop) {
			ret = get_intensity_average(video0);
			if (ret > 0)
				mchp_dscmi_gain_cal(video0, ret);
			else
				return ret;
		}

		usleep(100000);
	}

	close(video0);
end:
	return ret;
}

