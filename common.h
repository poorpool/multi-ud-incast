#ifndef POORPOOL_MUI_COMMON_H
#define POORPOOL_MUI_COMMON_H

#include <cstdint>

// 一次 UD 传输的包的大小。不能超过 UD MTU
constexpr int64_t kPacketSize = 4096;
// 客户端并发发送的 packet 数量上限
constexpr int kClientPacketNumLimit = 65;
// 最多能有几个 MPI 进程
constexpr int kClientNumLimit = 10;

#endif // POORPOOL_MUI_COMMON_H