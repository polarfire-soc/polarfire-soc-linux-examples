#!/usr/bin/env python3

import asyncio, logging
from asyncua import Server, ua
import sys, subprocess, os


async def stop_stream():
    args = "v4l2-ctl -d /dev/video0 --set-ctrl=gain_automatic=0"
    subprocess.call(args, shell=True)
    args = "kill $(pidof ffmpeg)"
    subprocess.call(args, shell=True)


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

    name = "Video Kit - OPC Server"
    addspace = await server.register_namespace(name)

    node = server.get_objects_node()
    camera_control_object = await node.add_object(addspace, "Camera Controls")

    server_start = await camera_control_object.add_variable(
        addspace, "Streaming Start", 0000
    )
    server_stop = await camera_control_object.add_variable(
        addspace, "Streaming Stop", 0000
    )

    quality_factor = await camera_control_object.add_variable(
        addspace, "Camera - Qaulity Factor [25-50]", 30
    )
    brightness = await camera_control_object.add_variable(
        addspace, "Camera - Brightness Control [0-255]", 137
    )
    contrast = await camera_control_object.add_variable(
        addspace, "Camera - Contrast Control [0-255]", 154
    )
    red_gain = await camera_control_object.add_variable(
        addspace, "Camera - Red Gain [0-255]", 122
    )
    green_gain = await camera_control_object.add_variable(
        addspace, "Camera - Green Gain [0-255]", 102
    )
    blue_gain = await camera_control_object.add_variable(
        addspace, "Camera - Blue Gain [0-255]", 138
    )

    command_control = await camera_control_object.add_variable(
        addspace, "Vision Command Control", 0000
    )
    ip_address = await camera_control_object.add_variable(
        addspace, "IP Address", "192.168.1.1"
    )

    horizontal_resolution = await camera_control_object.add_variable(
        addspace, "Camera - Horizontal Resolution [432|640|960|1280|1920]", 1280
    )
    vertical_resolution = await camera_control_object.add_variable(
        addspace, "Camera - Vertical Resolution [240|480|544|720|1072]", 720
    )

    await command_control.set_writable()
    await server_start.set_writable()
    await server_stop.set_writable()
    await quality_factor.set_writable()
    await brightness.set_writable()
    await contrast.set_writable()
    await red_gain.set_writable()
    await green_gain.set_writable()
    await blue_gain.set_writable()
    await ip_address.set_writable()
    await horizontal_resolution.set_writable()
    await vertical_resolution.set_writable()

    _logger.info("Starting server!")
    print("Video Kit OPC-UA Server Started")

    # default and valid resolutions
    valid_horizontal_resolutions = [432, 640, 960, 1280, 1920]
    default_horizontal_resolution = 1280
    valid_vertical_resolutions = [240, 480, 544, 720, 1072]
    default_vertical_resolution = 720

    async with server:
        try:
            while True:
                server_start_value = await server_start.get_value()
                server_stop_value = await server_stop.get_value()
                ip_address_value = await ip_address.get_value()
                command_control_value = await command_control.get_value()
                quality_factor_value = await quality_factor.get_value()
                brightness_value = await brightness.get_value()
                contrast_value = await contrast.get_value()
                red_gain_value = await red_gain.get_value()
                green_gain_value = await green_gain.get_value()
                blue_gain_value = await blue_gain.get_value()
                horizontal_resolution_value = await horizontal_resolution.get_value()
                vertical_resolution_value = await vertical_resolution.get_value()

                # Check and correct resolution value
                if horizontal_resolution_value not in valid_horizontal_resolutions:
                    horizontal_resolution_value = default_horizontal_resolution
                    await horizontal_resolution.set_value(default_horizontal_resolution)

                if vertical_resolution_value not in valid_vertical_resolutions:
                    vertical_resolution_value = default_vertical_resolution
                    await vertical_resolution.set_value(default_vertical_resolution)

                # Check for start value, if matches then start the stream
                if server_start_value == 1111:
                    print("Please wait while stream is being started")
                    await stop_stream()
                    args = (
                        'ffmpeg  -i /dev/video0  -c:v copy -f rtp -sdp_file video.sdp "rtp://'
                        + ip_address_value
                        + ':10000" </dev/null  > messages 2>error_log &'
                    )
                    subprocess.call(args, shell=True)
                    args = "v4l2-ctl -d /dev/video0 --set-ctrl=gain_automatic=1"
                    subprocess.call(args, shell=True)
                    args = (
                        "v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=vertical_blanking=1170"
                    )
                    subprocess.call(args, shell=True)
                    await server_start.set_value(0000)

                # Check for stop value, if matches then stop the stream
                if server_stop_value == 1111:
                    print("Please wait while stream is being stopped")
                    await stop_stream()
                    await server_stop.set_value(0000)

                # Check for update value, if matches then update the stream
                if command_control_value == 1111:
                    print("Please wait while stream is being updated")
                    args = (
                        "/usr/bin/v4l2-ctl -d /dev/video0 --set-ctrl=quality_factor="
                        + str(quality_factor_value)
                        + " --set-ctrl=brightness="
                        + str(brightness_value)
                        + " --set-ctrl=contrast="
                        + str(contrast_value)
                        + " --set-ctrl=gain_red="
                        + str(red_gain_value)
                        + " --set-ctrl=gain_green="
                        + str(green_gain_value)
                        + " --set-ctrl=gain_blue="
                        + str(blue_gain_value)
                    )
                    subprocess.call(args, shell=True)
                    # program Horizontal Resolution value
                    args = (
                        "devmem2 0x40001078 w "
                        + str(horizontal_resolution_value)
                        + " >/dev/null"
                    )
                    subprocess.call(args, shell=True)
                    # program Vertical Resolution value
                    args = (
                        "devmem2 0x4000107C w "
                        + str(vertical_resolution_value)
                        + " >/dev/null"
                    )
                    subprocess.call(args, shell=True)
                    await command_control.set_value(0000)

                await asyncio.sleep(1)
        finally:
            await server.stop()
            print("OPC Server stopped")


if __name__ == "__main__":
    asyncio.run(main())
