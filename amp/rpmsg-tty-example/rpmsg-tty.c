// SPDX-License-Identifier: MIT
/*
 * RPMsg TTY example for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sched.h>
#include <string.h>

#define DEFAULT_RPMSG_DEVICE "/dev/ttyRPMSG4"
#define RPMSG_MAX_SIZE		256

char *rpmsg_device = "";

void parse_cmd_line_args(int argc, char *argv[])
{
	int c;

	while ((c = getopt (argc, argv, "d")) != -1)
	{
		switch (c)
		{
			case 'd':
				rpmsg_device = argv[2];
			break;
			default:
				break;
		}
	}
}

void send_message_from_stdin(int fd)
{
	char str[RPMSG_MAX_SIZE];
	int len = 0;

	while(len <= 1)
	{
		fgets(str, RPMSG_MAX_SIZE, stdin);
		len = strlen(str);
	}

	str[strcspn(str, "\n")] = 0;

	write(fd, str, strlen(str));

	if(!strncmp(str,"quit", 4) && strlen(str) <= 5)
		exit(0);
}

int main(int argc, char **argv)
{
	int fd;
	struct termios tty;

	parse_cmd_line_args(argc, argv);

	if(strlen(rpmsg_device) == 0)
	{
		printf("Opening device %s...\n", DEFAULT_RPMSG_DEVICE);
		fd = open(DEFAULT_RPMSG_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
	}
	else
	{
		printf("Opening device %s...\n", rpmsg_device);
		fd = open(rpmsg_device, O_RDWR | O_NOCTTY | O_NDELAY);
	}

	if (fd < 0)
	{
		printf("Error, cannot open device\n");
		return 1;
	}

	tcgetattr(fd, &tty);              /* get current attributes */
	cfmakeraw(&tty);                  /* raw input */
	tty.c_cc[VMIN] = 0;               /* non blocking */
	tty.c_cc[VTIME] = 0;              /* non blocking */
	tcsetattr(fd, TCSANOW, &tty);     /* write attributes */
	printf("Device is open\n");

	printf("Enter message to send or type quit to quit :");

	for ( ;; )
	{
		send_message_from_stdin(fd);
	}

	return EXIT_SUCCESS;
}
