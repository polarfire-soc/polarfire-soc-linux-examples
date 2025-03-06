// SPDX-License-Identifier: MIT
/**
 * Microchip CoreTSN API Library
 *
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * Author: Pallela Venkat Karthik <pallela.karthik@microchip.com>
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include "microchip-tsn-lib.h"
#include "microchip-tsn-cmds.h"

#define MAX_DEVICES 32
#define CORETSN_DRIVER_SYSFS_PATH "/sys/bus/platform/drivers/microchip-coretsn"

static int get_device_list(__u64 *dev_id)
{
	DIR *dir;
	struct dirent *entry;
	const char *prefix = "mchpcoretsn";
	__u64 device_id;
	unsigned int count = 0;
	int ret;

	dir = opendir("/dev");
	if (!dir) {
		perror("opendir");
		return 1;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
			ret = sscanf(entry->d_name + strlen(prefix), "%llx", &device_id);
			//if (ret != 1)
			//	continue;
			*dev_id = device_id;
			//printf("%u %u\n",*dev_id, count);
			dev_id++;
			count++;
		}
	}

	closedir(dir);

	return count;
}

static int file_exists(const char *path)
{
	FILE *fp;

	fp = fopen(path, "r");
	if (fp) {
		fclose(fp);
		return 1;
	}

	return 0;
}

int microchip_tsn_get_device_list(struct microchip_tsn_device *dev, int max_devices)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_device_info *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret = 0;
	int i;
	int path_count;
	char tsn_cdev_path[32];
	__u64 dev_id[MAX_DEVICES];

	if (max_devices < 0 || max_devices > MAX_DEVICES)
		return -EINVAL;

	if (!file_exists(CORETSN_DRIVER_SYSFS_PATH))
		return -ENOENT;

	path_count = get_device_list(dev_id);

	for (i = 0; i < path_count ; i++) {
		dev[i].tsn_dev_id = dev_id[i];
		sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev_id[i]);
	}

	if (ret == 0)
		ret = path_count;

	return ret;
}

int microchip_tsn_device_get_qbv_conf(struct microchip_tsn_device *dev, struct qbv_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_qbv *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	int control_list_length;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_qbv);
	alloc_size += MAX_DEVICES * sizeof(struct mchp_tsn_gcl_entry);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_GET_QBV;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret) {
		ret = -errno;
	} else {
		tsn_conf = (struct mchp_tsn_config_qbv *)cmd->tsn_config_data;
		conf->initial_gate_state = tsn_conf->initial_gate_state;
		conf->priority_enable = tsn_conf->priority_enable;
		conf->gate_enable = tsn_conf->gate_enable;
		conf->priority_queue_enable = tsn_conf->priority_queue_enable;
		memcpy(conf->priority_queue_prios, tsn_conf->priority_queue_prios,
		       MCHP_TSN_NUM_PRIO_QUEUES);
		conf->control_list_length = tsn_conf->control_list_length;
		conf->cycle_time = be32toh(tsn_conf->cycle_time);
		conf->basetime_sec = be64toh(tsn_conf->basetime_sec);
		conf->basetime_nsec = be32toh(tsn_conf->basetime_nsec);
		conf->basetime_adjust = tsn_conf->basetime_adjust;

		control_list_length = conf->control_list_length;

		for (i = 0; i < control_list_length; i++) {
			conf->gcle[i].time_interval = be32toh(tsn_conf->gcle[i].time_interval);
			conf->gcle[i].gate_state = tsn_conf->gcle[i].gate_state;
		}
		ret = cmd->cmd_status;
	}

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_device_get_qbu_conf(struct microchip_tsn_device *dev, struct qbu_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_qbu *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_qbu);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_GET_QBU;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret) {
		ret = -errno;
	} else {
		tsn_conf = (struct mchp_tsn_config_qbu *)cmd->tsn_config_data;
		conf->pre_empt_en = tsn_conf->pre_empt_en;
		conf->pre_empt_size = be16toh(tsn_conf->pre_empt_size);
		ret = cmd->cmd_status;
	}

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_device_get_qci_conf(struct microchip_tsn_device *dev, struct qci_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_qci *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_qci);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_GET_QCI;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret) {
		ret = -errno;
	} else {
		tsn_conf = (struct mchp_tsn_config_qci *)cmd->tsn_config_data;
		conf->da_check = tsn_conf->da_check;
		conf->sa_check = tsn_conf->sa_check;
		memcpy(conf->destination_mac_addr, tsn_conf->destination_mac_addr, ETHER_ADDR_LEN);
		memcpy(conf->source_mac_addr, tsn_conf->source_mac_addr, ETHER_ADDR_LEN);
		ret = cmd->cmd_status;
	}

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_misc_get_rx_port_id(struct microchip_tsn_device *dev,
				      struct misc_rx_port_id_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_misc_rx_port_id *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_misc_rx_port_id);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_GET_MISC_RX_PORT;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret) {
		ret = -errno;
	} else {
		tsn_conf = (struct mchp_tsn_config_misc_rx_port_id *)cmd->tsn_config_data;
		conf->port_id_rx_check = tsn_conf->port_id_rx_check;
		conf->port_id_rx = be16toh(tsn_conf->port_id_rx);
		ret = cmd->cmd_status;
	}

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_misc_get_length_deduct_byte(struct microchip_tsn_device *dev,
					      struct misc_length_deduct_byte_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_misc_length_deduct_byte *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_misc_length_deduct_byte);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_GET_MISC_LENGTH_DEDUCT_BYTE;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret) {
		ret = -errno;
	} else {
		tsn_conf = (struct mchp_tsn_config_misc_length_deduct_byte *)cmd->tsn_config_data;
		conf->crc_deduct_len = be16toh(tsn_conf->crc_deduct_len);
		ret = cmd->cmd_status;
	}

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_misc_get_tx_ptp_prioq(struct microchip_tsn_device *dev,
					struct misc_ptp_tx_prioq_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_misc_ptp_tx_prioq *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_misc_ptp_tx_prioq);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_GET_MISC_PTP_TX_PRIOQ;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret) {
		ret = -errno;
	} else {
		tsn_conf = (struct mchp_tsn_config_misc_ptp_tx_prioq *)cmd->tsn_config_data;
		conf->ptp_tx_prioq = tsn_conf->ptp_tx_prioq;
		ret = cmd->cmd_status;
	}

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_device_set_qbv_conf(struct microchip_tsn_device *dev, struct qbv_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_qbv *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	int control_list_length;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_qbv);
	alloc_size += MAX_DEVICES * sizeof(struct mchp_tsn_gcl_entry);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_SET_QBV;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);
	cmd->tsn_config_size = htobe16(sizeof(struct mchp_tsn_config_qbv)
			       + MAX_DEVICES * sizeof(struct mchp_tsn_gcl_entry));

	tsn_conf = (struct mchp_tsn_config_qbv *)cmd->tsn_config_data;
	tsn_conf->initial_gate_state = conf->initial_gate_state;
	tsn_conf->priority_enable =  conf->priority_enable;
	tsn_conf->gate_enable =  conf->gate_enable;
	tsn_conf->priority_queue_enable = conf->priority_queue_enable;
	memcpy(tsn_conf->priority_queue_prios, conf->priority_queue_prios,
	       MCHP_TSN_NUM_PRIO_QUEUES);
	tsn_conf->control_list_length = conf->control_list_length;
	tsn_conf->cycle_time = htobe32(conf->cycle_time);
	tsn_conf->basetime_sec = htobe64(conf->basetime_sec);
	tsn_conf->basetime_nsec = htobe32(conf->basetime_nsec);
	tsn_conf->basetime_adjust = conf->basetime_adjust;

	control_list_length = conf->control_list_length;

	for (i = 0; i < control_list_length; i++) {
		tsn_conf->gcle[i].time_interval = htobe32(conf->gcle[i].time_interval);
		tsn_conf->gcle[i].gate_state = conf->gcle[i].gate_state;
	}

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);

	if (ret)
		ret = -errno;
	else
		ret = cmd->cmd_status;

	if (cmd->cmd_status_string_avail)
		printf("%s\n", cmd->cmd_status_string);

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_device_set_qbu_conf(struct microchip_tsn_device *dev, struct qbu_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_qbu *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_qbu);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_SET_QBU;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);
	cmd->tsn_config_size = htobe16(sizeof(struct mchp_tsn_config_qbu));

	tsn_conf = (struct mchp_tsn_config_qbu *)cmd->tsn_config_data;
	tsn_conf->pre_empt_en = conf->pre_empt_en;
	tsn_conf->pre_empt_size = htobe16(conf->pre_empt_size);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret)
		ret = -errno;
	else
		ret = cmd->cmd_status;

	if (cmd->cmd_status_string_avail)
		printf("%s\n", cmd->cmd_status_string);

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_device_set_qci_conf(struct microchip_tsn_device *dev, struct qci_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_qci *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_qci);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_SET_QCI;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);
	cmd->tsn_config_size = htobe16(sizeof(struct mchp_tsn_config_qci));

	tsn_conf = (struct mchp_tsn_config_qci *)cmd->tsn_config_data;
	tsn_conf->da_check = conf->da_check;
	tsn_conf->sa_check = conf->sa_check;
	memcpy(tsn_conf->destination_mac_addr, conf->destination_mac_addr, ETHER_ADDR_LEN);
	memcpy(tsn_conf->source_mac_addr, conf->source_mac_addr, ETHER_ADDR_LEN);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);

	if (ret)
		ret = -errno;
	else
		ret = cmd->cmd_status;

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_misc_set_rx_port_id(struct microchip_tsn_device *dev,
				      struct misc_rx_port_id_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_misc_rx_port_id *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_misc_rx_port_id);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_SET_MISC_RX_PORT;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);
	cmd->tsn_config_size = htobe16(sizeof(struct mchp_tsn_config_misc_rx_port_id));

	tsn_conf = (struct mchp_tsn_config_misc_rx_port_id *)cmd->tsn_config_data;
	tsn_conf->port_id_rx_check = conf->port_id_rx_check;
	tsn_conf->port_id_rx = htobe16(conf->port_id_rx);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret)
		ret = -errno;
	else
		ret = cmd->cmd_status;

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_misc_set_tx_ptp_prioq(struct microchip_tsn_device *dev,
					struct misc_ptp_tx_prioq_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_misc_ptp_tx_prioq *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_misc_ptp_tx_prioq);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_SET_MISC_PTP_TX_PRIOQ;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);
	cmd->tsn_config_size = htobe16(sizeof(struct mchp_tsn_config_misc_ptp_tx_prioq));

	tsn_conf = (struct mchp_tsn_config_misc_ptp_tx_prioq *)cmd->tsn_config_data;
	tsn_conf->ptp_tx_prioq = conf->ptp_tx_prioq;

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret)
		ret = -errno;
	else
		ret = cmd->cmd_status;

	free(cmd);
	close(tsn_fd);

	return ret;
}

int microchip_tsn_misc_set_length_deduct_byte(struct microchip_tsn_device *dev,
					      struct misc_length_deduct_byte_conf *conf)
{
	struct mchp_tsn_config_cmd_resp *cmd;
	struct mchp_tsn_config_misc_length_deduct_byte *tsn_conf;
	__u32 alloc_size;
	int tsn_fd;
	int ret;
	int i;
	char tsn_cdev_path[32];

	sprintf(tsn_cdev_path, "%s%llx", MCHP_TSN_CDEV_PATH_PREFIX, dev->tsn_dev_id);
	tsn_fd = open(tsn_cdev_path, O_RDWR, 0666);
	if (tsn_fd < 0)
		return -errno;

	alloc_size = sizeof(struct mchp_tsn_config_cmd_resp);
	alloc_size += sizeof(struct mchp_tsn_config_misc_length_deduct_byte);

	cmd = malloc(alloc_size);
	if (!cmd)
		return -ENOMEM;

	cmd->cmd = MCHP_TSN_SET_MISC_LENGTH_DEDUCT_BYTE;
	cmd->tsn_dev_id = htobe64(dev->tsn_dev_id);
	cmd->tsn_config_size = htobe16(sizeof(struct mchp_tsn_config_misc_length_deduct_byte));

	tsn_conf = (struct mchp_tsn_config_misc_length_deduct_byte *)cmd->tsn_config_data;
	tsn_conf->crc_deduct_len = htobe16(conf->crc_deduct_len);

	ret = ioctl(tsn_fd, MCHP_TSN_CONFIG_CMD, cmd);
	if (ret)
		ret = -errno;
	else
		ret = cmd->cmd_status;

	free(cmd);
	close(tsn_fd);

	return ret;
}
