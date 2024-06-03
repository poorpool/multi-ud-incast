# UD 多打一测试

## 结论

1. 一定要把 server 线程绑到对应的 numa node 上，不然性能挂逼了！绑定正确的 NUMA 才能 enable DDIO，不然单线程受到内存中的 memcpy 速度限制 https://github.com/poorpool/memcpy-vs-loopback。
2. transmit depth=17 的时候性能比较一般，=33的时候性能就很好了，基本能持平 ib_send_bw；
3. 其他内容调整好了以后，加上 memcpy 性能损失不是特别大，但也是有的。memcpy 对没对齐问题也不大（当然，最好还是能和 4、8、16 这样的数字对齐）。

## 测试结果

使用下列命令来结束 clients：

```bash
kill -usr1 `pgrep mpirun`
```

1 个进程都是 s52->s53，9 个进程都是 s51, s52, s53 各 3 个

### 单进程各 MTU

10 个服务线程，1 个 client 进程，MTU 4096，depth 65

```
Passed 2.46 seconds
  #0 11.09 GiB/s with 0.01% lost. Sended 7159024, received 7158636, ecn marked 0 
In total, 11.09 GiB/s, 0.01% lost, send 2.91 Mops, recv 2.91 Mops, ecn marked rate 0.00%
```

**10 个服务线程，1 个 client 进程，MTU 4096，depth 33**。这个最重要，证明了这个框架是有能力打满的，且 memcpy 不消耗带宽

```
Passed 5.32 seconds
  #0 10.60 GiB/s with 0.01% lost. Sended 14779529, received 14778228, 
In total, 10.60 GiB/s, 0.01% lost, send 2.78 Mops, recv 2.78 Mops
```


10 个服务线程，1 个 client 进程，MTU 4096，depth 17（深度小！）

```
Passed 4.78 seconds
  #0 8.62 GiB/s with 0.00% lost. Sended 10800587, received 10800348, 
In total, 8.62 GiB/s, 0.00% lost, send 2.26 Mops, recv 2.26 Mops
```

10 个服务线程，1 个 client 进程，MTU 2048，depth 33

```
Passed 9.21 seconds
  #0 7.78 GiB/s with 0.00% lost. Sended 37551843, received 37551619, 
In total, 7.78 GiB/s, 0.00% lost, send 4.08 Mops, recv 4.08 Mops
```

10 个服务线程，1 个 client 进程，MTU 1024，depth 65

```
Passed 11.35 seconds
  #0 4.27 GiB/s with 0.00% lost. Sended 50839160, received 50839160, 
In total, 4.27 GiB/s, 0.00% lost, send 4.48 Mops, recv 4.48 Mops
```

### 多进程各 MTU

10 个服务线程，9 个 client 进程，MTU 4096，depth 65

```
Passed 6.33 seconds
  #0 2.05 GiB/s with 0.00% lost. Sended 3402479, received 3402479, 
  #1 2.14 GiB/s with 0.00% lost. Sended 3547561, received 3547561, 
  #2 2.18 GiB/s with 0.00% lost. Sended 3617442, received 3617271, 
  #3 1.08 GiB/s with 73.05% lost. Sended 6635077, received 1788070, 
  #4 1.01 GiB/s with 73.16% lost. Sended 6256673, received 1678997, 
  #5 1.05 GiB/s with 73.11% lost. Sended 6488951, received 1744555, 
  #6 1.10 GiB/s with 72.44% lost. Sended 6598005, received 1818512, 
  #7 1.10 GiB/s with 72.39% lost. Sended 6602808, received 1822721, 
  #8 1.04 GiB/s with 72.46% lost. Sended 6271097, received 1726774, 
In total, 12.74 GiB/s, 57.21% lost, send 7.81 Mops, recv 3.34 Mops
```

**10 个服务线程，9 个 client 进程，MTU 4096，depth 33**。所有测试丢包率为 0 的都是目标网卡所在交换机

```
Passed 13.16 seconds
  #0 2.05 GiB/s with 0.00% lost. Sended 7080303, received 7080303, 
  #1 2.20 GiB/s with 0.00% lost. Sended 7589294, received 7589294, 
  #2 2.20 GiB/s with 0.01% lost. Sended 7607791, received 7607410, 
  #3 1.07 GiB/s with 73.04% lost. Sended 13690769, received 3691409, 
  #4 1.06 GiB/s with 73.05% lost. Sended 13532629, received 3646401, 
  #5 1.07 GiB/s with 73.06% lost. Sended 13654052, received 3679004, 
  #6 1.09 GiB/s with 72.46% lost. Sended 13695528, received 3772427, 
  #7 1.09 GiB/s with 72.46% lost. Sended 13689526, received 3769725, 
  #8 1.08 GiB/s with 72.49% lost. Sended 13529521, received 3722267, 
In total, 12.91 GiB/s, 57.18% lost, send 7.91 Mops, recv 3.38 Mops
```

