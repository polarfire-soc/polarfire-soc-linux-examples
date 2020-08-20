#!/usr/bin/env bash

mkdir -p "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"
chmod a+x "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"
chmod a+w "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"
chmod a+r "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"

echo "stopping collectd"
systemctl stop collectd

echo "starting collectd for iio"
systemctl restart collectdiio

echo "starting webserver"
export FLASK_APP='/opt/microchip/iiohttpserver/run.py'
flask run --host="0.0.0.0" --port=80

echo "stopping collectd for iio"
systemctl stop collectdiio

echo "restarting collectd"
systemctl restart collectd

