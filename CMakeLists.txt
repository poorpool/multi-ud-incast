cmake_minimum_required(VERSION 3.11.0)
project(multi-cast-ud VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -ggdb -fno-omit-frame-pointer -fno-inline-functions")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3")

set(MELON_DEPENDENCY_PREFIX "/home/cyx/melonfs/deps/install")
set(CMAKE_PREFIX_PATH ${MELON_DEPENDENCY_PREFIX})

find_package(libjson-rpc-cpp REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(MPI REQUIRED)

add_executable(mui_server
  mui_server.cc
  rdma.cc
  get_clock.cc
)
target_link_libraries(mui_server PRIVATE
  libjson-rpc-cpp::jsonrpcserver
  jsoncpp_lib
  ibverbs
  numa
)

add_executable(mui_client
  mui_client.cc
  rdma.cc
  get_clock.cc
)
target_link_libraries(mui_client PRIVATE
  MPI::MPI_CXX
  libjson-rpc-cpp::jsonrpcclient
  jsoncpp_lib
  ibverbs
  pthread
  numa
)