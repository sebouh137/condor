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

theta=0
phi=0

def print_usage():
    print("""
    useage example:

    python parquet2lund.py --elevation 5450 --input in.parquet --output out.lund -z zenith -a azimuth

    converts in.parquet to out.lund, extrapolating all particle's trajectories to z=5350 (the approximate elevation of CONDOR + 50 m).  The "elevation" argument is optional.  The zenith and azimuth are in degrees.  If omitted, they default to 0
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
    if sys.argv[i] in "--zenith -z".split() and len(sys.argv)>=i:
        theta=np.pi/180*float(sys.argv[i+1])
        continue
    if sys.argv[i] in "--azimuth -a".split() and len(sys.argv)>=i:
        phi=np.pi/180*float(sys.argv[i+1])
        continue
if inputfile==None or outputfile==None:
    print_usage()
    exit()

df = pq.read_pandas(inputfile).to_pandas()
nevents = len(set(df.shower))
UNUSED =0

#condition=f"end_z<{elevation} and start_z>{elevation}"
#condition='pdg==130'

masses={}
pdgdb = ROOT.TDatabasePDG()


with open(outputfile, "w") as out:
    for shower in range(nevents):
        subdf = df.query(f"shower == {shower}")
        print(len(subdf), UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED,file=out, sep='\t') 
        ii=0
        for i in subdf.index:
            ii+=1
            #norm = np.sqrt((subdf.end_x[i]-subdf.start_x[i])**2+(subdf.end_y[i]-subdf.start_y[i])**2+(subdf.end_z[i]-subdf.start_z[i])**2)
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
            px = p*np.cos(phi)*np.sin(theta)
            py = p*np.sin(phi)*np.sin(theta)
            pz = -p*np.cos(theta)


            
            vx = subdf.x[i]*100
            vy = subdf.y[i]*100
            vz = subdf.z[i]*100

            #extrapolate to the correct elevation
            if elevation is not None:
                vx -= np.tan(theta)*np.cos(phi)*(elevation-vz)
                vy -= np.tan(theta)*np.sin(phi)*(elevation-vz)
                vz = elevation
                
            print(ii, UNUSED, 1, subdf.pdg[i], UNUSED,UNUSED, px,py, pz, E, m, vx,vy,vz, file=out, sep='\t')
            #print(subdf[i].shower)
        del subdf
