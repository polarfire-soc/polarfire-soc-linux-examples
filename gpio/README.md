# PolarFire SoC GPIO Examples

## Blinking LEDs on the PolarFire SoC Icicle Kit

Using this application, User LEDs (LED 1, 2, 3 and 4) on the PolarFire SoC Icicle Kit are toggled.
For more information on which GPIO's are used in the Libero design, please refer to [icicle-kit-reference-design](https://github.com/polarfire-soc/icicle-kit-reference-design).

### Description:

The User LEDs (LED 1, 2, 3 and 4) are blinked by running a Linux application.

### Running the Application:

The led-blinky application (./led-blinky) is available in the `/opt/microchip/gpio` directory in the rootfs

```
root@icicle-kit-es:~# cd /opt/microchip/gpio
```

Type the `./led-blinky` command and Press Enter to execute the application.

```
root@icicle-kit-es:/opt/microchip/gpio# ./led-blinky                        
        # Choose one of  the following options:
        Enter 1 to blink LEDs 1, 2, 3 and 4
        Press 'Ctrl+c' to Exit
```

Enter 1 to blink LED 1, LED 2, LED 3, and LED 4 on the Icicle Kit

```
root@icicle-kit-es:/opt/microchip/gpio# ./led-blinky                      
        # Choose one of  the following options:
        Enter 1 to blink LEDs 1, 2, 3 and 4
        Press 'Ctrl+c' to Exit
1
         LEDs are blinking press 'Ctrl+c' to Exit
```
