with open("elevations.txt","w") as f:
    #print("#row,column,elevation",file=f)
    for i in range(14):
        for j in range(14):
            elevation=(i-7)*6+(j-7)*10+(j-7)**2*0.3
            print(i+1,j+1, elevation,sep=',',file=f)
