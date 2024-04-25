#include "FastMemcpy.h"
#include "common.h"
#include "rdma.h"
#include <atomic>
#include <csignal>
#include <infiniband/verbs.h>
#include <json/value.h>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/tcpsocketserver.h>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <vector>

using jsonrpc::JSON_STRING;
using jsonrpc::PARAMS_BY_NAME;
using jsonrpc::Procedure;
using std::thread;
using std::vector;

// 和 cacheline 对齐的 int，避免多线程访问 int 数组冲突
struct __attribute__((aligned(64))) PaddingInt {
  int val_;
};

// 工作线程上下文
struct WorkerContext {
  int id_;
  // RDMA send CQ
  ibv_cq *send_cq_;
  // RDMA recv CQ
  ibv_cq *recv_cq_;
  // RDMA recv buffer
  char *buf_;
  // RDMA MR
  ibv_mr *mr_;
  // RDMA UD QP
  ibv_qp *qp_;
  // 信息交换时自己的 info
  RdmaExchangeInfo self_info_;
  // 模拟 memcpy 的目的地
  char small_buffer_[kPacketSize];

  // 初始化工作线程，返回成功与否
  bool InitContext(int id);
  // 销毁资源
  void DestroyContext();
};

// 服务端配置
struct ServerConfig {
  // RNIC 名
  char *dev_name_;
  // 网卡的 gid 索引下标
  int gid_idx_;
  // 服务端接收请求的端口号
  int server_port_;
  // 运行多少服务线程
  int thread_num_;
  // 从哪个核开始绑
  int start_core_;
};

struct ServerContext {
  // 从命令行读取的服务端配置
  ServerConfig cfg_;
  // 设备信息
  RdmaDeviceInfo dev_info_;
  // 实际的工作线程
  vector<thread> workers_;
  // 这些工作线程的上下文
  vector<WorkerContext *> ctxs_;
  // 线程初始化 barrier
  pthread_barrier_t barrier_;
  // 读取出来的 gid
  ibv_gid gid_;
  // 是否还没被 ctrl+c 或者客户端停止运行
  bool should_run_;
  // 每个 client 收到的包计数
  PaddingInt received_cnts_[kClientNumLimit];

  // 初始化 RDMA 环境和工作线程，返回成功与否
  void InitContext();
  // 销毁资源
  void DestroyContext();
} g_ctx;

bool WorkerContext::InitContext(int id) {
  id_ = id;

  // 创建 CQ
  send_cq_ =
      ibv_create_cq(g_ctx.dev_info_.ctx_, kRdmaCqSize, nullptr, nullptr, 0);
  if (send_cq_ == nullptr) {
    printf("Failed to create RDMA send CQ\n");
    return false;
  }
  recv_cq_ =
      ibv_create_cq(g_ctx.dev_info_.ctx_, kRdmaCqSize, nullptr, nullptr, 0);
  if (recv_cq_ == nullptr) {
    printf("Failed to create RDMA recv CQ\n");
    return false;
  }

  // 创建缓冲区
  int64_t buffer_size = kRdmaServerBufferUnitSize * kRdmaWQSize;
  buf_ = static_cast<char *>(aligned_alloc(4096, buffer_size));
  if (buf_ == nullptr) {
    printf("Failed to create RDMA buffer\n");
    return false;
  }
  memset(buf_, 0x3f, buffer_size);

  // 注册 MR
  int mr_flags =
      IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  mr_ = ibv_reg_mr(g_ctx.dev_info_.pd_, buf_, buffer_size, mr_flags);
  if (mr_ == nullptr) {
    printf("Failed to register memory region\n");
    return false;
  }

  // 创建 QP
  qp_ = RdmaCreateQP(g_ctx.dev_info_.pd_, send_cq_, recv_cq_, IBV_QPT_UD);
  if (qp_ == nullptr) {
    printf("Failed to create QP.\n");
    return false;
  }
  ModifyQpToInit(qp_);
  ModifyQpToRtr(qp_);
  ModifyQpToRts(qp_);

  // 取得信息
  self_info_.gid_ = g_ctx.gid_;
  self_info_.lid_ = g_ctx.dev_info_.port_attr_.lid;
  self_info_.qpn_ = qp_->qp_num;

  return true;
}

