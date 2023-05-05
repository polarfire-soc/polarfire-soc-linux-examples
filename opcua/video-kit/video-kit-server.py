#!/usr/bin/env python3
import sys
from opcua import ua, Server
from pathlib import Path
from random import randint
import time
from datetime import datetime, timedelta
import os, locale
import subprocess

# Set Locale
locale.setlocale(locale.LC_ALL, 'en_US')
t = time.strftime("%a, %d %b %Y %H:%M:%S")
print (t) # works fine: Fr, 05 Jun 2020 14:37:02

PORT = "4840"
if sys.argv[1:]:
    PORT = sys.argv[1]

server = Server()
url = "opc.tcp://0.0.0.0:" + PORT
server.set_endpoint(url)

name = "PFSoC OPC Server"
addspace = server.register_namespace(name)

node = server.get_objects_node()
camera_control_object = node.add_object(addspace, "Camera Controls")

server_start = camera_control_object.add_variable(addspace, "Streaming Start", 0000)
server_stop = camera_control_object.add_variable(addspace, "Streaming Stop", 0000)

quality_factor = camera_control_object.add_variable(addspace, "Camera - Qaulity Factor [25-50]", 30)
brightness = camera_control_object.add_variable(addspace, "Camera - Brightness Control [0-255]", 137)
contrast = camera_control_object.add_variable(addspace, "Camera - Contrast Control [0-255]", 154)
red_gain = camera_control_object.add_variable(addspace, "Camera - Red Gain [0-255]", 122)
green_gain = camera_control_object.add_variable(addspace, "Camera - Green Gain [0-255]", 102)
blue_gain = camera_control_object.add_variable(addspace, "Camera - Blue Gain [0-255]", 138)

command_control = camera_control_object.add_variable(addspace, "Vision Command Control", 0000)
ip_address = camera_control_object.add_variable(addspace, "IP Address", "192.168.1.1")

horizontal_resolution = camera_control_object.add_variable(addspace, "Camera - Horizontal Resolution", "1280")
vertical_resolution = camera_control_object.add_variable(addspace, "Camera - Vertical Resolution", "720")

command_control.set_writable()
server_start.set_writable()
server_stop.set_writable()
quality_factor.set_writable()
brightness.set_writable()
contrast.set_writable()
red_gain.set_writable()
green_gain.set_writable()
blue_gain.set_writable()
ip_address.set_writable()
horizontal_resolution.set_writable()
vertical_resolution.set_writable()

server.start()
print("OPC Server started")

def stop_stream():
    args = 'v4l2-ctl -d /dev/video0 --set-ctrl=gain_automatic=0'
    subprocess.call(args, shell=True)
    args = 'kill $(pidof ffmpeg)'
    subprocess.call(args, shell=True)

try:
    while True:
        server_start_value = server_start.get_value()
        server_stop_value = server_stop.get_value()
        ip_address_value = ip_address.get_value()
        command_control_value = command_control.get_value()
        quality_factor_value = quality_factor.get_value()
        brightness_value = brightness.get_value()
        contrast_value = contrast.get_value()
        red_gain_value = red_gain.get_value()
        green_gain_value = green_gain.get_value()
        blue_gain_value = blue_gain.get_value()
        horizontal_resolution_value = horizontal_resolution.get_value()
        vertical_resolution_value = vertical_resolution.get_value()

        # Check for start value, if matches then start the stream
        if server_start_value == 1111:
            print("Please wait while stream is being started")
            stop_stream()
            args = 'ffmpeg  -i /dev/video0  -c:v copy -f rtp -sdp_file video.sdp "rtp://' + ip_address_value + ':10000" </dev/null  > messages 2>error_log &'
            subprocess.call(args, shell=True)
            args = 'v4l2-ctl -d /dev/video0 --set-ctrl=gain_automatic=1'
            subprocess.call(args, shell=True)
            args = 'v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=vertical_blanking=1170'
            subprocess.call(args, shell=True)
            server_start.set_value(0000)

        # Check for stop value, if matches then stop the stream
        if server_stop_value == 1111:
            print("Please wait while stream is being stopped")
            stop_stream()
            server_stop.set_value(0000)

        # Check for update value, if matches then update the stream
        if command_control_value == 1111:
            print("Please wait while stream is being updated")
            args = '/usr/bin/v4l2-ctl -d /dev/video0 --set-ctrl=quality_factor=' + str(quality_factor_value) + ' --set-ctrl=brightness=' + str(brightness_value) + ' --set-ctrl=contrast=' + str(contrast_value) + ' --set-ctrl=gain_red=' + str(red_gain_value) + ' --set-ctrl=gain_green=' + str(green_gain_value) + ' --set-ctrl=gain_blue=' + str(blue_gain_value)
            subprocess.call(args, shell=True)
            # program Horizontal Resolution value
            args = 'devmem2 0x40001078 w ' + str(horizontal_resolution_value) + ' >/dev/null'
            subprocess.call(args, shell=True)
            # program Vertical Resolution value
            args = 'devmem2 0x4000107C w ' + str(vertical_resolution_value) + ' >/dev/null'
            subprocess.call(args, shell=True)
            command_control.set_value(0000)

        time.sleep(1)
finally:
    server.stop()
    print("OPC Server stopped")

