#ifndef POORPOOL_RDMA_H
#define POORPOOL_RDMA_H

#include <arpa/inet.h>
#include <byteswap.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <infiniband/verbs.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

// RDMA 网卡使用网卡的哪个端口（一般都是 1）
constexpr int kRdmaDefaultPort = 1;
// RDMA SQ/RQ size
constexpr int kRdmaWQSize = 4096;
// RDMA CQ size。如果共用 CQ，那应该是两倍的 QP size。否则相等就行
constexpr int kRdmaCqSize = 4096;
// RDMA 最大内联消息长度
// 根据 guideline 和唐鼎测试，是 cacheline_size - WQE_header_size
constexpr int kRdmaMaxInlineSize = 256 - 68;

constexpr int PACKET_SIZE = 10;
constexpr int BUF_SIZE = 100 * 1024 * 1064;

struct config_t {
  const char *dev_name; /* IB设备名称 */
  char *server_name;    /* 服务器主机名 */
  u_int32_t tcp_port;   /* 服务器TCP端口 */
  int ib_port;          /* 本地IB端口 */
  int gid_idx;          /* GID索引 */
};

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

struct RdmaRCConnExchangeInfo {
  uint64_t addr;   /* 缓冲区地址 */
  uint32_t rkey;   /* 远程键 */
  uint16_t lid;    /* IB端口的LID */
  uint32_t qp_num; /* QP号 */
  // TODO: 改为 union ibv_gid gid; 并添加gid_idx
  uint8_t gid[16]; /* GID */
};

struct RdmaUDConnExchangeInfo {
  uint64_t addr; /* 缓冲区地址 */
  uint32_t qkey; /* 远程键 */
  uint16_t lid;  /* IB端口的LID */
  uint32_t qpn;  /* QP号 */
  uint32_t psn;
  union ibv_gid gid;
  int gid_idx;
};

// 根据传入的网卡名来查找、打开 RDMA 设备并创建 PD
RdmaDeviceInfo RdmaGetAndOpenDevice(const std::string &device_name);
// 创建 RDMA QP
ibv_qp *RdmaCreateQP(ibv_pd *pd, ibv_cq *send_cq, ibv_cq *recv_cq,
                     ibv_qp_type qp_type);

ibv_ah *CreateAH(ibv_pd *pd, int port, int sl, RdmaUDConnExchangeInfo dest,
                 int sgid_idx);
int sock_sync_data(int sock, int xfer_size, char *local_data,
                   char *remote_data);
int modify_qp_to_init(struct ibv_qp *qp, RdmaUDConnExchangeInfo remote_info);
int modify_qp_to_rtr(struct ibv_qp *qp, RdmaUDConnExchangeInfo remote_info);
int modify_qp_to_rts(struct ibv_qp *qp, RdmaUDConnExchangeInfo remote_info);
int post_recv(const void *buf, uint32_t len, uint32_t lkey, ibv_qp *qp,
              int wr_id);
int post_send(const void *buf, uint32_t len, uint32_t lkey, ibv_qp *qp,
              int wr_id, enum ibv_wr_opcode opcode, ibv_ah *local_ah,
              RdmaUDConnExchangeInfo remote_info);
int poll_completion(ibv_cq *cq);

// RDMA gid 转换成字符串
std::string RdmaGid2Str(ibv_gid gid);
// 将字符串转换成 RDMA gid
ibv_gid RdmaStr2Gid(const std::string &s);
#endif // POORPOOL_RDMA_H