10 个服务线程，9 个 client 进程，MTU 4096，depth 17

```
Passed 26.50 seconds
  #0 2.06 GiB/s with 0.00% lost. Sended 14285315, received 14285315, 
  #1 2.07 GiB/s with 0.00% lost. Sended 14362994, received 14362994, 
  #2 2.06 GiB/s with 0.00% lost. Sended 14343984, received 14343798, 
  #3 1.02 GiB/s with 73.00% lost. Sended 26278961, received 7094570, 
  #4 1.02 GiB/s with 73.00% lost. Sended 26212817, received 7078288, 
  #5 1.02 GiB/s with 73.01% lost. Sended 26282224, received 7093831, 
  #6 1.05 GiB/s with 72.37% lost. Sended 26361526, received 7284321, 
  #7 1.04 GiB/s with 72.41% lost. Sended 26130100, received 7210491, 
  #8 1.04 GiB/s with 72.41% lost. Sended 26221665, received 7234593, 
In total, 12.38 GiB/s, 57.11% lost, send 7.57 Mops, recv 3.24 Mops
```

10 个服务线程，9 个 client 进程，MTU 2048，depth 33

```
Passed 12.67 seconds
  #0 1.98 GiB/s with 0.00% lost. Sended 13181842, received 13181842, 
  #1 1.98 GiB/s with 0.00% lost. Sended 13160707, received 13160707, 
  #2 1.97 GiB/s with 0.00% lost. Sended 13089465, received 13089465, 
  #3 0.98 GiB/s with 73.58% lost. Sended 24506412, received 6475750, 
  #4 0.97 GiB/s with 73.57% lost. Sended 24470148, received 6468121, 
  #5 0.98 GiB/s with 73.57% lost. Sended 24535372, received 6484849, 
  #6 1.00 GiB/s with 72.86% lost. Sended 24571975, received 6667644, 
  #7 1.00 GiB/s with 72.87% lost. Sended 24541256, received 6658803, 
  #8 1.01 GiB/s with 72.86% lost. Sended 24633116, received 6684635, 
In total, 11.88 GiB/s, 57.75% lost, send 14.74 Mops, recv 6.23 Mops
```

10 个服务线程，9 个 client 进程，MTU 1024，depth 65

```
Passed 8.37 seconds
  #0 1.84 GiB/s with 0.00% lost. Sended 16133460, received 16133460, 
  #1 1.82 GiB/s with 0.00% lost. Sended 16001636, received 16001636, 
  #2 1.84 GiB/s with 0.00% lost. Sended 16148569, received 16148486, 
  #3 0.90 GiB/s with 72.71% lost. Sended 29078007, received 7934021, 
  #4 0.91 GiB/s with 72.73% lost. Sended 29444264, received 8029635, 
  #5 0.90 GiB/s with 72.76% lost. Sended 28933228, received 7882682, 
  #6 0.93 GiB/s with 72.13% lost. Sended 29156234, received 8125537, 
  #7 0.94 GiB/s with 72.14% lost. Sended 29478771, received 8213844, 
  #8 0.92 GiB/s with 72.14% lost. Sended 29108786, received 8108869, 
In total, 11.00 GiB/s, 56.78% lost, send 26.70 Mops, recv 11.54 Mops
```

### ECN 比率

10 个服务线程，9 个 client 进程，MTU 4096，depth 33。

竟然只标记 0.24% 的包？但是其实打开交换机的会发现，可能它很多标记 ECN 的包都赶上了那个微突发的时间然后丢失了。由此可见，进行流量控制还是很有必要的。

```
Passed 15.80 seconds
  #0 2.68 GiB/s with 0.00% lost. Sended 11113286, received 11113286, ecn marked 0 
  #1 0.81 GiB/s with 0.00% lost. Sended 3363067, received 3363067, ecn marked 0 
  #2 2.68 GiB/s with 0.00% lost. Sended 11115763, received 11115674, ecn marked 0 
  #3 0.42 GiB/s with 45.84% lost. Sended 3191237, received 1728323, ecn marked 11296 
  #4 1.95 GiB/s with 45.69% lost. Sended 14880765, received 8081492, ecn marked 35740 
  #5 0.42 GiB/s with 45.81% lost. Sended 3178312, received 1722473, ecn marked 9099 
  #6 1.50 GiB/s with 45.72% lost. Sended 11446110, received 6212543, ecn marked 28040 
  #7 0.40 GiB/s with 45.82% lost. Sended 3031085, received 1642141, ecn marked 9489 
  #8 1.50 GiB/s with 45.72% lost. Sended 11435355, received 6206615, ecn marked 27562 
In total, 12.35 GiB/s, 29.65% lost, send 4.60 Mops, recv 3.24 Mops, ecn marked rate 0.24%
```

