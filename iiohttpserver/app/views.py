import csv
import glob
import os

#import logging

from flask import render_template
from flask import jsonify
from app import app

TESTING = False
HOSTNAME = os.uname()[1]
FILESTUB = '/var/lib/collectd/'+HOSTNAME+'/sensors-microchip,pac1934/{}-channel{}*'

@app.route('/')

def index():
    return chart()

@app.route('/about')
def about():
    return render_template("about.html")

@app.route('/_refresh', methods=['GET'])
def _refresh():
    currenthistory = []
    voltagehistory = []
    current = []
    voltage = []

    for i in range(4):
        if TESTING:
            csvfilename = "dummy_update.csv"
        else:
            currentpattern = FILESTUB.format('current', i)
            voltagepattern = FILESTUB.format('voltage', i)

            csvfiles = glob.glob(currentpattern)
            if len(csvfiles) == 0:
                app.logger.info("no data source")
                return render_template("waiting.html")
            csvfilename = max(csvfiles, key=os.path.getctime)

        if csvfilename:
            app.logger.info(csvfilename)
        else:
            app.logger.info("no data source")
            return render_template("waiting.html")

        csvfile = open(csvfilename, 'r')
        reader = csv.reader(csvfile, skipinitialspace=True)
        thiscurrenthistory = []
        for row in reader:
            if row[0] == 'epoch':
                continue
            thiscurrenthistory.append({str((int(float(row[0])))): row[1]})
        current.append(round(float(row[1]), 2))
        csvfile.close()
        currenthistory.append(thiscurrenthistory.copy())

        if TESTING:
            csvfilename = "dummy_update.csv"
        else:
            csvfiles = glob.glob(voltagepattern)
            if len(csvfiles) == 0:
                app.logger.info("no data source")
                return render_template("waiting.html")
            csvfilename = max(csvfiles, key=os.path.getctime)

        csvfile = open(csvfilename, 'r')
        reader = csv.reader(csvfile, skipinitialspace=True)
        thisvoltagehistory = []
        for row in reader:
            if row[0] == 'epoch':
                continue
            thisvoltagehistory.append({str((int(float(row[0])))): row[1]})
        voltage.append(round(float(row[1]), 2))
        csvfile.close()
        voltagehistory.append(thisvoltagehistory.copy())

    readings = [
        {
            "channel": "VDDA",
            "voltage": voltage[0],
            "current": current[0],
            "voltage-history": voltagehistory[0],
            "current-history": currenthistory[0]
        },
        {
            "channel": "VDD25",
            "voltage": voltage[1],
            "current": current[1],
            "voltage-history": voltagehistory[1],
            "current-history": currenthistory[1]
        },
        {
            "channel": "VDDA25",
            "voltage": voltage[2],
            "current": current[2],
            "voltage-history": voltagehistory[2],
            "current-history": currenthistory[2]
        },
        {
            "channel": "VDD",
            "voltage": voltage[3],
            "current": current[3],
            "voltage-history": voltagehistory[3],
            "current-history": currenthistory[3]
        }
    ]
    return jsonify(readings=readings)


@app.route('/chart')
def chart():
    currenthistory = []
    voltagehistory = []
    current = []
    voltage = []

    for i in range(4):
        if TESTING:
            csvfilename = "dummy.csv"
        else:
            currentpattern = FILESTUB.format('current', i)
            voltagepattern = FILESTUB.format('voltage', i)

            csvfiles = glob.glob(currentpattern)
            if len(csvfiles) == 0:
                app.logger.info("no data source")
                return render_template("waiting.html")
            csvfilename = max(csvfiles, key=os.path.getctime)

        if csvfilename:
            app.logger.info(csvfilename)
        else:
            app.logger.info("no data source")
            return

        csvfile = open(csvfilename, 'r')
        reader = csv.reader(csvfile, skipinitialspace=True)
        thiscurrenthistory = []
        for row in reader:
            if row[0] == 'epoch':
                continue
            thiscurrenthistory.append({str((int(float(row[0])))): row[1]})
        current.append(round(float(row[1]), 2))
        csvfile.close()
        currenthistory.append(thiscurrenthistory.copy())

        if TESTING:
            csvfilename = "dummy.csv"
        else:
            csvfiles = glob.glob(voltagepattern)
            if len(csvfiles) == 0:
                app.logger.info("no data source")
                return render_template("waiting.html")
            csvfilename = max(csvfiles, key=os.path.getctime)

        csvfile = open(csvfilename, 'r')
        reader = csv.reader(csvfile, skipinitialspace=True)
        thisvoltagehistory = []
        for row in reader:
            if row[0] == 'epoch':
                continue
            thisvoltagehistory.append({str((int(float(row[0])))): row[1]})
        voltage.append(round(float(row[1]), 2))
        csvfile.close()
        voltagehistory.append(thisvoltagehistory.copy())

    readings = [
        {
            "channel": "VDDA",
            "index": 0,
            "voltage": voltage[0],
            "current": current[0],
            "voltage-history": voltagehistory[0],
            "current-history": currenthistory[0]
        },
        {
            "channel": "VDD25",
            "index": 1,
            "voltage": voltage[1],
            "current": current[1],
            "voltage-history": voltagehistory[1],
            "current-history": currenthistory[1]
        },
        {
            "channel": "VDDA25",
            "index": 2,
            "voltage": voltage[2],
            "current": current[2],
            "voltage-history": voltagehistory[2],
            "current-history": currenthistory[2]
        },
        {
            "channel": "VDD",
            "index": 3,
            "voltage": voltage[3],
            "current": current[3],
            "voltage-history": voltagehistory[3],
            "current-history": currenthistory[3]
        }
    ]
    return render_template("chart.html", readings=readings)