void WorkerContext::DestroyContext() {
  // QP
  ibv_destroy_qp(qp_);

  // CQ
  ibv_destroy_cq(send_cq_);
  ibv_destroy_cq(recv_cq_);

  // MR and buffer
  ibv_dereg_mr(mr_);
  free(buf_);
  buf_ = nullptr;
}

void ServerContext::InitContext() {
  // 获取并打开网卡设备、创建 PD
  dev_info_ = RdmaGetAndOpenDevice(cfg_.dev_name_);
  // 设置本地交换信息
  if (cfg_.gid_idx_ >= 0) {
    int ret =
        ibv_query_gid(dev_info_.ctx_, kRdmaDefaultPort, cfg_.gid_idx_, &gid_);
    if (ret) {
      printf("ibv_query_gid failed\n");
      exit(0);
    }
  } else {
    memset(&gid_, 0, sizeof(union ibv_gid));
  }
  for (auto &received_cnt : received_cnts_) {
    received_cnt.val_ = 0;
  }

  pthread_barrier_init(&barrier_, nullptr, cfg_.thread_num_ + 1);
  should_run_ = true;
}

void ServerContext::DestroyContext() {
  for (int i = 0; i < g_ctx.cfg_.thread_num_; i++) {
    ctxs_[i]->DestroyContext();
    delete ctxs_[i];
    ctxs_[i] = nullptr;
  }
  pthread_barrier_destroy(&barrier_);
  // PD and ctx
  ibv_dealloc_pd(dev_info_.pd_);
  ibv_close_device(dev_info_.ctx_);
}

void WorkerFunc(int id) {
  if (g_ctx.cfg_.start_core_ >= 0) {
    BindCore(id + g_ctx.cfg_.start_core_);
  }
  auto *w_ctx = new WorkerContext;
  if (!w_ctx->InitContext(id)) {
    printf("WorkerThread Init failed\n");
    exit(0);
  }
  g_ctx.ctxs_[id] = w_ctx;
  for (int i = 0; i < kRdmaWQSize; i++) {
    RdmaPostUdRecv(w_ctx->buf_ + i * kRdmaServerBufferUnitSize,
                   kRdmaServerBufferUnitSize, w_ctx->mr_->lkey, w_ctx->qp_, i);
  }
  pthread_barrier_wait(&g_ctx.barrier_); // 初始化完毕

  ibv_wc wc[kRdmaPollNum];
  while (g_ctx.should_run_) {
    int n;
    // send 什么也不做
    n = ibv_poll_cq(w_ctx->send_cq_, kRdmaPollNum, wc);
    if (n != 0) {
      printf("strange %d\n", n);
    }
    // recv
    n = ibv_poll_cq(w_ctx->recv_cq_, kRdmaPollNum, wc);
    for (int i = 0; i < n; i++) {
      if (wc[i].status != IBV_WC_SUCCESS) {
        printf("Error wc[i].status %d\n", wc[i].status);
        continue;
      }
      if (wc[i].opcode != IBV_WC_RECV) {
        printf("Error wc[i].opcode %d\n", wc[i].status);
        continue;
      }
      g_ctx.received_cnts_[wc[i].imm_data].val_++;
      memcpy_fast(w_ctx->small_buffer_,
             w_ctx->buf_ + wc[i].wr_id * kRdmaServerBufferUnitSize -
                 kPacketSize,
             kPacketSize);
      RdmaPostUdRecv(w_ctx->buf_ + wc[i].wr_id * kRdmaServerBufferUnitSize,
                     kRdmaServerBufferUnitSize, w_ctx->mr_->lkey, w_ctx->qp_,
                     wc[i].wr_id);
    }
  }
}

