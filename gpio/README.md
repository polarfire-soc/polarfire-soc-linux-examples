# PolarFire SoC GPIO Examples

## GPIO Test applications on the PolarFire SoC

These applications demonstrate example userspace GPIO access on Microchip PolarFire SoC platforms.

### GPIO Application on PolarFire SoC Icicle Kit

The switches (SW2 / SW3) are connected to GPIO#30/31 and are used to verify
the state of the pin in the 'gpiod-test' application and also the status change
of the pins in the 'gpiod-event' application.

See the [Icicle Kit Reference Design][1] for the GPIO mapping.

[1]: https://github.com/polarfire-soc/icicle-kit-reference-design

### 1. Read GPIO State (`gpiod-test`)

This application reads and prints the current GPIO value.

Run the application using:

```bash
./gpiod-test <gpio-pin>
```

```text
root@mpfs-icicle-kit:/opt/microchip/gpio# ./gpiod-test <gpio-pin>
        Press 1 to read GPIO state
        Press any other key to exit
```

### 2. Monitor GPIO Events (`gpiod-event`)

The application monitors GPIO events and prints the event count every second.
If no event occurs, a "no event" message is displayed.

```bash
./gpiod-event <gpio-pin>
```

```text
root@mpfs-icicle-kit:/opt/microchip/gpio# ./gpiod-event <gpio-pin>
        Monitoring GPIO events for 20 seconds (Ctrl+C to terminate)

        No event notification received on line  #<gpio-pin>
        No event notification received on line  #<gpio-pin>
        No event notification received on line  #<gpio-pin>
        No event notification received on line  #<gpio-pin>
        Got event notification on line #<gpio-pin> 1 times
        Got event notification on line #<gpio-pin> 2 times
```
