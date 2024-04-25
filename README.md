# UD 多打一测试

## 结论

1. 一定要把线程绑到对应的 numa node 上，不然性能挂逼了！
2. transmit depth=17 的时候性能比较一般，=33的时候性能就很好了，基本能持平 ib_send_bw；
3. 其他内容调整好了以后，加上 memcpy 性能损失不是特别大，但也是有的。memcpy 对没对齐问题也不大（当然，最好还是能和 4、8、16 这样的数字对齐）。

## 测试结果

1 个进程都是 s52->s53，9 个进程都是 s51, s52, s53 各 3 个

### 单进程各 MTU

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

10 个服务线程，9 个 client 进程，MTU 4096，depth 17

10 个服务线程，9 个 client 进程，MTU 2048，depth 33

10 个服务线程，9 个 client 进程，MTU 1024，depth 65

```

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
cyx@s52:~/projects$ ib_send_bw -D 10 s53 -m 1024
 WARNING: BW peak won't be measured in this run.
---------------------------------------------------------------------------------------
                    Send BW Test
 Dual-port       : OFF          Device         : mlx5_0
 Number of qps   : 1            Transport type : IB
 Connection type : RC           Using SRQ      : OFF
 PCIe relax order: ON
 ibv_wr* API     : ON
 TX depth        : 128
 CQ Moderation   : 1
 Mtu             : 1024[B]
 Link type       : Ethernet
 GID index       : 3
 Max inline data : 0[B]
 rdma_cm QPs     : OFF
 Data ex. method : Ethernet
---------------------------------------------------------------------------------------
 local address: LID 0000 QPN 0x1ac3 PSN 0xc20386
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:52
 remote address: LID 0000 QPN 0x3b02 PSN 0xbb0c4e
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:192:168:200:53
---------------------------------------------------------------------------------------
 #bytes     #iterations    BW peak[MB/sec]    BW average[MB/sec]   MsgRate[Mpps]
Conflicting CPU frequency values detected: 800.493000 != 3067.271000. CPU Frequency is not max.
 65536      1051999          0.00               10958.41                   0.175335
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
