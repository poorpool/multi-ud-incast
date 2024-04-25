#include "rdma.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <infiniband/verbs.h>
#include <string>

using std::string;

string RdmaGid2Str(ibv_gid gid) {
  string res;
  for (unsigned char i : gid.raw) {
    char s[3];
    sprintf(s, "%02X", i);
    res += string(s);
  }
  return res;
}

// 将字符 ch 转换成十六进制格式
char get_xdigit(char ch) {
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  return -1;
}

ibv_gid RdmaStr2Gid(const std::string &s) {
  union ibv_gid gid;
  for (int32_t i = 0; i < 16; i++) {
    unsigned char x;
    x = get_xdigit(s[i * 2]);
    gid.raw[i] = x << 4;
    x = get_xdigit(s[i * 2 + 1]);
    gid.raw[i] |= x;
  }
  return gid;
}

RdmaDeviceInfo RdmaGetAndOpenDevice(const string &device_name) {
  int num_devices;
  ibv_device **dev_list = ibv_get_device_list(&num_devices);
  if (dev_list == nullptr) {
    printf("Failed to get RDMA device list.\n");
    return {};
  }

  RdmaDeviceInfo rdi;

  // 找指定设备
  ibv_device *ib_dev = nullptr;
  for (int i = 0; i < num_devices; i++) {
    if (!strcmp(ibv_get_device_name(dev_list[i]), device_name.c_str())) {
      ib_dev = dev_list[i];
      break;
    }
  }
  if (ib_dev == nullptr) {
    printf("RDMA device %s not found.\n", device_name.c_str());
    return {};
  }
  rdi.ctx_ = ibv_open_device(ib_dev);
  if (rdi.ctx_ == nullptr) {
    printf("Failed to open RDMA device %s.\n", device_name.c_str());
    return {};
  }

  // 查询设备属性
  ibv_query_device(rdi.ctx_, &rdi.device_attr_);

  // 查询端口属性
  ibv_query_port(rdi.ctx_, kRdmaDefaultPort, &rdi.port_attr_);

  // 创建 PD
  rdi.pd_ = ibv_alloc_pd(rdi.ctx_);
  if (rdi.pd_ == nullptr) {
    printf("Failed to create RDMA PD %s.\n", device_name.c_str());
    return {};
  }

  ibv_free_device_list(dev_list);
  dev_list = nullptr;
  ib_dev = nullptr;

  return rdi;
}

ibv_ah *RdmaCreateAh(ibv_pd *pd, RdmaExchangeInfo &dest, int sgid_idx) {
  ibv_ah_attr ah_attr;
  memset(&ah_attr, 0, sizeof(ah_attr));
  ah_attr.dlid = dest.lid_;
  ah_attr.sl = kRdmaDefaultServiceLevel;
  ah_attr.is_global = 0;
  ah_attr.port_num = kRdmaDefaultPort;
  if (dest.gid_.global.interface_id) {
    ah_attr.is_global = 1;
    ah_attr.grh.hop_limit = 1;
    ah_attr.grh.dgid = dest.gid_;
    ah_attr.grh.sgid_index = sgid_idx;
  }
  return ibv_create_ah(pd, &ah_attr);
}

ibv_qp *RdmaCreateQP(ibv_pd *pd, ibv_cq *send_cq, ibv_cq *recv_cq,
                     ibv_qp_type qp_type) {
  ibv_qp_init_attr qp_init_attr;
  memset(&qp_init_attr, 0, sizeof(qp_init_attr));
  qp_init_attr.qp_type = qp_type;
  qp_init_attr.sq_sig_all = 1;                // 向所有 WR 发送信号
  qp_init_attr.send_cq = send_cq;             // 发送完成队列
  qp_init_attr.recv_cq = recv_cq;             // 接收完成队列
  qp_init_attr.cap.max_send_wr = kRdmaWQSize; // 最大发送 WR 数量
  qp_init_attr.cap.max_recv_wr = kRdmaWQSize; // 最大接收 WR 数量
  qp_init_attr.cap.max_send_sge = 1;          // 最大发送 SGE 数量
  qp_init_attr.cap.max_recv_sge = 1;          // 最大接收 SGE 数量
  qp_init_attr.cap.max_inline_data = kRdmaMaxInlineSize;
  return ibv_create_qp(pd, &qp_init_attr);
}

