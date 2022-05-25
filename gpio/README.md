# PolarFire SoC GPIO Examples

## GPIO Test applications on the PolarFire SoC Icicle Kit

These applications will read/write GPIO lines and also test interrupt processing on a single <user selected> gpio pin.

For more information on which GPIO's are used in the Libero design, please refer to [icicle-kit-reference-design](https://github.com/polarfire-soc/icicle-kit-reference-design).

### GPIO Connection Details
The User LEDs (LED 1, 2, 3 and 4) are connected to GPIO#16/17/18/19 and are used by the test application 'gpiod-test' to verify the LED's blink.

The switches (SW2 / SW3) are connected to GPIO#30/31 and are used to verify the state of the pin in the 'gpiod-test' application and also the status change of the pins in the 'gpiod-event' application.


### Running the Application:

The application 'gpiod-test' will blink the user LED's or read the GPIO#30 (SW2) value.
The application 'gpiod-event <gpio#>' will display events for the GPIO#.

```
root@icicle-kit-es:~# cd /opt/microchip/gpio
```

Type the `./gpiod-test` command and Press Enter to execute the application.

```
root@icicle-kit-es:/opt/microchip/gpio# ./gpiod-test
        # Choose one of  the following options:
        Enter 1 to blink LEDs for 20 seconds, LED's 1,2,3,4
        Enter 2 to verify Read SW2 value, connected to GPIO30"
        Press any key to exit
```

Type the `./gpiod-event <gpio-pin>` command and Press Enter to execute the application. As GPIO events are captured counts are displayed, or "no event" notifications if no GPIO event is detected in each 1 second window.
The example will run continuously until either 20 windows have elapsed or ctrl+c is pressed.

```
root@icicle-kit-es:/opt/microchip/gpio# ./gpiod-event <gpio-pin>
        No event notification received on line
        Got event notification on line #<gpio pin> x times
        No event notification received on line
        No event notification received on line
```