### ib_send_bw UD

MTU 4096

```
cyx@s52:~/projects$ ib_send_bw -D 10 s53 -m 4096 -c UD
 WARNING: BW peak won't be measured in this run.
 Max msg size in UD is MTU 4096
 Changing to this MTU
---------------------------------------------------------------------------------------
                    Send BW Test
 Dual-port       : OFF          Device         : mlx5_0
 Number of qps   : 1            Transport type : IB
 Connection type : UD           Using SRQ      : OFF
 PCIe relax order: ON
 ibv_wr* API     : ON
 TX depth        : 128
 CQ Moderation   : 100
 Mtu             : 4096[B]
 Link type       : Ethernet
 GID index       : 3
 Max inline data : 0[B]
 rdma_cm QPs     : OFF
 Data ex. method : Ethernet
---------------------------------------------------------------------------------------
 local address: LID 0000 QPN 0x2e74 PSN 0xc0f953
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:52
 remote address: LID 0000 QPN 0x5eb1 PSN 0x6303f9
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:53
---------------------------------------------------------------------------------------
 #bytes     #iterations    BW peak[MB/sec]    BW average[MB/sec]   MsgRate[Mpps]
Conflicting CPU frequency values detected: 800.565000 != 3311.538000. CPU Frequency is not max.
 4096       17338400         0.00               11288.12                   2.889760
---------------------------------------------------------------------------------------
```

MTU 2048

```
cyx@s52:~/projects$ ib_send_bw -D 10 s53 -m 2048 -c UD
 WARNING: BW peak won't be measured in this run.
 Max msg size in UD is MTU 2048
 Changing to this MTU
---------------------------------------------------------------------------------------
                    Send BW Test
 Dual-port       : OFF          Device         : mlx5_0
 Number of qps   : 1            Transport type : IB
 Connection type : UD           Using SRQ      : OFF
 PCIe relax order: ON
 ibv_wr* API     : ON
 TX depth        : 128
 CQ Moderation   : 100
 Mtu             : 2048[B]
 Link type       : Ethernet
 GID index       : 3
 Max inline data : 0[B]
 rdma_cm QPs     : OFF
 Data ex. method : Ethernet
---------------------------------------------------------------------------------------
 local address: LID 0000 QPN 0x2e73 PSN 0x432003
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:52
 remote address: LID 0000 QPN 0x5eb0 PSN 0xa65d7d
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:53
---------------------------------------------------------------------------------------
 #bytes     #iterations    BW peak[MB/sec]    BW average[MB/sec]   MsgRate[Mpps]
Conflicting CPU frequency values detected: 800.635000 != 1980.377000. CPU Frequency is not max.
 2048       28759800         0.00               9362.02            4.793356
---------------------------------------------------------------------------------------
```

MTU 1024
```
cyx@s52:~/projects$ ib_send_bw -D 10 s53 -m 1024 -c UD
 WARNING: BW peak won't be measured in this run.
 Max msg size in UD is MTU 1024
 Changing to this MTU
---------------------------------------------------------------------------------------
                    Send BW Test
 Dual-port       : OFF          Device         : mlx5_0
 Number of qps   : 1            Transport type : IB
 Connection type : UD           Using SRQ      : OFF
 PCIe relax order: ON
 ibv_wr* API     : ON
 TX depth        : 128
 CQ Moderation   : 100
 Mtu             : 1024[B]
 Link type       : Ethernet
 GID index       : 3
 Max inline data : 0[B]
 rdma_cm QPs     : OFF
 Data ex. method : Ethernet
---------------------------------------------------------------------------------------
 local address: LID 0000 QPN 0x2e71 PSN 0x47d0d0
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:52
 remote address: LID 0000 QPN 0x5eae PSN 0x1044a0
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:53
---------------------------------------------------------------------------------------
 #bytes     #iterations    BW peak[MB/sec]    BW average[MB/sec]   MsgRate[Mpps]
Conflicting CPU frequency values detected: 800.400000 != 2398.226000. CPU Frequency is not max.
 1024       30565500         0.00               4974.89            5.094292
---------------------------------------------------------------------------------------
```

