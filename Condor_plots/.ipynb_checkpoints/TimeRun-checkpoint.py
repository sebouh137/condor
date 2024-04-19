import schedule
from datetime import datetime, timedelta, time
import os
import subprocess
from apscheduler.schedulers.blocking import BlockingScheduler

count = 0

def job_function():
    os.system("python udaq_histogram.py -t 60 -d 1500 -v 2400")
    global count, scheduler

    # Execute the job till the count of 5 
    count = count + 1
    if count == 2:
        scheduler.remove_job('my_job_id')
        scheduler.shutdown()

os.system("python udaq_histogram.py -t 60 -d 1500 -v 2400")
scheduler = BlockingScheduler()
scheduler.add_job(job_function, 'interval', minutes=2, id='my_job_id')

scheduler.start()