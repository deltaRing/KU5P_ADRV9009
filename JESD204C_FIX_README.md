# ADRV9009 + AMD JESD204C v4.2（8B10B）软件修正

## 已修改的文件

- `devices/adi_hal/jesd_core.h`
- `devices/adi_hal/jesd_core.c`
- `app/headless.c`
- `app/rf_config.c`

## 根因

原 `jesd_core.h/.c` 使用的是旧版 ADI/Xilinx JESD204B 驱动寄存器表，和工程中的 AMD JESD204C v4.2 IP 不匹配。

最致命的一项是：旧驱动将 `0x20` 当成 F（Octets per Frame）寄存器写入；但在 JESD204C v4.x 中，`0x020` 是 `RESET`。TX 的 F=2 会写 1，RX/ORX 的 F=4 会写 3，从而让 RESET bit0 一直被置位。这正好解释了 `reset done` 一直不完成以及 ADRV9009 deframer 的 `0x12/0x13/0x1A`。

新的头文件中已经改为：

```c
#define JESD204_REG_TRX_RESET 0x020
```

并且整张 JESD204C v4.x 寄存器表已一并修正，不能只改单个 RESET 地址。

## 本工程使用的配置值

| Core | L | M | F | K | N/Np | Scramble | Subclass |
|---|---:|---:|---:|---:|---:|---:|---:|
| TX → ADRV9009 Deframer A | 4 | 4 | 2 | 32 | 16/16 | 1 | 1 |
| RX ← ADRV9009 Framer A | 2 | 4 | 4 | 32 | 16/16 | 1 | 1 |
| ORX ← ADRV9009 Framer B | 2 | 4 | 4 | 32 | 16/16 | 1 | 1 |

关键寄存器的预期回读：

| Core | 地址 | 预期值 | 含义 |
|---|---:|---:|---|
| TX | `0x034` | `0x00000001` | Subclass 1 |
| TX | `0x03C` | `0x03031F01` | ILA=4、scramble、K=32、F=2 |
| TX | `0x040` | `0x0000000F` | 4 lanes |
| TX | `0x070` | `0x00000000` | BID=0、DID=0 |
| TX | `0x074` | `0x000F0F03` | M=4、N=16、Np=16、CS=0 |
| TX | `0x078` | `0x00000000` | S=1、HD=0、CF=0 |
| RX/ORX | `0x034` | `0x00000001` | Subclass 1 |
| RX/ORX | `0x03C` | `0x00031F03` | ILA、scramble、K=32、F=4 |
| RX/ORX | `0x040` | `0x00000003` | 2 lanes |

外部复位释放、GT reset 完成后，`0x020` 的 bit0 应为 0；若仍为 1，新代码会打印完整 RESET 回读以及 `gt_busy`、`external reset`、`register reset` 三项。

## ADRV9009 deframer 状态说明

- `0x12`：收到 SYSREF，但 frame sync 丢失。
- `0x13`：在 `0x12` 基础上还有 sync error。
- `0x1A`：收到 SYSREF/LMFC 状态变化，但还没有进入稳定 user-data 状态。

这些值与 FPGA TX 核被旧软件误写入 reset 的现象一致。

## 仍需在 Vivado Block Design 中核对

软件修正后若 `RESET=0x10/0x11/0x90/0x91` 仍持续存在，需要继续检查 FPGA 外部复位网络：

1. `tx_core_reset`、`rx_core_reset`、`orx_core_reset` 在 MicroBlaze 写复位寄存器为 0 后必须为低。
2. JESD204 PHY 的 `tx_reset_done`/`rx_reset_done` 必须正确回接三个 JESD204C 核。
3. `util_vector_logic_1` 当前把两路 active-high reset 用 AND 合并；通常这里应按实际极性改为 OR，否则只来一路复位时无法复位核心。
4. 顶层 `main.v` 中 `fclk_resetn` 被固定为 `1'b1`，这会绕过 AXI 自定义模块的复位。建议连接到真实的 active-low AXI/peripheral reset，再重新生成 bitstream。

先用这个软件包重新编译并下载 ELF。第一次启动重点记录 TX/RX/ORX 的 `RESET(0x020)`、`STAT_STATUS(0x060)`、PHY `0x080`，以及 ADRV9009 deframer/framer 状态。

