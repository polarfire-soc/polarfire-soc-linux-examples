// SPDX-License-Identifier: MIT
/*
 * @file print.c
 * @brief logging with different log levels
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#include "print.h"

#define LOG_TO_CONSOLE 1
#define LOG_TO_SYSLOG 2

static int log_destination = LOG_TO_CONSOLE;
static int log_level = LOG_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Get the current timestamp.
 *
 * This function retrieves the current time using the CLOCK_MONOTONIC clock
 * and formats it into a string with millisecond precision.
 *
 * @param buffer The buffer to store the formatted timestamp.
 * @param buffer_size The size of the buffer.
 */
void get_timestamp(char *buffer, size_t buffer_size)
{
	struct timespec tv;

	if (clock_gettime(CLOCK_MONOTONIC, &tv) != 0) {
		snprintf(buffer, buffer_size, "error");
		return;
	}
	snprintf(buffer, buffer_size, "%ld.%03ld", tv.tv_sec, tv.tv_nsec / 1000000);
}

/**
 * @brief Log a message with a specified log level.
 *
 * This function logs a message to the specified log destination (console or syslog)
 * with a timestamp. The log message is formatted using printf-style formatting.
 *
 * @param level The log level of the message.
 * @param format The format string for the log message.
 * @param ... Additional arguments for the format string.
 */
void log_message(int level, const char *format, ...)
{
	char log_buffer[1024];
	char timestamp[32];
	va_list args;

	// Check if the message log level is higher than the current log level
	if (level > log_level)
		return;

	// Get the current timestamp
	get_timestamp(timestamp, sizeof(timestamp));

	// Format the log message
	va_start(args, format);
	vsnprintf(log_buffer, sizeof(log_buffer), format, args);
	va_end(args);

	// Lock the mutex to ensure thread-safe logging
	pthread_mutex_lock(&log_mutex);
	if (log_destination == LOG_TO_CONSOLE)
		printf("[%s] %s\n", timestamp, log_buffer); // Log to console
	else
		syslog(level, "[%s] %s", timestamp, log_buffer); //// Log to syslog
	pthread_mutex_unlock(&log_mutex); // Unlock the mutex
}
