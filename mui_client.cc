#include "common.h"
#include "rdma.h"
#include <csignal>
#include <cstddef>
#include <infiniband/verbs.h>
#include <iostream>
#include <mpi.h>
#include <string>

using std::string;

// 客户端配置
struct ClientConfig {
  // RNIC 名
  char *dev_name_;
  // 网卡的 gid 索引下标
  int gid_idx_;
  // 服务端 IP 或主机名
  char *server_name_;
  // 服务端接收请求的端口号
  int server_port_;
};

// 客户端全局上下文
struct ClientContext {
  // 从命令行中读取的配置
  ClientConfig cfg_;
  // 相当于 ID
  int mpi_rank_;
  // 整个 MPI 世界的 size
  int mpi_size_;
  // 主机名
  string hostname_;
  // 是否还没被 ctrl+c 停止
  bool should_run_;

  // 打开的 RNIC 的信息
  RdmaDeviceInfo dev_info_;
  // RDMA send CQ
  ibv_cq *send_cq_;
  // RDMA recv CQ
  ibv_cq *recv_cq_;
  // RDMA send buffer
  char *buf_;
  // RDMA MR
  ibv_mr *mr_;
  // RDMA UD QP
  ibv_qp *qp_;

  // 初始化 RDMA 环境，返回成功与否
  bool InitContext();
  // 销毁资源
  void DestroyContext();
} g_ctx;

bool ClientContext::InitContext() {
  // 获取并打开网卡设备、创建 PD
  dev_info_ = RdmaGetAndOpenDevice(cfg_.dev_name_);

  // 创建 CQ
  send_cq_ = ibv_create_cq(dev_info_.ctx_, kRdmaCqSize, nullptr, nullptr, 0);
  if (send_cq_ == nullptr) {
    printf("Failed to create RDMA send CQ\n");
    return false;
  }
  recv_cq_ = ibv_create_cq(dev_info_.ctx_, kRdmaCqSize, nullptr, nullptr, 0);
  if (recv_cq_ == nullptr) {
    printf("Failed to create RDMA recv CQ\n");
    return false;
  }

  // 创建缓冲区
  int64_t buffer_size = kPacketSize * kClientPacketNumLimit;
  buf_ = static_cast<char *>(aligned_alloc(4096, buffer_size));
  if (buf_ == nullptr) {
    printf("Failed to create RDMA buffer\n");
    return false;
  }
  memset(buf_, 0x3f, buffer_size);

  // 注册 MR
  int mr_flags =
      IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  mr_ = ibv_reg_mr(dev_info_.pd_, buf_, buffer_size, mr_flags);
  if (mr_ == nullptr) {
    printf("Failed to register memory region\n");
    return false;
  }

  // 创建 QP
  qp_ = RdmaCreateQP(dev_info_.pd_, send_cq_, recv_cq_, IBV_QPT_UD);
  if (qp_ == nullptr) {
    printf("Failed to create QP.\n");
    return false;
  }
  ModifyQpToInit(qp_);
  ModifyQpToRtr(qp_);
  ModifyQpToRts(qp_);

  should_run_ = true;
  return true;
}

void ClientContext::DestroyContext() {
  // QP
  ibv_destroy_qp(qp_);

  // CQ
  ibv_destroy_cq(send_cq_);
  ibv_destroy_cq(recv_cq_);

  // MR and buffer
  ibv_dereg_mr(mr_);
  free(buf_);
  buf_ = nullptr;

  // PD and ctx
  ibv_dealloc_pd(dev_info_.pd_);
  ibv_close_device(dev_info_.ctx_);
}

void CtrlCHandler(int  /*signum*/) {
  g_ctx.should_run_ = false;
  if (g_ctx.mpi_rank_ == 0) {
    printf("received ctrl+c, try to stop...");
  }
}

int main(int argc, char *argv[]) {
  signal(SIGTERM, CtrlCHandler);
  signal(SIGINT, CtrlCHandler);
  if (argc != 5) {
    printf("Usage: %s dev_name gid_index server_name server_port\n", argv[0]);
    return 0;
  }
  g_ctx.cfg_.dev_name_ = argv[1];
  g_ctx.cfg_.gid_idx_ = atoi(argv[2]);
  g_ctx.cfg_.server_name_ = argv[3];
  g_ctx.cfg_.server_port_ = atoi(argv[4]);

  // MPI init
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &g_ctx.mpi_size_);
  MPI_Comm_rank(MPI_COMM_WORLD, &g_ctx.mpi_rank_);
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);
  g_ctx.hostname_ = processor_name;

  if (!g_ctx.InitContext()) {
    MPI_Finalize();
    return 0;
  }

  for (int i = 0; i < kClientPacketNumLimit; i++) {
    RdmaPostUdSend(g_ctx.buf_ + i * kPacketSize, kPacketSize, lkey, g_ctx.qp_,
                   i, ah, remote_info);
  }

  ibv_wc wc[kRdmaPollNum];
  int64_t unfinished_cnt = kClientPacketNumLimit;
  int64_t finished_cnt = 0;
  MPI_Barrier(MPI_COMM_WORLD);
  // 计时
  while (g_ctx.should_run_ || unfinished_cnt > 0) {
    int n;
    // recv 什么也不做
    n = ibv_poll_cq(g_ctx.recv_cq_, kRdmaPollNum, wc);
    // send 检查能不能继续发
    n = ibv_poll_cq(g_ctx.recv_cq_, kRdmaPollNum, wc);
    for (int i = 0; i < n; i++) {
      if (wc[i].status != IBV_WC_SUCCESS) {
        printf("Error wc[i].status %d\n", wc[i].status);
        continue;
      }
      if (wc[i].opcode != IBV_WC_SEND) {
        printf("Error wc[i].opcode %d\n", wc[i].status);
        continue;
      }
      unfinished_cnt--;
      finished_cnt++;
      RdmaPostUdSend(const void *buf, uint32_t len, uint32_t lkey, ibv_qp *qp,
                     int wr_id, ibv_ah *ah, RdmaUDConnExchangeInfo remote_info)
    }
    int sn =
  }
  MPI_Barrier(MPI_COMM_WORLD);
  // 计时

  g_ctx.DestroyContext();
  MPI_Finalize();

  return 0;
}