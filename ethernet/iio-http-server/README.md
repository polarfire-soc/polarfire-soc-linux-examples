# PolarFire SoC Ethernet/IIO Example

This test program interacts with the PAC193x voltage/current sensor on the board
and displays graphs of the readings using a webserver.

To run this example program:

1. Ensure collectd is installed.  By default it installs to `/usr/sbin/collectd`

2. Check `collectdiio.service` is in place at `/lib/systemd/system`
   If not, copy `collection/collectdiio.service` to `/lib/systemd/system`

3. Run `ifconfig` to determine IP address - e.g. 192.168.20.11

4. Run `run.sh`

5. Start web browser on another machine on the local network

6. In your web browser, navigate to to the IP address from `ifconfig`, in this
   example: 192.168.20.11
