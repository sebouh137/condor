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
pdgdb = ROOT.TDatabasePDG()

masses={}
df['m']=np.zeros(len(df))
for pdg in list(set(df.pdg)):
    part = pdgdb.GetParticle(pdg)
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

vx = df.x[i]*1000
vy = df.y[i]*1000
vz = df.z[i]*1000

#extrapolate to the correct elevation                                                                                        
if elevation is not None:
    vx -= np.tan(theta)*np.cos(phi)*(elevation*1000-vz)
    vy -= np.tan(theta)*np.sin(phi)*(elevation*1000-vz)
    vz = elevation*1000
df['vx']=vx
df['vy']=vy
df['vz']=vz
df['ct'] =df.time*299792458*1000
df['status']=np.ones(len(df))

nevents = len(set(df.shower))
UNUSED =0

#condition=f"end_z<{elevation} and start_z>{elevation}"
#condition='pdg==130'





with pyhepmc.open(outputfile, "w", precision=6) as f:
    for shower in range(nevents):
        subdf = df.query(f"shower == {shower}")
        #print(len(subdf), UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED,file=out, sep='\t') 
        #ii=0
        
        event=pyhepmc.GenEvent()

        #parent and children particle indices
        #use the indices that leave this empty
        #parents=[(0,0)]*len(subdf)
        #children=[(0,0)]*len(subdf)
        
        event.from_hepevt(shower,subdf.px, subdf.py, subdf.pz, subdf.energy,subdf.m, subdf.pdg,subdf.status)
          #                    parents, children,
          #                subdf.vx, subdf.vy, subdf.vz, subdf.ct)
        for i in range(len(event.particles)):
            p=event.particles[i]
            v=pyhepmc.GenVertex(pyhepmc.FourVector(list(subdf.vx)[i], list(subdf.vy)[i], list(subdf.vz)[i], list(subdf.ct)[i]))
            v.add_particle_out(p)
            event.add_vertex(v)
            
        #del subdf

        f.write(event)
