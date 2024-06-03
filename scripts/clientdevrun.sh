#!/bin/bash
# Usage: %s dev_name gid_index server_name server_port
# 需要写 ipv4 地址
# numa 0: 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19
# numa 1: 22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39
mpirun -np 1 --hostfile scripts/mpi_host \
  --bind-to cpu-list:ordered --cpu-list 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 \
  -mca pml ucx -mca btl ^vader,tcp,openib,uct \
  -x LD_LIBRARY_PATH=/home/cyx/melonfs/deps/install/lib \
  /home/cyx/projects/multi-ud-incast/build/mui_client mlx5_0 3 192.168.1.53 10733