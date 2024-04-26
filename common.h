#ifndef POORPOOL_MUI_COMMON_H
#define POORPOOL_MUI_COMMON_H

#include <cstdint>
#include <infiniband/verbs.h>
#include <netinet/ip.h>

// 一次 UD 传输的包的大小。不能超过 UD MTU
constexpr int64_t kPacketSize = 4096;
// 客户端并发发送的 packet 数量上限
constexpr int kClientPacketNumLimit = 33;
// 最多能有几个 MPI 进程
constexpr int kClientNumLimit = 10;

static inline uint16_t ipv4_calc_hdr_csum(uint16_t *data,
                                          unsigned int num_hwords) {
  unsigned int i = 0;
  uint32_t sum = 0;

  for (i = 0; i < num_hwords; i++)
    sum += *(data++);

  sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}

// 检查 ibv_grh 实际上是哪个 IP 版本
static inline int get_grh_header_version(struct ibv_grh *grh) {
  int ip6h_version = (ntohl(grh->version_tclass_flow) >> 28) & 0xf;
  auto *ip4h =
      reinterpret_cast<struct iphdr *>(reinterpret_cast<char *>(grh) + 20);
  struct iphdr ip4h_checked;

  if (ip6h_version != 6) {
    if (ip4h->version == 4)
      return 4;
    return -1;
  }
  /* version may be 6 or 4 */
  if (ip4h->ihl != 5) /* IPv4 header length must be 5 for RoCE v2. */
    return 6;
  /*
   * Verify checksum.
   * We can't write on scattered buffers so we have to copy to temp
   * buffer.
   */
  memcpy(&ip4h_checked, ip4h, sizeof(ip4h_checked));
  /* Need to set the checksum field (check) to 0 before re-calculating
   * the checksum.
   */
  ip4h_checked.check = 0;
  ip4h_checked.check =
      ipv4_calc_hdr_csum(reinterpret_cast<uint16_t *>(&ip4h_checked), 10);
  /* if IPv4 header checksum is OK, believe it */
  if (ip4h->check == ip4h_checked.check)
    return 4;
  return 6;
}

#endif // POORPOOL_MUI_COMMON_H