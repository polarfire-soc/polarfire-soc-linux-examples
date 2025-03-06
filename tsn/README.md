# Configuring CoreTSN Device from Linux User Application

This repository contains a command line interface example application that can be used to
configure Microchip CoreTSN device.
This CLI Application uses an API Library to configure the device.

Another application microchip-tsn-replace-tsnbasetime is useful in modifying the
basetime parameter of the configuration file based on current PTP time

## CLI Userguide

### Display List of TSN devices
microchip-tsn-cli --show-devices

### Set configuration to a TSN Device
microchip-tsn-cli --device=< devid > --set=config-type

| config-type  | Description                         |
| ------------ | ----------------------------------- |
| qbuconf      | Set Configuration for IEEE 802.1Qbv |
| qciconf      | Set Configuration for IEEE 802.1Qbu |
| qbvconf      | Set Configuration for IEEE 802.1Qbv |


### Get configuration to a TSN Device
microchip-tsn-cli --device=< devid > --set=config-type

| config-type  | Description                         |
| ------------ | ----------------------------------- |
| qbuconf      | Get Configuration for IEEE 802.1Qbv |
| qciconf      | Get Configuration for IEEE 802.1Qbu |
| qbvconf      | Get Configuration for IEEE 802.1Qbv |


### Other command line options

| option             | Description                                                           |
| ------------------ | --------------------------------------------------------------------- |
| --json             | print configuration in JSON format                                    |
| --conf-file=< filename >        | Set Configuration from config file which is in JSON format |
| --export-conf-file=< filename > | Get Configuration and save in config file in JSON format |
| --edit-mode        | Edit few parameters of configuration which are already set with other parameters unchanged |
| --show-gcl-csv     | show GCL(Gate Control List) for QBV config in CSV  format |
| --show-gcl-rct     | show Remaining cycle time during entering GCL |


## API Reference

### `int microchip_tsn_get_device_list(struct microchip_tsn_device *dev, int max_devices);`

**Description:**
Retrieves a list of TSN devices.

**Parameters:**
- `dev`: Pointer to a structure where the device list will be stored.
- `max_devices`: Maximum number of devices to retrieve.

**Return Value:**
- Returns the number of devices retrieved on success.
- Returns a negative value on failure.

### `int microchip_tsn_device_get_qbv_conf(struct microchip_tsn_device *dev, struct qbv_conf *conf);`

**Description:**
Gets the QBV (Scheduled Traffic) configuration of a TSN device.

**Parameters:**
- `dev`: Pointer to the TSN device structure.
- `conf`: Pointer to a structure where the QBV configuration will be stored.

**Return Value:**
- Returns 0 on success.
- Returns a negative value on failure.

### `int microchip_tsn_device_set_qbv_conf(struct microchip_tsn_device *dev, struct qbv_conf *conf);`

**Description:**
Sets the QBV (Scheduled Traffic) configuration of a TSN device.

**Parameters:**
- `dev`: Pointer to the TSN device structure.
- `conf`: Pointer to the structure containing the QBV configuration to be set.

**Return Value:**
- Returns 0 on success.
- Returns a negative value on failure.

### `int microchip_tsn_device_get_qbu_conf(struct microchip_tsn_device *dev, struct qbu_conf *conf);`

**Description:**
Gets the QBU (Frame Preemption) configuration of a TSN device.

**Parameters:**
- `dev`: Pointer to the TSN device structure.
- `conf`: Pointer to a structure where the QBU configuration will be stored.

**Return Value:**
- Returns 0 on success.
- Returns a negative value on failure.

### `int microchip_tsn_device_set_qbu_conf(struct microchip_tsn_device *dev, struct qbu_conf *conf);`

**Description:**
Sets the QBU (Frame Preemption) configuration of a TSN device.

**Parameters:**
- `dev`: Pointer to the TSN device structure.
- `conf`: Pointer to the structure containing the QBU configuration to be set.

**Return Value:**
- Returns 0 on success.
- Returns a negative value on failure.

### `int microchip_tsn_device_get_qci_conf(struct microchip_tsn_device *dev, struct qci_conf *conf);`

**Description:**
Gets the QCI (Per-Stream Filtering and Policing) configuration of a TSN device.

**Parameters:**
- `dev`: Pointer to the TSN device structure.
- `conf`: Pointer to a structure where the QCI configuration will be stored.

**Return Value:**
- Returns 0 on success.
- Returns a negative value on failure.

### `int microchip_tsn_device_set_qci_conf(struct microchip_tsn_device *dev, struct qci_conf *conf);`

**Description:**
Sets the QCI (Per-Stream Filtering and Policing) configuration of a TSN device.

**Parameters:**
- `dev`: Pointer to the TSN device structure.
- `conf`: Pointer to the structure containing the QCI configuration to be set.

**Return Value:**
- Returns 0 on success.
- Returns a negative value on failure.


## Replace basetime in TSN config file
### Command format
microchip-tsn-replace-tsngentime --ptpfile=/dev/< ptpfile > --addtimesec=< addsecs > --addtimensec=< addnsec > --infile=inputfile --outfile=outputfile [--secroundoff]

| option             | Description                                                                |
| ------------------ | -------------------------------------------------------------------------- |
| --ptpfile          | Path to ptp dev node |
| --addtimesec       | Seconds to add from current PTP TIME |
| --addtimensec      | Nano Seconds to add from current PTP TIME after adding seconds |
| --infile           | TSN Config file to which basetime needs to be changed |
| --outfile          | TSN Config file to save with new basetime |
| --secroundoff      | After adding seconds and nanoseconds to current time, set nanoseconds to 0 |



### Example without seconds roundoff
microchip-tsn-replace-tsngentime --ptpfile=/dev/ptp0 --addtimesec=5 --addtimensec=0 --infile=gen.json --outfile=gen2.json

### Example with seconds roundoff
microchip-tsn-replace-tsngentime --ptpfile=/dev/ptp0 --addtimesec=5 --addtimensec=0 --infile=gen.json --outfile=gen2.json --secroundoff

### Example with seconds roundoff updating same config file
microchip-tsn-replace-tsngentime --ptpfile=/dev/ptp0 --addtimesec=5 --addtimensec=0 --infile=gen.json --outfile=gen.json --secroundoff

## Hardware Requirements

PolarFire SoC Video Kit (MPFS250TS)
FMC Daughterboard (FMC_SGMII_DB)
