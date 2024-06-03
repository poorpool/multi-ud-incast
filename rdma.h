#ifndef POORPOOL_RDMA_H
#define POORPOOL_RDMA_H

#include "common.h"
#include <infiniband/verbs.h>
#include <string>

// RDMA 网卡使用网卡的哪个端口（一般都是 1）
constexpr int kRdmaDefaultPort = 1;
// 默认 service level
constexpr int kRdmaDefaultServiceLevel = 0;
// RDMA SQ/RQ size。并不是越大越好
constexpr int kRdmaWQSize = 128;
// RDMA CQ size。如果共用 CQ，那应该是两倍的 QP size。否则相等就行
constexpr int kRdmaCqSize = 128;
// RDMA 最大内联消息长度
// 根据 guideline 和唐鼎测试，是 cacheline_size - WQE_header_size
constexpr int kRdmaMaxInlineSize = 256 - 68;
// 一下子 poll 几个 WC
constexpr int kRdmaPollNum = 8;
// 全部采用一样的 QKey，就不生成了
constexpr uint32_t kRdmaDefaultQKey = 114514;
// server UD recv 的缓冲区单元大小
constexpr int64_t kRdmaServerBufferUnitSize = sizeof(ibv_grh) + kPacketSize;

// 查找并打开 RDMA 设备的结果
struct RdmaDeviceInfo {
  // 该设备上下文
  ibv_context *ctx_;
  // 该设备 PD
  ibv_pd *pd_;
  // 该设备属性
  ibv_device_attr device_attr_;
  // 查询的端口的属性
  ibv_port_attr port_attr_;
};

// JSON RPC 时交换的信息
struct RdmaExchangeInfo {
  uint16_t lid_;
  uint32_t qpn_;
  union ibv_gid gid_;
};

// RDMA gid 转换成字符串
std::string RdmaGid2Str(ibv_gid gid);
// 将字符串转换成 RDMA gid
ibv_gid RdmaStr2Gid(const std::string &s);

// 根据传入的网卡名来查找、打开 RDMA 设备并创建 PD
RdmaDeviceInfo RdmaGetAndOpenDevice(const std::string &device_name);
// 创建 RDMA UD AH
ibv_ah *RdmaCreateAh(ibv_pd *pd, RdmaExchangeInfo &dest, int sgid_idx);
// 创建 RDMA QP
ibv_qp *RdmaCreateQP(ibv_pd *pd, ibv_cq *send_cq, ibv_cq *recv_cq,
                     ibv_qp_type qp_type);
// 将 UD QP 状态调整为 INIT
int ModifyQpToInit(struct ibv_qp *qp);
// 将 UD QP 状态调整为 RTR
int ModifyQpToRtr(struct ibv_qp *qp);
// 将 UD QP 状态调整为 RTS
int ModifyQpToRts(struct ibv_qp *qp);
// 下发 UD recv WR
int RdmaPostUdRecv(const void *buf, uint32_t len, uint32_t lkey, ibv_qp *qp,
                   int wr_id);
// 下发 UD send WR
int RdmaPostUdSend(const void *buf, uint32_t len, uint32_t lkey, ibv_qp *qp,
                   int wr_id, ibv_ah *ah, RdmaExchangeInfo &dest,
                   uint32_t imm_data);
// 绑定线程到 core 上
void BindCore(int core);

int get_numa_node(void *memory);
#endif // POORPOOL_RDMA_H