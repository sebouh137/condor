dir=`pwd`
cd ~/corsika-78000/run/

# remove previous output file
rm ./DAT000006 ./DAT000006.long
# run corsika
./corsika78000Linux_EPOS_urqmd < ${dir}/all-inputs-epos-condor
# move output here
mv ./DAT000006 ./DAT000006.long ${dir}
# convert 
cd ../src/utils/coast/CorsikaRead
./Corsika2Hepmc ${dir}/DAT000006 ${dir}/out.hepmc
