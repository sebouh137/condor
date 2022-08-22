#adds the fcal hitprocess to GEMC, and then recompiles it
cp gemc_mod/condor_hitprocess*  $GEMC/hitprocess/
cp gemc_mod/SConstruct $GEMC
cp gemc_mod/HitProcess_MapRegister.cc $GEMC/hitprocess/
cd $GEMC
scons -j 8

