# first install dependencies
# pip install pandas
# pip install pyarrow
# pip install numpy
# pip install ROOT
# pip install pyhepmc

import pyarrow.parquet as pq
import pyhepmc
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

    python parquet2hepmc.py --elevation 5450 --input in.parquet --output out.hepmc -z zenith -a azimuth

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

masses={}
df['m']=np.zeroes(len(df))
for pdg in list(set(df.pdg)):
    part = db.GetParticle(pdg)
    if part != None:
        m=part.Mass()
    else: # Particle not found in database.  Just set the mass to zero.                                                      
        m= 0
    masses[pdg]=m
    df['m'] += (df.pdg==pdg)*m

E  = df.energy[i]
p = np.sqrt(E**2-df.m**2)
df['px'] = p*np.cos(phi)*np.sin(theta)
df['py'] = p*np.sin(phi)*np.sin(theta)
df['pz'] = -p*np.cos(theta)

vx = subdf.x[i]*1000
vy = subdf.y[i]*1000
vz = subdf.z[i]*1000

#extrapolate to the correct elevation                                                                                        
if elevation is not None:
    vx -= np.tan(theta)*np.cos(phi)*(elevation*1000-vz)
    vy -= np.tan(theta)*np.sin(phi)*(elevation*1000-vz)
    vz = elevation*1000
df['vx']=vx
df['vy']=vy
df['vz']=vz
df['ct'] =df.time*299792458*1000


nevents = len(set(df.shower))
UNUSED =0

#condition=f"end_z<{elevation} and start_z>{elevation}"
#condition='pdg==130'

masses={}
pdgdb = ROOT.TDatabasePDG()


with pyhepmc.open(outputfile, "w", precision=5) as f:
    for shower in range(nevents):
        subdf = df.query(f"shower == {shower}")
        print(len(subdf), UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED,file=out, sep='\t') 
        ii=0


        
        event=pyhepmc.GenEvent(subdf.px, subdf.py, subdf.pz, subdf.energy, subdf.m, subdf.pdg, subdf.status,
                               None, None, subdf.vx, subdf.vy, subdf.vz, subdf.ct)
        """for i in subdf.index:
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
                vx -= np.tan(theta)*np.cos(phi)*(elevation*100-vz)
                vy -= np.tan(theta)*np.sin(phi)*(elevation*100-vz)
                vz = elevation*100

            #rotate 90 degrees in yz, to make y be vertical (for GEMC)
            tmp=vy
            vy=vz
            vz=-tmp

            tmp=py
            py=pz
            pz=-tmp

            #correct for time difference
            mintime=np.min(subdf.time)
            c=299792458

            p=np.sqrt(px**2+py**2+pz**2)
            ux, uy, uz = px/p, py/p, pz/p
            
            vx-=100*(subdf.time[i]-mintime)*ux/c
            vy-=100*(subdf.time[i]-mintime)*uy/c
            vx-=100*(subdf.time[i]-mintime)*uz/c

            particle=pyhepmc.GenParticle(pyhepmc.FourVector(
            print(ii, UNUSED, 1, subdf.pdg[i], UNUSED,UNUSED, px,py, pz, E, m, vx,vy,vz, file=out, sep='\t')
            #print(subdf[i].shower)
        """
        del subdf

        f.write(event)