### 单进程各深度（去掉 memcpy 模拟 ib_send_bw）


10 个服务线程，1 个 client 进程，MTU 4096，depth 65

```
Passed 6.65 seconds
  #0 11.10 GiB/s with 0.00% lost. Sended 19342986, received 19342661, ecn marked 0 
In total, 11.10 GiB/s, 0.00% lost, send 2.91 Mops, recv 2.91 Mops, ecn marked rate 0.00%
```

**10 个服务线程，1 个 client 进程，MTU 4096，depth 33**

```
Passed 5.77 seconds
  #0 10.63 GiB/s with 0.00% lost. Sended 16071462, received 16071343, ecn marked 0 
In total, 10.63 GiB/s, 0.00% lost, send 2.79 Mops, recv 2.79 Mops, ecn marked rate 0.00%
```

10 个服务线程，1 个 client 进程，MTU 4096，depth 17

```
Passed 12.37 seconds
  #0 8.65 GiB/s with 0.00% lost. Sended 28033091, received 28033034, 
In total, 8.65 GiB/s, 0.00% lost, send 2.27 Mops, recv 2.27 Mops
```

10 个服务线程，1 个 client 进程，MTU 2048，depth 33

```
Passed 11.51 seconds
  #0 7.78 GiB/s with 0.00% lost. Sended 46991360, received 46991360, 
In total, 7.78 GiB/s, 0.00% lost, send 4.08 Mops, recv 4.08 Mops
```

10 个服务线程，1 个 client 进程，MTU 1024，depth 65

```
Passed 11.43 seconds
  #0 4.27 GiB/s with 0.00% lost. Sended 51205791, received 51205791, 
In total, 4.27 GiB/s, 0.00% lost, send 4.48 Mops, recv 4.48 Mops
```

### ib_send_bw RC

```
---------------------------------------------------------------------------------------
                    RDMA_Write BW Test
 Dual-port       : OFF          Device         : mlx5_0
 Number of qps   : 1            Transport type : IB
 Connection type : RC           Using SRQ      : OFF
 PCIe relax order: ON
 ibv_wr* API     : ON
 TX depth        : 128
 CQ Moderation   : 1
 Mtu             : 4096[B]
 Link type       : Ethernet
 GID index       : 3
 Max inline data : 0[B]
 rdma_cm QPs     : OFF
 Data ex. method : Ethernet
---------------------------------------------------------------------------------------
 local address: LID 0000 QPN 0x27c6 PSN 0x7be456 RKey 0x203d15 VAddr 0x007f39f67cf000
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:52
 remote address: LID 0000 QPN 0x61eb PSN 0x24a752 RKey 0x1fff00 VAddr 0x007fa0968fd000
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:53
---------------------------------------------------------------------------------------
 #bytes     #iterations    BW peak[MB/sec]    BW average[MB/sec]   MsgRate[Mpps]
Conflicting CPU frequency values detected: 800.140000 != 1400.921000. CPU Frequency is not max.
 65536      5000             11555.13            11554.28                  0.184868
---------------------------------------------------------------------------------------
```

## 检查 numa 与运行指南

通过下列命令查看网卡的 numa node：

```bash
cat /sys/class/infiniband/mlx5_0/device/numa_node
```

另一种方式是：

```bash
lspci | grep Mellanox
lspci -s xxxxx -v
```

通过 numactl -H 可以看到每个 numa 持有哪些 core。

使用 Intel MLC 看 numa 间内存带宽：

```bash
cd /home/cyx/chores/Linux
./mlc

Measuring idle latencies (in ns)...
                Numa node
Numa node            0       1
       0          89.3   138.9
       1         138.9    89.4

Using Read-only traffic type
                Numa node
Numa node            0       1
       0        69287.5 34400.6
       1        34407.0 68981.6
```

运行时使用 scripts 下的脚本即可

看上去内存带宽挺大的，但就是 numa 错了性能就炸了。。。

## 交换机和网卡 MTU 设置

https://ordinary-secure-3a8.notion.site/RDMA-MTU-2c48226057fa4672a7854b77a76a0839?pvs=4

## perf

第二步需要去火焰图目录进行

```
sudo perf record --call-graph fp -p `pgrep mui_server` -a -g -e cpu-clock
sudo chown cyx perf.data && perf script -i perf.data &>perf.unfolded && ./stackcollapse-perf.pl perf.unfolded > perf.folded && ./flamegraph.pl perf.folded > perf.svg
```
