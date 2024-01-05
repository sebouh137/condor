rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/local    #$EIC_SHELL_PREFIX
make install -j8
