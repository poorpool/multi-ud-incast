#!/bin/bash
mpirun -np 4 -mca pml ucx -mca btl ^vader,tcp,openib,uct \
  ./build/mui_client
