#!/usr/bin/env python3

import asyncio, logging
from asyncua import Server, ua
import sys, subprocess, os

load_spi_dev = "/opt/microchip/dt-overlays/overlay.sh mpfs_icicle_stepper7_click.dtbo"
spi_directory = "/boot"

print("Attempting to load the MCP23S08 SPI device.")

try:
    subprocess.check_call(load_spi_dev, shell=True, cwd=spi_directory)
    print("MCP23S08 SPI device loaded successfully.")
except subprocess.CalledProcessError as e:
    if e.returncode == 255:
        print("MCP23S08 SPI device overlay already exists. Continuing.")
    else:
        print(f"Failed to load the SPI device. Error: {e.returncode}")


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

    motor_dir = await motor_control_object.add_variable(
        addspace, "Stepper Motor Direction (0-Clockwise, 1-Anti Clockwise)", 0
    )

    motor_rotation = await motor_control_object.add_variable(
        addspace, "Stepper Motor Rotation (1-Full, 2-Half, 3-Quarter)", 1
    )

    motor_revolutions = await motor_control_object.add_variable(
        addspace, "Stepper Motor Revolutions", 0
    )

    await motor_start.set_writable()
    await motor_dir.set_writable()
    await motor_rotation.set_writable()
    await motor_revolutions.set_writable()

    _logger.info("Starting server!")
    print("Stepper Motor - OPC-UA Server Started")

    # valid values
    max_dir = 1
    max_rotation = 3
    max_revolutions = 100

    # default values
    default_dir = 0
    default_rotation = 1
    default_revolutions = 0

    async with server:
        try:
            while True:
                start = await motor_start.get_value()
                direction = await motor_dir.get_value()
                revolutions = await motor_revolutions.get_value()
                rotation = await motor_rotation.get_value()

                # Check and correct dir value
                if direction > max_dir:
                    direction = default_dir
                    await motor_dir.set_value(default_dir)

                # Check and correct revolution value
                if revolutions > max_revolutions:
                    revolutions = default_revolutions
                    await motor_revolutions.set_value(default_revolutions)

                # Check and correct rotation value
                if rotation > max_rotation:
                    rotation = default_rotation
                    await motor_rotation.set_value(default_rotation)

                if revolutions > 0:
                    steps = 50 * revolutions
                else:
                    steps = {3: 12, 2: 24, 1: 50}.get(rotation, 50 * default_rotation)

                if start == 1111:
                    print("Starting the Motor")
                    try:
                        # Set GPIO pin 21 high
                        subprocess.call(
                            [
                                "/opt/microchip/opcua/icicle-kit/opcua_run_motor.sh",
                                str(steps),
                                str(direction),
                            ]
                        )

                    except subprocess.CalledProcessError as e:
                        print(f"An error occurred: {e}")
                        break

                    await motor_rotation.set_value(default_rotation)
                    await motor_revolutions.set_value(default_revolutions)
                    await motor_start.set_value(0)

                await asyncio.sleep(1)
        finally:
            await server.stop()
            print("OPC Server stopped")


if __name__ == "__main__":
    asyncio.run(main())
