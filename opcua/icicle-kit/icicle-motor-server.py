#!/usr/bin/env python3

import asyncio, logging
from asyncua import Server, ua
import sys, subprocess, os


async def disable_motor():
    # program the CS pin on Mikrobus connector to logic 0
    register_address = "0x20002214"
    register_value = (
        subprocess.check_output(["devmem2", register_address])
        .decode()
        .strip()
        .splitlines()[-1]
        .split()[5]
    )
    updated_value = register_value[:8] + "D" + register_value[9:]
    subprocess.call(["devmem2", register_address, "w", updated_value])


async def enable_motor():
    # program the CS pin on Mikrobus connector to logic 1
    register_address = "0x20002214"
    register_value = (
        subprocess.check_output(["devmem2", register_address])
        .decode()
        .strip()
        .splitlines()[-1]
        .split()[5]
    )
    updated_value = register_value[:8] + "E" + register_value[9:]
    subprocess.call(["devmem2", register_address, "w", updated_value])


async def set_pwm_duty_cycle(pwm_pin, duty_cycle):
    with open(f"/sys/class/pwm/{pwm_pin}/duty_cycle", "w") as duty_cycle_file:
        duty_cycle_file.write(str(duty_cycle))


async def set_pwm_rate(pwm_pin, rate):
    with open(f"/sys/class/pwm/{pwm_pin}/period", "w") as period_file:
        period_file.write(str(rate))


sleep_val = 0.1


