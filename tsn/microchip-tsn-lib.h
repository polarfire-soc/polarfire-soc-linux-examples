/* SPDX-License-Identifier: MIT */
/**
 * Microchip CoreTSN API Library
 *
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * Author: Pallela Venkat Karthik <pallela.karthik@microchip.com>
 *
 */

#ifndef _MICROCHIP_TSN_LIB_H_
#define _MICROCHIP_TSN_LIB_H_

#include <stdint.h>
#include <endian.h>
#include <string.h>
#include <linux/types.h>

#define ETHER_ADDR_LEN			6
#define MAX_TSN_CONFIG_SIZE		4096
#define MICROCHIP_TSN_NUM_PRIO_QUEUES	8
#define MCHP_TSN_CDEV_PATH_PREFIX	"/dev/mchpcoretsn"

struct microchip_tsn_device {
	__u64 tsn_dev_id;
};

struct gcl_entry {
	__u32 time_interval;
	__u8  gate_state;
};

struct qbv_conf {
	__u8 initial_gate_state;
	__u8 priority_enable;
	__u8 gate_enable;
	__u8 priority_queue_enable;
	__u8 priority_queue_prios[MICROCHIP_TSN_NUM_PRIO_QUEUES];
	__u8 control_list_length;
	__u32 cycle_time;
	__u64 basetime_sec;
	__u32 basetime_nsec;
	__u8 basetime_adjust;
	struct gcl_entry gcle[];

};

struct qbu_conf {
	__u8 pre_empt_en;
	__u16 pre_empt_size;
};

struct qci_conf {
	__u8 da_check;
	__u8 sa_check;
	__u8 destination_mac_addr[ETHER_ADDR_LEN];
	__u8 source_mac_addr[ETHER_ADDR_LEN];
};

struct misc_rx_port_id_conf {
	__u8 port_id_rx_check;
	__u16 port_id_rx;
};

struct misc_ptp_tx_prioq_conf {
	__u8 ptp_tx_prioq;
};

struct misc_length_deduct_byte_conf {
	__u16 crc_deduct_len;
};

int microchip_tsn_get_device_list(struct microchip_tsn_device *dev, int max_devices);
int microchip_tsn_device_get_qbv_conf(struct microchip_tsn_device *dev, struct qbv_conf *conf);
int microchip_tsn_device_set_qbv_conf(struct microchip_tsn_device *dev, struct qbv_conf *conf);
int microchip_tsn_device_get_qbu_conf(struct microchip_tsn_device *dev, struct qbu_conf *conf);
int microchip_tsn_device_set_qbu_conf(struct microchip_tsn_device *dev, struct qbu_conf *conf);
int microchip_tsn_device_get_qci_conf(struct microchip_tsn_device *dev, struct qci_conf *conf);
int microchip_tsn_device_set_qci_conf(struct microchip_tsn_device *dev, struct qci_conf *conf);
int microchip_tsn_misc_get_rx_port_id(struct microchip_tsn_device *dev,
				      struct misc_rx_port_id_conf *conf);
int microchip_tsn_misc_set_rx_port_id(struct microchip_tsn_device *dev,
				      struct misc_rx_port_id_conf *conf);
int microchip_tsn_misc_get_tx_ptp_prioq(struct microchip_tsn_device *dev,
					struct misc_ptp_tx_prioq_conf *conf);
int microchip_tsn_misc_set_tx_ptp_prioq(struct microchip_tsn_device *dev,
					struct misc_ptp_tx_prioq_conf *conf);
int microchip_tsn_misc_get_length_deduct_byte(struct microchip_tsn_device *dev,
					      struct misc_length_deduct_byte_conf *conf);
int microchip_tsn_misc_set_length_deduct_byte(struct microchip_tsn_device *dev,
					      struct misc_length_deduct_byte_conf *conf);
#endif
