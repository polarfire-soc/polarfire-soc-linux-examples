#!/usr/bin/env python3
import sys
import time, os
from datetime import datetime, timedelta
from opcua import ua, Server, Client

PORT = "4840"
if sys.argv[1:]:
    PORT = sys.argv[1]

server = Server()
url = "opc.tcp://0.0.0.0:" + PORT
server.set_endpoint(url)

name = "PFSoC OPC Server"
addspace = server.register_namespace(name)

node = server.get_objects_node()
camera_control_object = node.add_object(addspace, "Video Kit - Camera Controls")

start_server = camera_control_object.add_variable(addspace, "Streaming Start", 0000)
stop_server = camera_control_object.add_variable(addspace, "Streaming Stop", 0000)

quality_factor_server = camera_control_object.add_variable(
    addspace, "Camera - Qaulity Factor [25-50]", 30
)
brightness_server = camera_control_object.add_variable(
    addspace, "Camera - Brightness Control [0-255]", 137
)
contrast_server = camera_control_object.add_variable(
    addspace, "Camera - Contrast Control [0-255]", 154
)
red_gain_server = camera_control_object.add_variable(
    addspace, "Camera - Red Gain [0-255]", 122
)
green_gain_server = camera_control_object.add_variable(
    addspace, "Camera - Green Gain [0-255]", 102
)
blue_gain_server = camera_control_object.add_variable(
    addspace, "Camera - Blue Gain [0-255]", 138
)

command_control_server = camera_control_object.add_variable(
    addspace, "Vision Command Control", 0000
)
ip_address_server = camera_control_object.add_variable(
    addspace, "IP Address", "192.168.1.1"
)

horizontal_resolution_server = camera_control_object.add_variable(
    addspace, "Camera - Horizontal Resolution", "1280"
)
vertical_resolution_server = camera_control_object.add_variable(
    addspace, "Camera - Vertical Resolution", "720"
)

start_server.set_writable()
stop_server.set_writable()
command_control_server.set_writable()
ip_address_server.set_writable()

quality_factor_server.set_writable()
brightness_server.set_writable()
contrast_server.set_writable()
red_gain_server.set_writable()
green_gain_server.set_writable()
blue_gain_server.set_writable()
horizontal_resolution_server.set_writable()
vertical_resolution_server.set_writable()

server.start()
print("ICICLE Kit - OPC server started at {}".format(url))

url = "opc.tcp://10.60.132.159:4840"
client = Client(url)
try:
    client.connect()
    print("ICICLE Kit - OPC Client connected to Video Kit - OPC Server")
    while True:
        # Server Values from UAExpert
        start_server_value = start_server.get_value()
        stop_server_value = stop_server.get_value()
        ip_address_server_value = ip_address_server.get_value()
        command_control_server_value = command_control_server.get_value()
        quality_factor_server_value = quality_factor_server.get_value()
        if quality_factor_server_value < 25 or quality_factor_server_value > 50:
            quality_factor_server.set_value(30)
        brightness_server_value = brightness_server.get_value()
        contrast_server_value = contrast_server.get_value()
        red_gain_server_value = red_gain_server.get_value()
        green_gain_server_value = green_gain_server.get_value()
        blue_gain_server_value = blue_gain_server.get_value()
        horizontal_resolution_server_value = horizontal_resolution_server.get_value()
        vertical_resolution_server_value = vertical_resolution_server.get_value()

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
            ip_address_client.set_value(ip_address_server_value)
            start_client.set_value(1111)
            start_server.set_value(0000)

        # Check for Camera Stop command from UAExpert
        if stop_server_value == 1111:
            print("Sending the command to stop the video stream\n")
            stop_client.set_value(1111)
            stop_server.set_value(0000)

        # Check for Camera Update command from UAExpert
        if command_control_server_value == 1111:
            print("Sending the command to update the video stream\n")
            quality_factor_client.set_value(quality_factor_server_value)
            brightness_client.set_value(brightness_server_value)
            contrast_client.set_value(contrast_server_value)
            red_gain_client.set_value(red_gain_server_value)
            green_gain_client.set_value(green_gain_server_value)
            blue_gain_client.set_value(blue_gain_server_value)
            horizontal_resolution_client.set_value(horizontal_resolution_server_value)
            vertical_resolution_client.set_value(vertical_resolution_server_value)
            command_control_client.set_value(1111)
            command_control_server.set_value(0000)

        time.sleep(1)

finally:
    server.stop()
    client.disconnect()
    print("OPC Server stopped")