async def main():
    logging.basicConfig(level=logging.WARNING)
    _logger = logging.getLogger(__name__)

    # Setup the server
    PORT = "4840"
    if sys.argv[1:]:
        PORT = sys.argv[1]
    server = Server()
    await server.init()
    server.set_security_policy([ua.SecurityPolicyType.NoSecurity])
    url = "opc.tcp://0.0.0.0:" + PORT
    server.set_endpoint(url)

    name = "Stepper Motor Control - OPC Server"
    addspace = await server.register_namespace(name)

    node = server.get_objects_node()
    motor_control_object = await node.add_object(addspace, "Stepper Motor Controls")

    motor_start = await motor_control_object.add_variable(
        addspace, "Stepper Motor Start (1111 - Start)", 0
    )
    motor_stop = await motor_control_object.add_variable(
        addspace, "Stepper Motor Stop (1111 - Stop)", 0
    )
    motor_dir = await motor_control_object.add_variable(
        addspace, "Stepper Motor Direction (0-Clockwise, 1-Anti Clockwise)", 0
    )
    motor_enable = await motor_control_object.add_variable(
        addspace, "Stepper Motor Enable (1111 - Enable)", 0
    )
    motor_disable = await motor_control_object.add_variable(
        addspace, "Stepper Motor Disable (1111 - Disable)", 0
    )
    motor_reset = await motor_control_object.add_variable(
        addspace, "Stepper Motor Reset", 0
    )
    motor_steps = await motor_control_object.add_variable(
        addspace, "Stepper Motor Steps (200|400|800)", 200
    )
    motor_speed = await motor_control_object.add_variable(
        addspace, "Stepper Motor Speed (1-Full, 2-Half, 3-Quarter, 0-Standby)", 3
    )
    command_control = await motor_control_object.add_variable(
        addspace, "Motor Command Control (1111 - Updates Speed/Direction)", 0000
    )

    await motor_start.set_writable()
    await motor_stop.set_writable()
    await motor_dir.set_writable()
    await motor_enable.set_writable()
    await motor_disable.set_writable()
    await motor_reset.set_writable()
    await motor_steps.set_writable()
    await motor_speed.set_writable()
    await command_control.set_writable()

    _logger.info("Starting server!")
    print("Stepper Motor - OPC-UA Server Started")

    # valid values
    valid_steps = [200, 400, 800]
    max_dir = 1
    max_speed = 3

    # default values
    default_steps = 200
    default_dir = 0
    default_speed = 0

    async with server:
        try:
            while True:
                start = await motor_start.get_value()
                stop = await motor_stop.get_value()
                dir = await motor_dir.get_value()
                enable = await motor_enable.get_value()
                disable = await motor_disable.get_value()
                reset = await motor_reset.get_value()
                steps = await motor_steps.get_value()
                speed = await motor_speed.get_value()
                command_control_value = await command_control.get_value()

                # Check and correct steps value
                if steps not in valid_steps:
                    steps = default_steps
                    await motor_steps.set_value(default_steps)
                # Check and correct dir value
                if dir > max_dir:
                    dir = default_dir
                    await motor_dir.set_value(default_dir)
                # Check and correct speed value
                if speed > max_speed:
                    speed = default_speed
                    await motor_speed.set_value(default_speed)

                if command_control_value == 1111:
                    global sleep_val
                    # Configure the speed and the steps
                    if speed == 1:
                        print("Configuring the Stepper Motor Speed - Full")
                        subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xfd"])
                        # Dictionary to map/use steps to sleep_val
                        sleep_values = {200: 0.05, 400: 0.1, 800: 0.2}
                        sleep_val = sleep_values.get(steps)
                    elif speed == 2:
                        print("Configuring the Stepper Motor Speed - Half")
                        subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xfb"])
                        # Dictionary to map/use steps to sleep_val
                        sleep_values = {200: 0.1, 400: 0.2, 800: 0.4}
                        sleep_val = sleep_values.get(steps)
                    elif speed == 3:
                        print("Configuring the Stepper Motor Speed - Quarter")
                        subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xff"])
                        # Dictionary to map/use steps to sleep_val
                        sleep_values = {200: 0.2, 400: 0.4, 800: 0.8}
                        sleep_val = sleep_values.get(steps)
                    else:
                        print("Configuring the Stepper Motor Speed - StandBy")
                        subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xf9"])

                    if dir == 0:
                        print("Configuring the Stepper Motor Direction - Clockwise...")
                    else:
                        print(
                            "Configuring the Stepper Motor Direction - Anti Clockwise..."
                        )

                    direction = "20=" + str(dir)
                    # set the direction (0 - Clockwise, 1 - Anti Clockwise)
                    subprocess.call(["gpioset", "gpiochip0", direction])
                    await command_control.set_value(0000)

                if start == 1111:
                    print("Starting the Motor")
                    if not os.path.exists("/sys/class/pwm/pwmchip0/pwm0"):
                        with open("/sys/class/pwm/pwmchip0/export", "w") as export_file:
                            export_file.write("0\n")
                    # Reset the motor
                    subprocess.call(["gpioset", "gpiochip0", "21=0"])
                    # set the GPIO expander's direction of I/O pins (port 1,2) as Output
                    subprocess.call(["i2cset", "-y", "0", "0x70", "0x3", "0xf9"])
                    await enable_motor()
                    pwm_pin = "pwmchip0/pwm0"
                    # Set rate to 1 ms second (1000000 nanoseconds)
                    await set_pwm_rate(pwm_pin, 1000000)
                    # Set duty cycle to 50% (500000 nanoseconds)
                    await set_pwm_duty_cycle(pwm_pin, 500000)
                    # Enable the channel
                    subprocess.run(
                        "echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable", shell=True
                    )
                    # Run the clock signal for configured steps
                    await asyncio.sleep(sleep_val)
                    # Disable the channel
                    subprocess.run(
                        "echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable", shell=True
                    )
                    await disable_motor()
                    # set the GPIO expander's direction of I/O pins (port 1,2) as Input
                    subprocess.call(["i2cset", "-y", "0", "0x70", "0x3", "0xff"])
                    await motor_start.set_value(0)

                if stop == 1111:
                    print("Stopping the Motor")
                    # Reset the motor
                    subprocess.call(["gpioset", "gpiochip0", "21=0"])
                    await disable_motor()
                    await motor_stop.set_value(0)

                if enable == 1111:
                    print("Enabling the Motor")
                    await enable_motor()
                    await motor_enable.set_value(0)

                if disable == 1111:
                    print("Disabling the Motor")
                    await disable_motor()
                    await motor_disable.set_value(0)

                if reset == 1111:
                    print("Resetting the Motor")
                    # Reset the motor
                    subprocess.call(["gpioset", "gpiochip0", "21=0"])
                    await disable_motor()
                    await motor_reset.set_value(0)
                await asyncio.sleep(1)
        finally:
            # set the GPIO expander's direction of I/O pins (port 1,2) as Input
            subprocess.call(["i2cset", "-y", "0", "0x70", "0x3", "0xff"])
            # set the stepper motor speed to default i.e. Quarter Speed
            subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xff"])
            await disable_motor()
            await server.stop()
            print("OPC Server stopped")


if __name__ == "__main__":
    asyncio.run(main())
