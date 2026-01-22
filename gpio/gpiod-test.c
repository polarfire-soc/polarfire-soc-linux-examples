// SPDX-License-Identifier: MIT
/*
 * GPIOD example -- read SW2 switch state
 *
 * Copyright (c) 2021 Microchip Inc.
 */

#include <errno.h>
#include <fcntl.h>
#include <gpiod.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SW2_GPIO30	30

int main()
{
	char runcmd;
	char *chipname = "/dev/gpiochip0";
	struct gpiod_chip *chip;
	int counter = 0;

	chip = gpiod_chip_open(chipname);
	if (!chip) {
		perror("Open chip failed\n");
		goto end;
	}

	while (1) {
		printf("\n\t# Choose one of the following options:");
		printf("\n\tEnter '1' to read the state of SW2 connected to GPIO30");
		printf("\n\tEnter 'any key' to exit: ");

		scanf("%c%*c", &runcmd);

		if (runcmd == '1') {
			struct gpiod_line *sw2_30;
			int value = -1;
			int previous_value = -1;

			sw2_30 = gpiod_chip_get_line(chip, SW2_GPIO30);
			gpiod_line_request_input(sw2_30, "gpio30-sw2");

			printf("\n Loop for 20 seconds reading the state of SW2, "
			       "connected as an input to GPIO30\n");

			counter = 20;
			while (counter--) {
				sleep(1);

				value = gpiod_line_get_value(sw2_30);
				if (value == -1) {
					printf("\n\tError reading GPIO value: GPIO# %d\n",
					       SW2_GPIO30);
				} else {
					if (value != previous_value)
						printf("\n\tSW2 switch value is now: %d\n",
						       value);
					previous_value = value;
				}
			}

			gpiod_line_release(sw2_30);
		} else {
			printf("\n\tExiting GPIO example program...\n");
			goto close_chip;
		}
	}

close_chip:
	gpiod_chip_close(chip);
end:
	return 0;
}
