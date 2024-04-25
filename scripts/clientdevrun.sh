#!/bin/bash
# Usage: %s dev_name gid_index server_name server_port
  # -mca odls_base_sigkill_timeout 2 \
mpirun -np 1 --hostfile scripts/mpi_host -mca pml ucx -mca btl ^vader,tcp,openib,uct \
  -x LD_LIBRARY_PATH=/home/cyx/melonfs/deps/install/lib \
  /home/cyx/projects/multi-ud-incast/build/mui_client mlx5_0 3 192.168.1.53 10733