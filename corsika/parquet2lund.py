# first install dependencies
# pip install pandas
# pip install pyarrow
# pip install numpy
# pip install ROOT
import pyarrow.parquet as pq
import pandas as pd
import pyarrow as pa
import numpy as np
import ROOT
import sys

elevation=5300
inputfile=None
outputfile=None

def print_usage():
    print("""
    useage example:

    python parquet2lund.py --elevation 5300 --input in.parquet --output out.lund

    converts in.parquet to out.lund, printing out only particles that cross the z= 5300 m plane (approximate elevation of condor).  The "elevation" argument is optional.  
    """)

    
for i in range(1,len(sys.argv)):
    if sys.argv[i] == "--elevation" and len(sys.argv)>=i:
        elevation=float(sys.argv[i+1])
        continue
    if sys.argv[i] == "--input" and len(sys.argv)>=i:
        inputfile=sys.argv[i+1]
        continue
    if sys.argv[i] == "--output" and len(sys.argv)>=i:
        outputfile=sys.argv[i+1]
        continue
if inputfile==None or outputfile==None:
    print_usage()
    exit()

df = pq.read_pandas(inputfile).to_pandas()
nevents = df.shower[len(df)-1]
UNUSED =0

condition=f"end_z<{elevation} and start_z>{elevation}"
#condition='pdg==130'

masses={}
pdgdb = ROOT.TDatabasePDG()


with open(outputfile, "w") as out:
    for shower in range(nevents+1):
        subdf = df.query(f"shower == {shower} and {condition}")
        print(len(subdf), UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED,file=out, sep='\t') 
        ii=0
        for i in subdf.index:
            ii+=1
            norm = np.sqrt((subdf.end_x[i]-subdf.start_x[i])**2+(subdf.end_y[i]-subdf.start_y[i])**2+(subdf.end_z[i]-subdf.start_z[i])**2)
            pdg = int(subdf.pdg[i])
            if pdg in masses.keys():
                m = masses[pdg]
            else:
                part = pdgdb.GetParticle(pdg)
                if part != None:
                    m=part.Mass()
                else: # Particle not found in database.  Just set the mass to zero.
                    m= 0
                masses[pdg]=m
                
                
            E  = subdf.energy[i]
            p = np.sqrt(E**2-m**2)
            px = p*(subdf.end_x[i]-subdf.start_x[i])/norm
            py = p*(subdf.end_y[i]-subdf.start_y[i])/norm
            pz = p*(subdf.end_z[i]-subdf.start_z[i])/norm


            
            vx = subdf.start_x[i]*100
            vy = subdf.start_y[i]*100
            vz = subdf.start_z[i]*100
            print(ii, UNUSED, 1, subdf.pdg[i], UNUSED,UNUSED, px,py, pz, E, m, vx,vy,vz, file=out, sep='\t')
            #print(subdf[i].shower)
        del subdf
