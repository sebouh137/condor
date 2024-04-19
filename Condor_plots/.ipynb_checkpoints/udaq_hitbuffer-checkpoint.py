#!/usr/bin/env python

import sys
import os
import argparse
import time
import glob
import serial
import datetime
import json
from cobs import cobs


PORT = '/dev/ttyUSB0'


def main():
        
    # dictionary of run info to be json dumped later
    runInfo = {}
    
    ap = argparse.ArgumentParser()
    ap.add_argument('-t', '--time', dest='runtime', type=int, default=10,
                    help='data acquisition time in seconds (default is 10 seconds)')
    ap.add_argument('-v', '--voltage', dest='voltage', type=int, default=2650,
                    help='voltage setting in daq units (default is 2650)')
    ap.add_argument('-d', '--disc', dest='disc', type=int, default=1390,
                    help='discriminator setting in daq units (default is 1390)')
    args = ap.parse_args()
    
    
    # subrun duration in seconds
    # need to decrease this time if trigger rate is >800 Hz
    # or increase the discriminator
    subruntime = 10
    
    runInfo['subruntime'] = subruntime
    runInfo['runtime'] = args.runtime
    
    # voltage sanity check
    if args.voltage > 2800:
        print('ERROR: voltage setting should never need to be >2800')
        sys.exit()
        
    # connect to udaq via USB
    try:
        ser = serial.Serial(port=PORT, baudrate=1000000)
    except:
        print("ERROR: is the USB cable connected?")
        sys.exit()
    
    ser.flushInput()
    ser.flushOutput()
    
    # reset any previous settings on the udaq
    commands = [
        'stop_run',
        'set_livetime_enable 0',
        'reset_schedule',
        'adc_reset_thresholds',
    ]
    
    for msg in commands:
        if cmdLoop(msg, ser, ntry=5) is None:
            sys.exit()
        
    # the uid and temperature
    uid = cmdLoop('get_uid', ser).strip().split()
    uid = ' '.join(uid[:3])
    temp = float(cmdLoop('getmon', ser).strip().split()[1])
    
    runInfo['uid'] = uid
    runInfo['temperature'] = temp
    runInfo['voltage'] = args.voltage
    runInfo['threshold'] = args.disc
    
    # initialize the adcs, set voltage, threshold, etc.
    commands = [
        'auxdac 1 {0}'.format(args.voltage),
        'dac 1 {0}'.format(args.disc),
        
        'timestamp_mode 4',
        'disc_opm 1',
        
        'adc_timer_delay 0 18',
        'adc_timer_delay 1 136',
        'adc_timer_delay 2 18',
        'adc_timer_delay 3 136',
        'adc_timer_delay 4 18',
        #'adc_timer_delay 4 136',
        'adc_timer_delay 5 136',
        'adc_timer_delay 6 136',
        'adc_timer_delay 7 136',
        
        'adc_hist_enable 0',  # turn off ascii histograms
        
        'adc_recording_thresholds  0 0 0',  # high gain
        'adc_recording_thresholds  2 0 0',  # med gain
        'adc_recording_thresholds 12 0 0',  # low gain
        
        'adc_enable  0 1',  # high gain
        'adc_enable  2 1',  # med gain
        'adc_enable 12 1',  # low gain
    ]
    
    for msg in commands:
        if cmdLoop(msg, ser) is None:
            sys.exit()
    
    mydatetime = datetime.datetime.now()
    mydate = str(mydatetime.date())
    mytime = str(mydatetime.time().strftime("%H:%M:%S"))
    
    runInfo['date'] = mydate
    runInfo['time'] = mytime
    
    runInfo['udaq_time'] = cmdLoop('print_time', ser).split('\n')[0]
    
    subruns = int(round(args.runtime/float(subruntime), 0))
    if subruns <= 0 : subruns = 1
    runInfo['subruns'] = subruns
    
    # start and stop the run here
    data = []
    cmdLoop('set_livetime_enable 1', ser)
    out = None
    for i in range(subruns):
        print('INFO: running subrun {0} of {1}'.format(i+1, subruns))
        if cmdLoop('run 1 0 0', ser, 5) is None:
            break
        print('INFO: collecting data for {0} seconds...'.format(subruntime))
        time.sleep(subruntime)
        
        # In noisy environments or with really high rates
        # the stop command takes a number of retries before
        # the udaq actually processes the command.
        out = cmdLoop('stop_run', ser, 50)
        if out is None:
            break
        
        # the hex dump - useful for debugging
        #hdump = cmdLoop('dump_hits_hex 4096', ser, ntry=5, decode=True)
        #print(hdump)

        dump = cmdLoop('dump_hits_binary', ser, ntry=5, decode=False)
        if dump is not None:
            data.append(dump)

        # print out the trigger rate
        rate = getRate(ser)
        print('INFO: trigger rate = {0} Hz'.format(rate))
        if rate > 0.0:
            runInfo['trigrate'] = rate
        
    if out is not None:
        cmdLoop('set_livetime_enable 0', ser)
        
        # paranoid safety measure - set voltage back to 0
        cmdLoop('auxdac 1 0', ser)
    
    # close the serial connection
    closeSerial(ser)
    
    if not len(data) > 0:
        print('\nERROR: no data found, quitting...')
        return

    # determine the run number and dir path/name
    rundir, runfile = getNextRun()
    
    # write out the binary
    with open(os.path.join(rundir, runfile+'.bin'), 'wb') as bfile:
        for dump in data:
            bfile.write(cobsDecode(dump))
    
    # write the info to json
    with open(os.path.join(rundir, runfile+'.json'), 'w') as jfile:
        json.dump(runInfo, jfile, separators=(', ', ': '), indent=4)


    print('\n  SUCCESS: wrote data to [{0}]\n'.format(runfile))
    
    return


