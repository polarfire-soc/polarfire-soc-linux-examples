// SPDX-License-Identifier: MIT
/*
 * GPIO example for the Microchip PolarFire SoC.
 *
 * Copyright (c) 2026 Microchip Technology Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <gpiod.h>

static int read_gpio_state(struct gpiod_chip *chip, unsigned int line_num)
{
	int counter = 0, value = -1, previous_value = -1;
	struct gpiod_line_settings *settings = NULL;
	struct gpiod_line_config *line_cfg = NULL;
	struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_request *gpio_req = NULL;

	settings = gpiod_line_settings_new();
	if (!settings) {
		perror("Failed to initialize GPIO line settings\n");
		return -1;
	}

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

	line_cfg = gpiod_line_config_new();
	if (!line_cfg) {
		perror("Failed to allocate GPIO line configuration\n");
		goto free_settings;
	}

	gpiod_line_config_add_line_settings(line_cfg, &line_num, 1, settings);

	req_cfg = gpiod_request_config_new();
	if (!req_cfg) {
		perror("Failed to set up GPIO request configuration\n");
		goto free_line_cfg;
	}

	gpiod_request_config_set_consumer(req_cfg, "gpio-input");

	gpio_req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	if (!gpio_req) {
		printf("\n\tFailed to request GPIO# %d\n", line_num);
		goto free_req_cfg;
	}

	printf("\nReading GPIO state for 20 seconds\n");
	counter = 20;

	while (counter--) {
		sleep(1);

		value = gpiod_line_request_get_value(gpio_req, line_num);
		if (value < 0) {
			printf("\n\tError reading GPIO# %d\n", line_num);
			continue;
		}

		if (value != previous_value)
			printf("\n\tGPIO state: %d\n", value);

		previous_value = value;
	}

	gpiod_line_request_release(gpio_req);

free_req_cfg:
	gpiod_request_config_free(req_cfg);
free_line_cfg:
	gpiod_line_config_free(line_cfg);
free_settings:
	gpiod_line_settings_free(settings);

	return 0;
}

int main(int argc, char **argv)
{
	char runcmd;
	static const char *const chip_path = "/dev/gpiochip0";
	struct gpiod_chip *chip;
	unsigned int line_num;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <gpio pin>\n", argv[0]);
		return -1;
	}

	line_num = (unsigned int)atoi(argv[1]);
	if (line_num > 31) {
		fprintf(stderr, "Max <gpio pin> 31\n");
		return -1;
	}

	chip = gpiod_chip_open(chip_path);
	if (!chip) {
		printf("Failed to open GPIO chip\n");
		return EXIT_FAILURE;
	}

	while (1) {
		printf("\nPress 1 to read GPIO state\n"
		       "Press any other key to exit\n");

		scanf("%c%*c", &runcmd);

		if (runcmd == '1') {
			read_gpio_state(chip, line_num);
		} else {
			printf("\n\tExiting GPIO example\n");
			break;
		}
	}

	gpiod_chip_close(chip);
	return 0;
}