// master 程序的 JSON RPC 端
class ServerJrpcServer : public jsonrpc::AbstractServer<ServerJrpcServer> {
public:
  // 注册 RPC 函数
  explicit ServerJrpcServer(jsonrpc::TcpSocketServer &server);

  // client 根据自己 id 查询 AH
  void QueryAh(const Json::Value &req, Json::Value &resp);
  // 查询计数器
  void QueryCounters(const Json::Value &req, Json::Value &resp);
  // 关闭该进程
  void Shutdown(const Json::Value &req);
} *jrpc_server;

ServerJrpcServer::ServerJrpcServer(jsonrpc::TcpSocketServer &server)
    : AbstractServer<ServerJrpcServer>(server) {
  this->bindAndAddMethod(
      Procedure("query-ah", PARAMS_BY_NAME, JSON_STRING, nullptr),
      &ServerJrpcServer::QueryAh);
  this->bindAndAddMethod(
      Procedure("query-counters", PARAMS_BY_NAME, JSON_STRING, nullptr),
      &ServerJrpcServer::QueryCounters);
  this->bindAndAddNotification(Procedure("shutdown", PARAMS_BY_NAME, nullptr),
                               &ServerJrpcServer::Shutdown);
}

void ServerJrpcServer::QueryAh(const Json::Value &req, // NOLINT
                               Json::Value &resp) {
  int id = req["id"].asInt();
  printf("Found client %d\n", id);
  WorkerContext *w_ctx = g_ctx.ctxs_[id % g_ctx.cfg_.thread_num_];
  resp["gid"] = RdmaGid2Str(w_ctx->self_info_.gid_);
  resp["lid"] = w_ctx->self_info_.lid_;
  resp["qpn"] = w_ctx->self_info_.qpn_;
}

void ServerJrpcServer::QueryCounters(const Json::Value &req, // NOLINT
                                     Json::Value &resp) {
  for (int i = 0; i < kClientNumLimit; i++) {
    int64_t tmp = g_ctx.received_cnts_[i].val_;
    resp[std::to_string(i)] = static_cast<Json::Value::Int64>(tmp);
  }
}

void ServerJrpcServer::Shutdown(const Json::Value &req) { // NOLINT
  g_ctx.should_run_ = false;
  printf("Shutdown...\n");
}

void CtrlCHandler(int /*signum*/) { g_ctx.should_run_ = false; }

int main(int argc, char *argv[]) {
  signal(SIGTERM, CtrlCHandler);
  signal(SIGINT, CtrlCHandler);
  printf(
      "**Note that you should bind threads and RNIC at the same numa node!**\n");
  if (argc != 6) {
    printf("Usage: %s dev_name gid_index server_port thread_num start_core\n",
           argv[0]);
    return 0;
  }
  g_ctx.cfg_.dev_name_ = argv[1];
  g_ctx.cfg_.gid_idx_ = atoi(argv[2]);
  g_ctx.cfg_.server_port_ = atoi(argv[3]);
  g_ctx.cfg_.thread_num_ = atoi(argv[4]);
  g_ctx.cfg_.start_core_ = atoi(argv[5]);

  g_ctx.InitContext();
  // 创建线程
  g_ctx.ctxs_ = vector<WorkerContext *>(g_ctx.cfg_.thread_num_, nullptr);
  for (int i = 0; i < g_ctx.cfg_.thread_num_; i++) {
    g_ctx.workers_.emplace_back(WorkerFunc, i);
  }
  pthread_barrier_wait(&g_ctx.barrier_);
  printf("Init success, start running...\n");

  jsonrpc::TcpSocketServer tss("0.0.0.0", g_ctx.cfg_.server_port_);
  jrpc_server = new ServerJrpcServer(tss);
  if (!jrpc_server->StartListening()) {
    printf("start jrpc server failed\n");
  }

  for (int i = 0; i < g_ctx.cfg_.thread_num_; i++) {
    g_ctx.workers_[i].join();
  }
  jrpc_server->StopListening();
  g_ctx.DestroyContext();
  printf("Stopped\n");
  return 0;
}