int ModifyQpToInit(struct ibv_qp *qp) {
  struct ibv_qp_attr attr;
  memset(&attr, 0, sizeof(ibv_qp_attr));

  attr.qp_state = IBV_QPS_INIT;
  attr.qkey = kRdmaDefaultQKey;
  attr.pkey_index = 0;
  attr.port_num = kRdmaDefaultPort;

  int ret = ibv_modify_qp(
      qp, &attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_QKEY);
  if (ret != 0) {
    printf("ibv_modify_qp to INIT failed %d", ret);
  }
  return ret;
}

int ModifyQpToRtr(struct ibv_qp *qp) {
  struct ibv_qp_attr attr;
  memset(&attr, 0, sizeof(ibv_qp_attr));

  attr.qp_state = IBV_QPS_RTR;

  int ret = ibv_modify_qp(qp, &attr, IBV_QP_STATE);
  if (ret) {
    printf("ibv_modify_qp to RTR failed %d", ret);
  }
  return ret;
}

int ModifyQpToRts(struct ibv_qp *qp) {
  struct ibv_qp_attr attr;
  memset(&attr, 0, sizeof(attr));

  attr.qp_state = IBV_QPS_RTS;
  attr.sq_psn = 0;
  int ret = ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_SQ_PSN);
  if (ret) {
    printf("ibv_modify_qp to RTS failed %d", ret);
  }
  return ret;
}

int RdmaPostUdRecv(const void *buf, uint32_t len, uint32_t lkey, ibv_qp *qp,
                   int wr_id) {
  int ret = 0;
  struct ibv_recv_wr *bad_wr = nullptr;

  struct ibv_sge list;
  memset(&list, 0, sizeof(ibv_sge));
  list.addr = reinterpret_cast<uint64_t>(buf);
  list.length = len;
  list.lkey = lkey;

  struct ibv_recv_wr wr;
  memset(&wr, 0, sizeof(ibv_recv_wr));
  wr.wr_id = wr_id;
  wr.next = nullptr;
  wr.sg_list = &list;
  wr.num_sge = 1;

  ret = ibv_post_recv(qp, &wr, &bad_wr);
  if (ret != 0) {
    printf("ibv_post_recv failed %d\n", ret);
  }
  return ret;
}

int RdmaPostUdSend(const void *buf, uint32_t len, uint32_t lkey, ibv_qp *qp,
                   int wr_id, ibv_ah *ah, RdmaExchangeInfo &dest,
                   uint32_t imm_data) {
  int ret = 0;
  struct ibv_send_wr *bad_wr = nullptr;

  struct ibv_sge list;
  memset(&list, 0, sizeof(ibv_sge));
  list.addr = reinterpret_cast<uint64_t>(buf);
  list.length = len;
  list.lkey = lkey;

  struct ibv_send_wr wr;
  memset(&wr, 0, sizeof(ibv_send_wr));
  wr.wr_id = wr_id;
  wr.next = nullptr;
  wr.sg_list = &list;
  wr.num_sge = 1;
  wr.imm_data = imm_data;
  wr.opcode = IBV_WR_SEND_WITH_IMM;
  wr.send_flags = IBV_SEND_SIGNALED;
  wr.wr.ud.ah = ah;
  wr.wr.ud.remote_qkey = kRdmaDefaultQKey;
  wr.wr.ud.remote_qpn = dest.qpn_;

  ret = ibv_post_send(qp, &wr, &bad_wr);
  if (ret != 0 || bad_wr != nullptr) {
    printf("ibv_post_send failed %d %p %p\n", ret, &wr, bad_wr);
  }
  return ret;
}

void BindCore(int core) {
  if (core < 0) {
    return;
  }
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  CPU_SET(core + 30, &cpuset);

  pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}