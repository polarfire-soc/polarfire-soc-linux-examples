#!/usr/bin/env python3
import sys
from opcua import ua, Server
from pathlib import Path
from random import randint
import time, timeit
from datetime import datetime, timedelta
import os, subprocess

PORT = "4840"
if sys.argv[1:]:
    PORT = sys.argv[1]

server = Server()
url = "opc.tcp://0.0.0.0:" + PORT
server.set_endpoint(url)

name = "Stepper Motor Control - OPC Server"
addspace = server.register_namespace(name)

node = server.get_objects_node()

motor_control_object = node.add_object(addspace, "Stepper Motor Controls")
motor_start = motor_control_object.add_variable(
    addspace, "Stepper Motor Start (1111 - Start)", 0
)
motor_stop = motor_control_object.add_variable(
    addspace, "Stepper Motor Stop (1111 - Stop)", 0
)
motor_dir = motor_control_object.add_variable(
    addspace, "Stepper Motor Direction (0-Clockwise, 1-Anti Clockwise)", 0
)
motor_enable = motor_control_object.add_variable(
    addspace, "Stepper Motor Enable (1111 - Enable)", 0
)
motor_disable = motor_control_object.add_variable(
    addspace, "Stepper Motor Disable (1111 - Disable)", 0
)
motor_reset = motor_control_object.add_variable(addspace, "Stepper Motor Reset", 0)
motor_steps = motor_control_object.add_variable(addspace, "Stepper Motor Steps", 200)
motor_speed = motor_control_object.add_variable(
    addspace, "Stepper Motor Speed (1-Full, 2-Half, 3-Quarter, 0-Standby)", 3
)
command_control = motor_control_object.add_variable(
    addspace, "Motor Command Control (1111 - Updates Speed/Direction)", 0000
)

motor_start.set_writable()
motor_stop.set_writable()
motor_dir.set_writable()
motor_enable.set_writable()
motor_disable.set_writable()
motor_reset.set_writable()
motor_steps.set_writable()
motor_speed.set_writable()
command_control.set_writable()

server.start()
print("OPC Server started")


def disable_motor():
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


def enable_motor():
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


def set_pwm_duty_cycle(pwm_pin, duty_cycle):
    with open(f"/sys/class/pwm/{pwm_pin}/duty_cycle", "w") as duty_cycle_file:
        duty_cycle_file.write(str(duty_cycle))


def set_pwm_rate(pwm_pin, rate):
    with open(f"/sys/class/pwm/{pwm_pin}/period", "w") as period_file:
        period_file.write(str(rate))


def sleep_milliseconds(microseconds):
    time.sleep(microseconds / 1_000)


try:
    while True:
        start = motor_start.get_value()
        stop = motor_stop.get_value()
        command_control_value = command_control.get_value()
        dir = motor_dir.get_value()
        enable = motor_enable.get_value()
        disable = motor_disable.get_value()
        reset = motor_reset.get_value()
        steps = motor_steps.get_value()
        speed = motor_speed.get_value()

        if command_control_value == 1111:
            if speed > 3:
                speed = 0
                motor_speed.set_value(0)

            # set the speed
            if speed == 1:
                print("Configuring the Stepper Motor Speed - Full")
                subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xfd"])
            elif speed == 2:
                print("Configuring the Stepper Motor Speed - Half")
                subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xfb"])
            elif speed == 3:
                print("Configuring the Stepper Motor Speed - Quarter")
                subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xff"])
            else:
                print("Configuring the Stepper Motor Speed - StandBy")
                subprocess.call(["i2cset", "-y", "0", "0x70", "0x1", "0xf9"])

            if dir > 1:
                dir = 0
                motor_dir.set_value(0)

            if dir == 0:
                print("Configuring the Stepper Motor Direction - Clockwise...")
            else:
                print("Configuring the Stepper Motor Direction - Anti Clockwise...")

            direction = "20=" + str(dir)
            # set the direction (0 - Clockwise, 1 - Anti Clockwise)
            subprocess.call(["gpioset", "gpiochip0", direction])
            command_control.set_value(0000)

        if start == 1111:
            print("Starting the Motor")
            if not os.path.exists("/sys/class/pwm/pwmchip0/pwm0"):
                with open("/sys/class/pwm/pwmchip0/export", "w") as export_file:
                    export_file.write("0\n")
            # Reset the motor
            subprocess.call(["gpioset", "gpiochip0", "21=0"])
            # set the GPIO expander's direction of I/O pins (port 1,2) as Output
            subprocess.call(["i2cset", "-y", "0", "0x70", "0x3", "0xf9"])
            enable_motor()
            pwm_pin = "pwmchip0/pwm0"
            # Set rate to 1 ms second (1000000 nanoseconds)
            set_pwm_rate(pwm_pin, 1000000)
            # Set duty cycle to 50% (500000 nanoseconds)
            set_pwm_duty_cycle(pwm_pin, 500000)
            # Enable the channel
            subprocess.run("echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable", shell=True)
            # Run the clock signal for 200 ms
            sleep_milliseconds(200)
            # Disable the channel
            subprocess.run("echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable", shell=True)
            disable_motor()
            # set the GPIO expander's direction of I/O pins (port 1,2) as Input
            subprocess.call(["i2cset", "-y", "0", "0x70", "0x3", "0xff"])
            motor_start.set_value(0000)

        if stop == 1111:
            print("Stopping the Motor")
            # Reset the motor
            subprocess.call(["gpioset", "gpiochip0", "21=0"])
            disable_motor()
            motor_stop.set_value(0)

        if enable == 1111:
            print("Enabling the Motor")
            enable_motor()
            motor_enable.set_value(0)

        if disable == 1111:
            print("Disabling the Motor")
            disable_motor()
            motor_disable.set_value(0)

        if reset == 1111:
            print("Resetting the Motor")
            # Reset the motor
            subprocess.call(["gpioset", "gpiochip0", "21=0"])
            disable_motor()
            motor_reset.set_value(0)

        time.sleep(1)
finally:
    # set the GPIO expander's direction of I/O pins (port 1,2) as Input
    subprocess.call(["i2cset", "-y", "0", "0x70", "0x3", "0xff"])
    disable_motor()
    server.stop()
    print("OPC Server stopped")
