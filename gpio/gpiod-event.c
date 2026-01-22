// SPDX-License-Identifier: MIT
/*
 * GPIO example for the Microchip PolarFire SoC.
 *
 * Copyright (c) 2026 Microchip Technology Inc.
 */

#include <gpiod.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef CONSUMER
#define CONSUMER "Consumer"
#endif

int main(int argc, char **argv)
{
	int ret, count_event = 0, previous_value = 1;
	const char *chipname = "gpiochip0";
	unsigned int line_num;
	char chip_path[64];
	const int64_t timeout_ns = 1000000000LL; /* 1 second */

	struct gpiod_chip *chip = NULL;
	struct gpiod_line_settings *settings = NULL;
	struct gpiod_line_config *line_cfg = NULL;
	struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_request *request = NULL;
	struct gpiod_edge_event_buffer *evbuf = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <gpio pin>\n", argv[0]);
		return -1;
	}

	line_num = (unsigned int)atoi(argv[1]);
	if (line_num > 31) {
		fprintf(stderr, "Max <gpio pin> 31\n");
		return -1;
	}

	snprintf(chip_path, sizeof(chip_path), "/dev/%s", chipname);
	chip = gpiod_chip_open(chip_path);
	if (!chip) {
		perror("Failed to open GPIO chip\n");
		ret = -1;
		goto end;
	}

 	settings = gpiod_line_settings_new();
	if (!settings) {
		perror("Failed to allocate GPIO line settings\n");
		ret = -1;
		goto close_chip;
	}
	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_RISING);

	line_cfg = gpiod_line_config_new();
	if (!line_cfg) {
		perror("Failed to allocate GPIO line configuration\n");
		ret = -1;
		goto free_settings;
	}

	ret = gpiod_line_config_add_line_settings(line_cfg, &line_num, 1, settings);
	if (ret < 0) {
		perror("Failed to apply line settings to requested GPIO line\n");
		goto free_line_cfg;
	}

	req_cfg = gpiod_request_config_new();
	if (!req_cfg) {
		perror("Failed to allocate GPIO request configuration\n");
		ret = -1;
		goto free_line_cfg;
	}
	gpiod_request_config_set_consumer(req_cfg, CONSUMER);

	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	if (!request) {
		perror("Failed to request GPIO line\n");
		ret = -1;
		goto free_req_cfg;
	}

	evbuf = gpiod_edge_event_buffer_new(16);
	if (!evbuf) {
		perror("Failed to allocate edge event buffer\n");
		ret = -1;
		goto release_request;
	}

	printf("Monitoring GPIO events for 20 seconds (Ctrl+C to terminate)\n\n");

	for (int i = 0; i < 20; i++) {
		ret = gpiod_line_request_wait_edge_events(request, timeout_ns);
		if (ret < 0) {
			perror("Failed while waiting for GPIO edge event\n");
			ret = -1;
			goto free_evbuf;
		} else if (ret == 0) {
			printf("No event notification received on line  #%u\n", line_num);
			continue;
		}

		ret = gpiod_line_request_read_edge_events(request, evbuf, 1);
		if (ret < 0) {
			perror("Failed to read GPIO edge event\n");
			ret = -1;
			goto free_evbuf;
		}

	if (ret > 0) {
		for (int j = 0; j < ret; j++) {
			int val = gpiod_line_request_get_value(request, line_num);

			if (val < 0)
				continue;

			if (val == 0 && previous_value != 0) {
				count_event += ret;
				printf("Got event notification on line #%u %d times \n",
						line_num, count_event);
			}

			previous_value = val;
		}
	}

	sleep(1);
	}

	ret = 0;

free_evbuf:
	gpiod_edge_event_buffer_free(evbuf);
release_request:
	gpiod_line_request_release(request);
free_req_cfg:
	gpiod_request_config_free(req_cfg);
free_line_cfg:
	gpiod_line_config_free(line_cfg);
free_settings:
	gpiod_line_settings_free(settings);
close_chip:
	gpiod_chip_close(chip);
end:
	return ret;
}