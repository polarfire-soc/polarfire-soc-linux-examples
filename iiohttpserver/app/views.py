import csv
import glob
import os

from flask import render_template
from flask import jsonify
from app import app

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
        if not csvfile:
            app.logger.info("failed to open {} for reading".format(csvfilename))
            return render_template("waiting.html")

        reader = csv.reader(csvfile, skipinitialspace=True)
        if reader:
            app.logger.info("opened {} for reading".format(csvfilename))
        else:
            app.logger.info("failed to open {} for reading".format(csvfilename))
            return render_template("waiting.html")
        thiscurrenthistory = []

        try:
            for row in reader:
                if row[0] == 'epoch':
                    continue
                thiscurrenthistory.append({str((int(float(row[0])))): row[1]})
            current.append(round(float(row[1]), 2))
            csvfile.close()
            currenthistory.append(thiscurrenthistory.copy())
        except:
            app.logger.info("{} may be corrupt".format(csvfilename))
            csvfile.close()
            return render_template("waiting.html")

        csvfiles = glob.glob(voltagepattern)
        if len(csvfiles) == 0:
            app.logger.info("no data source")
            return render_template("waiting.html")
        csvfilename = max(csvfiles, key=os.path.getctime)

        csvfile = open(csvfilename, 'r')
        if not csvfile:
            app.logger.info("failed to open {} for reading".format(csvfilename))
        reader = csv.reader(csvfile, skipinitialspace=True)
        if reader:
            app.logger.info("opened {} for reading".format(csvfilename))
        else:
            app.logger.info("failed to open {} for reading".format(csvfilename))
            return render_template("waiting.html")
        thisvoltagehistory = []

        try:
            for row in reader:
                if row[0] == 'epoch':
                    continue
                thisvoltagehistory.append({str((int(float(row[0])))): row[1]})
            voltage.append(round(float(row[1]), 2))
            csvfile.close()
            voltagehistory.append(thisvoltagehistory.copy())
        except:
            app.logger.info("{} may be corrupt".format(csvfilename))
            csvfile.close()
            return render_template("waiting.html")

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
        if not csvfile:
            app.logger.info("failed to open {} for reading".format(csvfilename))
        reader = csv.reader(csvfile, skipinitialspace=True)
        if reader:
            app.logger.info("opened {} for reading".format(csvfilename))
        else:
            app.logger.info("failed to open {} for reading".format(csvfilename))
            return render_template("waiting.html")

        thiscurrenthistory = []

        try:
            for row in reader:
                if row[0] == 'epoch':
                    continue
                thiscurrenthistory.append({str((int(float(row[0])))): row[1]})
            current.append(round(float(row[1]), 2))
            csvfile.close()
            currenthistory.append(thiscurrenthistory.copy())
        except:
            app.logger.info("{} may be corrupt".format(csvfilename))
            csvfile.close()
            return render_template("waiting.html")

        csvfiles = glob.glob(voltagepattern)
        if len(csvfiles) == 0:
            app.logger.info("no data source")
            return render_template("waiting.html")
        csvfilename = max(csvfiles, key=os.path.getctime)

        csvfile = open(csvfilename, 'r')
        if not csvfile:
            app.logger.info("failed to open {} for reading".format(csvfilename))
        reader = csv.reader(csvfile, skipinitialspace=True)
        if reader:
            app.logger.info("opened {} for reading".format(csvfilename))
        else:
            app.logger.info("failed to open {} for reading".format(csvfilename))
            return render_template("waiting.html")
        thisvoltagehistory = []

        try:
            for row in reader:
                if row[0] == 'epoch':
                    continue
                thisvoltagehistory.append({str((int(float(row[0])))): row[1]})
            voltage.append(round(float(row[1]), 2))
            csvfile.close()
            voltagehistory.append(thisvoltagehistory.copy())
        except:
            app.logger.info("{} may be corrupt".format(csvfilename))
            csvfile.close()
            return render_template("waiting.html")

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
