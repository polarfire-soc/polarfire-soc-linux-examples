#!/usr/bin/env bash

mkdir -p "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"
chmod a+x "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"
chmod a+w "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"
chmod a+r "/var/lib/collectd/$HOSTNAME/sensors-microchip,pac1934"

echo "stopping collectd"
systemctl stop collectd
cp /etc/collectd.conf collection/collectd.conf.orig
cp collection/collectd.conf /etc/collectd.conf

echo "restarting collectd"
systemctl restart collectd

echo "starting webserver"
export FLASK_APP='run.py'
flask run --host="0.0.0.0" --port=80

echo "stopping collectd"
systemctl stop collectd

echo "restarting collectd"
cp collection/collectd.conf.orig /etc/collectd.conf
systemctl restart collectd

