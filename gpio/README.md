# PolarFire SoC GPIO Examples

## GPIO Test applications on the PolarFire SoC Icicle Kit

These applications will read/write GPIO lines and also test interrupt processing on a single <user selected> gpio pin.

For more information on which GPIO's are used in the Libero design, please refer to [icicle-kit-reference-design](https://github.com/polarfire-soc/icicle-kit-reference-design).

### Description:

The User LEDs (LED 1, 2, 3 and 4) are blinked and SW#2 value is displayed by running a Linux application.
On the Icicle-Kit reference SW#2 / SW#3 are connnected to GPIO#30/31 whihc can be used to verify GPIO events.

### Running the Application:

The application 'gpiod-test' application will flash the user LED's or read the SW2 value.
The application 'gpio-event <gpio#>' will display events for the GPIO#.


```
root@icicle-kit-es:~# cd /opt/microchip/gpio
```

Type the `./gpiod-test` command and Press Enter to execute the application.

```
root@icicle-kit-es:/opt/microchip/gpio# ./gpiod-test
        # Choose one of  the following options:
        Enter 1 to blink LEDs for 10 seconds, LED's 1,2,3,4
        Enter 2 to verify Read SW2 value, connected to GPIO30"
        Press any key to exit
```

Type the `./gpio-event <gpio-pin>` command and Press Enter to execute the application.

```
root@icicle-kit-es:/opt/microchip/gpio# ./gpio-event <gpio-pin>
        # As GPIO events are captured they are displayed, application will exit after 10 iterations.
        .
        .
        .
        .
        poll() GPIO <gpio-pin> interrupt occurred
        .
        .
        .
```
