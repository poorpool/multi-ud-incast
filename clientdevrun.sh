#!/bin/bash
export LD_LIBRARY_PATH=/home/cyx/melonfs/deps/install/lib
mpirun -np 4 -mca pml ucx -mca btl ^vader,tcp,openib,uct \
  ./build/mui_client
