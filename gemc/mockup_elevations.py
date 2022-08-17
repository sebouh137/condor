import numpy as np

#relative elevations of the 4 corners
SW=0
SE=23.9
NW=17.0
NE=32.5

ncol=14
nrow=14

#slopes
ai=(SE+NE-SW-NW)/(2*(ncol-1))
aj=(NW+NE-SW-SE)/(2*(nrow-1))
#quadratic term
aij=(NE-NW+SW-SE)/((ncol-1)*(nrow-1))

all_elevations=[]
with open("elevations.txt","w") as f:
    #print("#column, row ,elevation",file=f)
    for i in range(ncol):
        for j in range(nrow):
            elevation=(i-(ncol-1)/2)*ai+(j-(nrow-1)/2)*aj+(i-(ncol-1)/2)*(j-(nrow-1)/2)*aij
            print(i+1,j+1, elevation,sep=',',file=f)
            all_elevations.append(elevation)
print("max elevation", np.max(all_elevations))
