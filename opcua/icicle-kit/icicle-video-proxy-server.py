#!/usr/bin/env python3

import asyncio, logging
from asyncua import Server, Client, ua
import sys, subprocess, os


async def main():
    logging.basicConfig(level=logging.WARNING)
    _logger = logging.getLogger(__name__)

    # Setup the server
    PORT = "4840"
    if sys.argv[1:]:
        PORT = sys.argv[1]
    # Create a new instance of the Server class.
    server = Server()
    # Initialize the server, setting up its initial configuration.
    await server.init()
    # Set the security policy for the server to NoSecurity, which means no security is applied.
    server.set_security_policy([ua.SecurityPolicyType.NoSecurity])
    # Define the URL for the server to listen on. Replace 'PORT' with the actual port number.
    url_proxy_server = "opc.tcp://0.0.0.0:" + PORT
    # Set the endpoint for the server, indicating where clients can connect to.
    server.set_endpoint(url_proxy_server)

    # Define a name for the server's namespace.
    name = "Video Proxy Server - OPC Server"
    # Register the namespace and get the corresponding namespace index.
    addspace = await server.register_namespace(name)

    # Get the objects node, which serves as the root node for object-related data.
    node = server.get_objects_node()
    camera_control_object = await node.add_object(addspace, "Camera Controls")

    # Define the variables to communicate with the server space.
    start_server = await camera_control_object.add_variable(
        addspace, "Streaming Start", 0000
    )
    stop_server = await camera_control_object.add_variable(
        addspace, "Streaming Stop", 0000
    )
    quality_factor_server = await camera_control_object.add_variable(
        addspace, "Camera - Qaulity Factor [25-50]", 30
    )
    brightness_server = await camera_control_object.add_variable(
        addspace, "Camera - Brightness Control [0-255]", 137
    )
    contrast_server = await camera_control_object.add_variable(
        addspace, "Camera - Contrast Control [0-255]", 154
    )
    red_gain_server = await camera_control_object.add_variable(
        addspace, "Camera - Red Gain [0-255]", 122
    )
    green_gain_server = await camera_control_object.add_variable(
        addspace, "Camera - Green Gain [0-255]", 102
    )
    blue_gain_server = await camera_control_object.add_variable(
        addspace, "Camera - Blue Gain [0-255]", 138
    )
    command_control_server = await camera_control_object.add_variable(
        addspace, "Vision Command Control", 0000
    )
    ip_address_server = await camera_control_object.add_variable(
        addspace, "IP Address", "192.168.1.1"
    )
    horizontal_resolution_server = await camera_control_object.add_variable(
        addspace, "Camera - Horizontal Resolution [432|640|960|1280|1920]", 1280
    )
    vertical_resolution_server = await camera_control_object.add_variable(
        addspace, "Camera - Vertical Resolution [240|480|544|720|1072]", 720
    )

    url_video_kit_opc_server = "opc.tcp://192.168.1.1:4840"
    await start_server.set_writable()
    await stop_server.set_writable()
    await command_control_server.set_writable()
    await ip_address_server.set_writable()
    await quality_factor_server.set_writable()
    await brightness_server.set_writable()
    await contrast_server.set_writable()
    await red_gain_server.set_writable()
    await green_gain_server.set_writable()
    await blue_gain_server.set_writable()
    await horizontal_resolution_server.set_writable()
    await vertical_resolution_server.set_writable()

    _logger.info("Starting server!")
    print("Video Proxy OPC-UA Server Started")

    # valid horizontal resolutions
    valid_horizontal_resolutions = [432, 640, 960, 1280, 1920]
    # default horizontal resolutions
    default_horizontal_resolution = 1280
    # valid vertical resolutions
    valid_vertical_resolutions = [240, 480, 544, 720, 1072]
    # default vertical resolutions
    default_vertical_resolution = 720

    async with server:
        client = Client(url_video_kit_opc_server)
        try:
            await client.connect()
            print("OPC Client connected to Video Kit - OPC Server")
            while True:
                # Server Values from UAExpert
                start_server_value = await start_server.get_value()
                stop_server_value = await stop_server.get_value()
                ip_address_server_value = await ip_address_server.get_value()
                command_control_server_value = await command_control_server.get_value()
                quality_factor_server_value = await quality_factor_server.get_value()
                if quality_factor_server_value < 25 or quality_factor_server_value > 50:
                    await quality_factor_server.set_value(30)
                brightness_server_value = await brightness_server.get_value()
                contrast_server_value = await contrast_server.get_value()
                red_gain_server_value = await red_gain_server.get_value()
                green_gain_server_value = await green_gain_server.get_value()
                blue_gain_server_value = await blue_gain_server.get_value()
                horizontal_resolution_server_value = (
                    await horizontal_resolution_server.get_value()
                )
                vertical_resolution_server_value = (
                    await vertical_resolution_server.get_value()
                )

                # Check and correct resolution value
                if (
                    horizontal_resolution_server_value
                    not in valid_horizontal_resolutions
                ):
                    horizontal_resolution_server_value = default_horizontal_resolution
                    await horizontal_resolution_server.set_value(
                        default_horizontal_resolution
                    )

                if vertical_resolution_server_value not in valid_vertical_resolutions:
                    vertical_resolution_server_value = default_vertical_resolution
                    await vertical_resolution_server.set_value(
                        default_vertical_resolution
                    )

                # Node Refereces between ICICLE Kit and Video Kit
                start_client = client.get_node("ns=2;i=2")
                stop_client = client.get_node("ns=2;i=3")
                quality_factor_client = client.get_node("ns=2;i=4")
                brightness_client = client.get_node("ns=2;i=5")
                contrast_client = client.get_node("ns=2;i=6")
                red_gain_client = client.get_node("ns=2;i=7")
                green_gain_client = client.get_node("ns=2;i=8")
                blue_gain_client = client.get_node("ns=2;i=9")
                command_control_client = client.get_node("ns=2;i=10")
                ip_address_client = client.get_node("ns=2;i=11")
                horizontal_resolution_client = client.get_node("ns=2;i=12")
                vertical_resolution_client = client.get_node("ns=2;i=13")

                # Check for Camera Start command from UAExpert
                # send the IP Address and start command  to the Video Kit
                if start_server_value == 1111:
                    print("Sending the command to start the video stream\n")
                    await ip_address_client.set_value(ip_address_server_value)
                    await start_client.set_value(1111)
                    await start_server.set_value(0000)

                # Check for Camera Stop command from UAExpert
                if stop_server_value == 1111:
                    print("Sending the command to stop the video stream\n")
                    await stop_client.set_value(1111)
                    await stop_server.set_value(0000)

                # Check for Camera Update command from UAExpert
                if command_control_server_value == 1111:
                    print("Sending the command to update the video stream\n")
                    await quality_factor_client.set_value(quality_factor_server_value)
                    await brightness_client.set_value(brightness_server_value)
                    await contrast_client.set_value(contrast_server_value)
                    await red_gain_client.set_value(red_gain_server_value)
                    await green_gain_client.set_value(green_gain_server_value)
                    await blue_gain_client.set_value(blue_gain_server_value)
                    await horizontal_resolution_client.set_value(
                        horizontal_resolution_server_value
                    )
                    await vertical_resolution_client.set_value(
                        vertical_resolution_server_value
                    )
                    await command_control_client.set_value(1111)
                    await command_control_server.set_value(0000)

                await asyncio.sleep(1)
        finally:
            await server.stop()
            await client.disconnect()
            print("OPC Server stopped")


if __name__ == "__main__":
    asyncio.run(main())
