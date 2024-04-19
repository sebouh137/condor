import os

voltages = range(2375,2425,1)

for voltage in voltages:
    os.system(f"python udaq_histogram.py -t 60 -d 1500 -v {voltage}")
    
thresholds = range(1425,1550,1)

'''
for threshold in thresholds:
    os.system(f"python udaq_histogram.py -t 60 -d {threshold} -v 2400")
'''