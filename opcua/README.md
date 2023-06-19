# MPFS OPC-UA Industrial Edge Demo

This demo guide describes how to run the OPC-UA Industrial Edge Demo and
demonstrates the basic functionality of the PolarFire SoC Video kit, the
PolarFire SoC Icicle kit, the stepper motor, and their communication with
each other over the OPC-UA protocol.

## Scenario 1

In this scenario, OPC-UA Client UAExpert acts as the main controller, which
controls the PolarFire SoC Video kit and the stepper motor on the Icicle kit.
Control signals from UAExpert are sent over the OPC-UA channel to the
PolarFire SoC Video kit to start/stop video streaming and Icicle kit to
spin the stepper motor in clockwise or anti-clockwise direction.

## Scenario 2

In this scenario, OPC-UA Client UAExpert communicates with the Video kit
through the Icicle kit. The objective here is to demonstrate the Icicle
kit’s capability to act as an OPC-UA Client. Control signals from the
UAExpert are sent over the OPC-UA channel to the Icicle kit, which in
turn is communicated to the PolarFire SoC Video kit for video streaming.

For a step-by-step guide on how to run the demo, please click the App Note
link [AN4977](https://www.microchip.com/en-us/application-notes/an4977).
It contains detailed instructions to assist you throughout the process.