## V2：SYSREF 时序与 `TAL_DEFRAMER_A = 0x06`

新日志已经证明旧版 RESET 寄存器问题解决：

- FPGA TX `STAT_STATUS=0x00001002`：8B10B SYNC 已建立，SYSREF 已捕获。
- FPGA RX/ORX `STAT_STATUS=0x00007002`：CGS、SYNC、RX data started、SYSREF 均已建立。
- 三条链路 `RX_ERR=0`。
- Talise deframer `0x06`：bit2=user data valid，bit1=SYSREF received；它不是完全掉链，只缺 bit7 valid checksum。

原程序在 `headless.c` 中过早拉高 `ad9528_sysref_req`，并在 Talise/FPGA 端点使能后没有产生新的受控 SYSREF 请求边沿。同时，`jesd204b_check()` 把 `0x06` 直接判为失败，随即重置已经进入 user-data 状态的链路，形成“偶尔成功 -> 马上重置 -> missing SYSREF”的循环。

V2 修改如下：

1. JESD 初始化前先将 `ad9528_sysref_req` 拉低。
2. Talise framers/deframer 已使能、FPGA 三个 JESD 核 reset_done 后，再拉高 SYSREF 请求 2 ms，然后拉低。
3. 在复位释放和 SYSREF 之后重新读取 PHY `0x080`，并直接解码 TX/RX reset 与 QPLL0 lock。
4. 对 deframer `0x06` 调用 `TALISE_getDfrmIlasMismatch()`，打印配置值、实际收到的 ILAS 和 mismatch 位图。
5. 若 ILAS 各字段全部一致，允许 `0x06` 继续进入 radio-on；若有任一字段或 lane checksum 不一致，则保留重试并输出精确差异。

`PLL STATUS=0x1D` 的旧打印发生在外部 JESD reset 仍被置位的窗口内，因此 bit4/bit3 为 1 并不等于 QPLL0 未锁。该寄存器的锁定位为低有效：`0x1D` 的 bit1=0 反而表示 QPLL0 已锁。应以新增的 `PHY STATUS after reset release/SYSREF` 为准；仅使用 QPLL0 时，复位完成后的常见回读是 `0x05`。

若仍出现 `ILAS_MISMATCH != 0`，位图含义为：bit0 DID、bit1 BID、bit2 LID0、bit3 L、bit4 SCR、bit5 F、bit6 K、bit7 M、bit8 N、bit9 CS、bit10 NP、bit11 S、bit12 CF、bit13 HD、bit14~17 lane0~3 checksum。请保留该行以及随后的 `Deframer CFG`/`Received ILAS` 两行。

## V3：修正 Talise 解串极性、lane 映射和 SYSREF 重试

V2 实机日志中的 PHY 最终状态始终为 `0x05`，说明 QPLL0 已锁且 TX/RX reset_done 均已完成。收到的 ILAS 却把 `L=3、M=3` 解码为 `L=28、M=28`。在 5 位编码中，`3=00011`、`28=11100`，两者是逐位反码；同时 `LID0=1` 而不是 0。这是接收串行极性补偿和 lane 映射错误的明确特征，不是 JESD204C 寄存器地址问题。

V3 修改如下：

1. `deframerA.deserializerLaneCrossbar` 从反向映射 `0x1B` 改为 identity 映射 `0xE4`，并使用自动 lane 映射。
2. `desInvertLanePolarity` 从 `0xF` 改为 `0x0`。Talise API 实际写入的是 `~desInvertLanePolarity & 0x0F`，因此 `0x0` 才会启用器件 laminate 所需的四 lane 默认补偿；原来的 `0xF` 会把内部实际反相值写成 0。
3. 三个 FPGA JESD 核 reset_done 后，在不重新初始化链路的前提下最多发出 10 次、每次 10 ms 的 SYSREF 请求窗口；每次打印 TX/RX/ORX 的 `0x060`，三个核心都捕获 bit1 后才继续。
4. 增加 TX ILA 寄存器回读。预期为：`03C=03031F01 070=00000000 074=000F0F03 078=00000000 404=00030000 484=00030001 504=00030002 584=00030003`。

修正后，Talise deframer 的正常状态应为 `0x86` 或 `0x8E`（bit3 只是 LMFC 状态），`ILAS_MISMATCH` 应为 0，且不应再看到 `3 -> 28`。
