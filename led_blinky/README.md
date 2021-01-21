# Blinking LEDs on the PolarFire SoC Icicle Kit

## Objective: 

Using this application, User LEDs (LED 1, 2, 3 and 4) on the PolarFire SoC Icicle Kit are toggled.
For more information on which GPIO's are used in the Libero design, please refer to [icicle-kit-reference-design](https://github.com/polarfire-soc/icicle-kit-reference-design).

## Description:

The User LEDs (LED 1, 2, 3 and 4) are blinked by running a Linux application.

## Hardware Requirements:

- PolarFire SoC Icicle Kit (MPFS250T-FCVG484EES)

## Pre-Requisite:

Before running this application, set up the Icicle kit and boot Linux as explained in [Updating PolarFire SoC Icicle-Kit FPGA Design and Linux Image](https://github.com/polarfire-soc/polarfire-soc-documentation/blob/master/boards/mpfs-icicle-kit-es/updating-icicle-kit/updating-icicle-kit-design-and-linux.md).

## Running the Application:

The led blinky application (./led_blinky) is available under /opt/microchip/blinky directory in rootfs

```
root@icicle-kit-es:~# cd /opt/microchip/blinky
```

Type the ./led_blinky command and Press Enter to execute the application.

```
root@icicle-kit-es:/opt/microchip/blinky# ./led_blinky                        
        # Choose one of  the following options:
        Enter 1 to blink LEDs 1, 2, 3 and 4
        Press 'Ctrl+c' to Exit
```

Enter 1 to blink LED 1, LED 2, LED 3, and LED 4 on the Icicle Kit

```
root@icicle-kit-es:/opt/microchip/blinky# ./led_blinky                      
        # Choose one of  the following options:
        Enter 1 to blink LEDs 1, 2, 3 and 4
        Press 'Ctrl+c' to Exit
1
         LEDs are blinking press 'Ctrl+c' to Exit
```