# cobs decoding the frames
def cobsDecode(binaryDump, debug=0):

    # find all the frame markers
    markers = []
    for i, val in enumerate(binaryDump):
        bval = val.to_bytes(length=1, byteorder='little')
        if bval == b'\x00':
            markers.append(i)
    if debug: print('COBS: found', len(markers)/2., 'frames')
    
    alldata = bytearray()
    for i in range(0, len(markers), 2):
        
        # grab the frame markers
        fstart = markers[i]
        fstop = markers[i+1]
        
        # select the frame
        cdata = binaryDump[fstart+1:fstop]
        
        # cobs decode the frame
        data = cobs.decode(cdata)
        
        # grab the checksum or later checking - trailing 2 bytes
        #cs = data[-2:]
        #if debug: print(cs)
        
        # skip "number of messages" frame
        if len(data) == 5:
            if debug: print('COBS: skipping message frame --> {0}'.format(data))
            continue
        
        # skip "OK" frame
        if 'OK' in data.decode(errors="ignore"):
            if debug: print('COBS: skipping \"OK\" frame --> {0}'.format(data))
            continue
        
        # strip off the BusID - first 1 byte
        # strip off the checksum - trailing 2 bytes
        alldata.extend(bytearray(data[1:-2]))
        
    return alldata


# flush and close the serial connection
def closeSerial(serial):
    serial.flushInput()
    serial.flushOutput()
    serial.close()


# try the command ntry times
def cmdLoop(msg, serial, ntry=3, decode=True):
    for i in range(ntry):
        print(msg)
        serial.write((msg+'\n').encode())
        out = collect_output(serial, decode)
        if decode:
            if 'OK' in out:
                return out
            else:
                print(out)
                serial.flushInput()
                serial.flushOutput()
                time.sleep(0.1)
        else:
            if 'OK' in out[-4:].decode(errors="ignore"):
                return out
            else:
                serial.flushInput()
                serial.flushOutput()
                time.sleep(0.1)
    print('ERROR: giving up')
    #closeSerial(serial)
    return None


# A method of collecting the output with an interbyte timeout
def collect_output(serial, decode=True):
    slept = False
    if decode:
        out = ''
    else:
        out = bytearray()
    while True:
        n = serial.inWaiting()
        if n == 0:
            if slept == True:
                break
            time.sleep(0.1)
            slept = not slept
        else:
            if decode:
                out += serial.read(n).decode()
            else:
                out.extend(serial.read(n))
                #print('out =', bytes(out))
            slept = False
    return out


# set the next run number and output dir
def getNextRun(runsdir='runs'):
    here = os.path.dirname(os.path.abspath(__file__))
    if os.path.exists(os.path.join(here, runsdir)):
        lastrun = sorted(glob.glob(os.path.join(here, runsdir, 'run_*')))
        if lastrun:
            lastrun = int(lastrun[-1].split('run_')[-1])
        else:
            lastrun = 0
    else:
        lastrun = 0
    run = 'run_{0}'.format(str(lastrun+1).zfill(7))
    rdir = os.path.join(here, runsdir, run)
    os.makedirs(os.path.join(here, runsdir, run))
    return rdir, run

    
def getRate(ser):
    stats = cmdLoop('get_run_statistics', ser).strip().split()
    events = int(stats[1])
    duration = float(stats[4])
    trigrate = events / duration
    return round(trigrate, 1)



if __name__ == "__main__":
    main()

