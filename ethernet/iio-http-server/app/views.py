import csv
import glob
import os

from flask import render_template
from flask import jsonify
from app import app

HOSTNAME = os.uname()[1]
FILESTUB = '/var/lib/collectd/'+HOSTNAME+'/sensors-microchip,pac1934/{}-channel{}*'

NUMCHANNELS = 4

@app.route('/')

def index():
    return chart()

@app.route('/about')
def about():
    return render_template("about.html")

def getreader(pattern):
    csvfiles = glob.glob(pattern)

    if len(csvfiles) == 0:
        return 1, None, None # no files

    csvfilename = max(csvfiles, key=os.path.getctime)
    if not csvfilename:
        return 1, None, None # no files

    csvfile = open(csvfilename, 'r')
    if not csvfile:
        return 2, None, None # can't open file

    reader = csv.reader(csvfile, skipinitialspace=True)
    if not reader:
        csvfile.close()
        return 3, None, None # file corrupt?

    return 0, reader, csvfile

def handlechannel(pattern):
    reading = []
    readinghistory = []
    thisreadinghistory = []

    status, reader, csvfile = getreader(pattern)

    if status > 0:
        return status, [], []

    try:
        for row in reader:
            if row[0] == 'epoch':
                continue
            thisreadinghistory.append({str((int(float(row[0])))): row[1]})
        csvfile.close()
        reading = round(float(row[1]), 2)
        readinghistory = thisreadinghistory.copy()
    except:
        csvfile.close()
        return 4, [], []

    return 0, reading, readinghistory

def handlevoltagechannel(i):
    voltagepattern = FILESTUB.format('voltage', i)
    return handlechannel(voltagepattern)

def handlecurrentchannel(i):
    currentpattern = FILESTUB.format('current', i)
    return handlechannel(currentpattern)

def makemsg(status):
    if(status == 1):
        return "no iio files found"
    elif(status == 2):
        return "can't open iio file"
    elif(status == 3):
        return "iio file is not csv"
    elif(status == 4):
        return "iio file may be corrupt"
    else:
        return "unknown error"

def makereadings(current, currenthistory, voltage, voltagehistory):
    readings = [
        {
            "channel": "VDD",
            "index": 0,
            "voltage": voltage[0],
            "current": current[0],
            "voltage-history": voltagehistory[0],
            "current-history": currenthistory[0]
        },
        {
            "channel": "VDDA25",
            "index": 1,
            "voltage": voltage[1],
            "current": current[1],
            "voltage-history": voltagehistory[1],
            "current-history": currenthistory[1]
        },
        {
            "channel": "VDD25",
            "index": 2,
            "voltage": voltage[2],
            "current": current[2],
            "voltage-history": voltagehistory[2],
            "current-history": currenthistory[2]
        },
        {
            "channel": "VDDA",
            "index": 3,
            "voltage": voltage[3],
            "current": current[3],
            "voltage-history": voltagehistory[3],
            "current-history": currenthistory[3]
        }
    ]

    return readings


@app.route('/chart')
def chart():
    currenthistory = []
    voltagehistory = []
    current = []
    voltage = []
    val = []
    history = []

    for i in range(NUMCHANNELS):
        status, val, history = handlecurrentchannel(i)
        if status != 0:
            return render_template("waiting.html", msg=makemsg(status))
        current.append(val)
        currenthistory.append(history)
        status, val, history = handlevoltagechannel(i)
        if status !=0:
            return render_template("waiting.html", msg=makemsg(status))
        voltage.append(val)
        voltagehistory.append(history)

    return render_template("chart.html", readings=makereadings(current, currenthistory, voltage, voltagehistory))

@app.route('/_refresh', methods=['GET'])
def _refresh():
    currenthistory = []
    voltagehistory = []
    current = []
    voltage = []
    val = []
    history = []

    for i in range(NUMCHANNELS):
        status, val, history = handlecurrentchannel(i)
        if status != 0:
            return jsonify(readings="error: {}".format(status))
        current.append(val)
        currenthistory.append(history)
        status, val, history = handlevoltagechannel(i)
        if status !=0:
            return jsonify(readings="error: {}".format(status))
        voltage.append(val)
        voltagehistory.append(history)

    return jsonify(readings=makereadings(current, currenthistory, voltage, voltagehistory))
