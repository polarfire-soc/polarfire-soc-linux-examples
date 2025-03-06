/* SPDX-License-Identifier: MIT */
/*
 * @file print.h
 * @brief logging support functions
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 */

#ifndef __PRINT_H
#define __PRINT_H

#include <syslog.h>

void log_message(int level, const char *format, ...);

#define pr_err(x...)     log_message(LOG_ERR, x)
#define pr_warning(x...) log_message(LOG_WARNING, x)
#define pr_info(x...)    log_message(LOG_INFO, x)
#define pr_debug(x...)   log_message(LOG_DEBUG, x)

#endif /* __PRINT_H */

