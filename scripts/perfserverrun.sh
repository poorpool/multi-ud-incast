#!/bin/bash
export LD_LIBRARY_PATH=/home/cyx/melonfs/deps/install/lib
sudo perf record --call-graph fp -a -g -e cpu-clock \
  ./build/mui_server mlx5_0 3 10733 1 5
