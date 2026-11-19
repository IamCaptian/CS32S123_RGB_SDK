# PSRAM 调试记录

日期：2026-11-19
项目：`projects/watch320`（CS32S123BAS，GUI_TFT_360_JD9855 360×360 QSPI 屏）

## 1. 现象

打开 PSRAM 后依次出现过两个现象：

1. 屏幕不显示（系统还在跑，串口有日志）
2. 完全不启动：复位后串口只有一个 `\0`，连 app 启动阶段的 `==>CP Trim` / `Hello Platform` / `startup` 都没有

## 2. 关键配置与触发链

工程里有多个"PSRAM 开关"，改一个会连带打开其他功能：

- `projects/watch320/config.h`
  - `CHIP_PACKAGE_SUPPORT_PSRAM = 1` 会同时让
    - `GUI_USE_SCREENSHOOT = 1`（截图转场）
    - `GUI_USE_BLUR = 1`（高斯模糊）
    - `AVI_DIALPLATE_EN = 1`（视频表盘）
    - `DVP_AUTO_RATIO` 从 2 变 1
  - `NOC_PSRAM_EN` 是真正打开 PSRAM 控制器的宏
- `platform/header/config_extra.h`
  - `GUI_USE_BLUR = 1` 会强制 `NOC_PSRAM_EN = 1`
  - `NOC_PSRAM_EN = 1` 时 `PSRAM_SIZE = NOC_PSRAM_SIZE = FSIZE_4M`
  - `USE_LCD_SCREEN` 时也会强制 `NOC_PSRAM_EN = 1`
- `platform/bsp/bsp_sys.c:822`
  - 启动时 `noc_init((NOC_PSRAM_EN << 1) | NOC_FLASH_EN)`，PSRAM 的 DDR 初始化/校准在这里
  - 休眠唤醒后 `func_lowpwr.c:547` 还会再初始化一次

注意：`config_extra.h` 里"芯片不支持 PSRAM 就自动关闭"的保护依赖 `CHIP_PACKAGE_SUPPORT_SELECT` / `CHIP_FUNC_NOT_PSRAM`，这两个宏在本工程**未定义**，保护实际不生效。`CHIP_PACKAGE_SUPPORT_PSRAM` 是纯手工开关，不会自动探测硬件。

## 3. 排查过程与结论

### 3.1 先确认 PSRAM 硬件是否正常

最小配置（只开 PSRAM 控制器，不开截图/模糊）：

```c
#define CHIP_PACKAGE_SUPPORT_PSRAM   0
#define NOC_PSRAM_EN                 1
#define NOC_PSRAM_SIZE               FSIZE_4M
#define GUI_USE_SCREENSHOOT          0
#define GUI_USE_BLUR                 0
#define GUI_SPU_PSRAM                0
```

串口日志：

```text
[DBG] io init done, psram_en=1 psram_size=0x400000
[DBG] before rtc_init
RTCCON0: 20904b14, RTCCNT: 6d4578e
[DBG] after rtc_init
[DBG] before noc_init
aps6408l_init
done 1, data 1, dqs 1
[DBG] after noc_init
[DBG] psram test start
[DBG] psram rw: 0x5a5aa5a5 0x12345678
[DBG] psram test end
```

结论：

- 初始化的颗粒是 `aps6408l`（8MB）
- DDR 校准通过（`done 1, data 1, dqs 1`）
- 对 `0x20000000 / 0x20000004` 读写正确

**PSRAM 控制器与颗粒本身没有问题，问题在打开 PSRAM 后连带打开的 GUI 功能和内存布局。**

### 3.2 根因：`disp_buf` 把 SRAM 的 `.bss` 撑爆

打开 `CHIP_PACKAGE_SUPPORT_PSRAM=1` 后，`platform/bsp/bsp_video/bsp_video_play.c` 里：

```c
u8 disp_buf[VIDEO_PLAY_DISP_WID_MAX*VIDEO_PLAY_DISP_HEI_MAX*2];
```

- 该宏的取值在 `bsp_video_play.c:6-17`：
  - PSRAM 关闭：固定 `240 × 320` → `disp_buf` 约 150KB
  - PSRAM 打开且屏幕比 sensor 大：取 `GUI_SCREEN_WIDTH × GUI_SCREEN_HEIGHT` → `360 × 360` → **253KB（0x3F480）**
- 这个 buffer **无条件放在 SRAM 的 `.bss`**，但 PSRAM 打开后视频播放实际用的是 PSRAM 里的 `video_play_disp_buff`，这个 253KB 只有 GIF 界面在用

结果（打开 `CHIP_PACKAGE_SUPPORT_PSRAM=1` 的 build）：

```text
__bss_start         = 0x29C00
__bss_end           = 0x75C1C     // 超过 camera heap 起始地址
__camera_heap_start = 0x61000
__dynamic_pool_size = 0xFFFEB200  // 0x61000 - 0x75E00，无符号下溢成负数
```

`platform/bsp/bsp_mem/heap.c` 中的后果：

```c
u32 size = min((u32)&__dynamic_pool_size, MAX_HEAP_SIZE);  // 下溢值被 min 成 0x40008
memset((void *)&__dynamic_pool_start, 0, size);            // 0x75E00 起抹 256KB
```

这会覆盖：

- `.disp`（0x80000 ~ 0x8FAEC，GUI 显示缓冲）
- LP RAM（0x90000 起，heap_ram / sleep_ram 等）
- TLSF 堆本身也和 `.disp` 重叠

属于必然出问题的内存破坏。打开 PSRAM 后"完全不启动"就是从这一版开始的。

## 4. 修复

`platform/bsp/bsp_video/bsp_video_play.c`：

```c
#if PSRAM_SIZE
//有PSRAM时放到PSRAM, 否则360*360*2(253KB)会把SRAM的bss撑爆(超过camera heap起始地址)
u8 disp_buf[VIDEO_PLAY_DISP_WID_MAX*VIDEO_PLAY_DISP_HEI_MAX*2] AT(.psram_buf.disp);
#else
u8 disp_buf[VIDEO_PLAY_DISP_WID_MAX*VIDEO_PLAY_DISP_HEI_MAX*2];
#endif
```

`AT(.psram_buf.disp)` 会被 `projects/watch320/ram.ld` 里的 `*(.psram_buf*)` 收进 `.psram_other`（NOLOAD），不再占用 SRAM。

修复后的实测日志：

```text
sram total = 0x5FC00, used = 0x1639C, remain = 0x49864
bss : 0x29C00 ~ 0x3679C  size: 0xCB9C
[DBG] io init done, psram_en=1 psram_size=0x400000
...
[DBG] before noc_init
aps6408l_init
done 1, data 1, dqs 1
[DBG] after noc_init
[DBG] psram rw: 0x5a5aa5a5 0x12345678
...
[DBG] before gui_init
gpu_init
gui_sw_init
CTP CST9217 init ok
TFT ID: ff985501
tft_360_jd9855_init
[DBG] after gui_init
dynamic_pool:0x36800, 0x2a800, use max size:0x2a800
malloc heap size:174080
[DBG] after customer_heap_init
func_run
task_stack_push:[3]<--[3]
func_clock
```

`__bss_end` 回落到 `0x3679C`，`__dynamic_pool_size = 0x2A800`（174,080 字节），系统可以正常启动到 `func_clock`。

## 5. 尚未处理的问题

### 5.1 `GUI_SPU_PSRAM`（整帧 PSRAM 推屏）开了会白屏

`platform/gui/gui.c` 中：

```c
u8 *gui_flush_lcd_buff_get(void)
{
    if((gui_share_cb.buff0_idx == 0x00) && (gui_share_cb.buff1_idx == 0x00)){
        return NULL;
    }
    ...
}
```

问题：`buff0_idx` / `buff1_idx` 全工程只有初始化和清零，**没有任何地方置 1**，所以 `gui_flush_lcd_buff_get()` 永远返回 NULL。

`libplatform.a` 的 `gui.o` 中 `os_gui_flush_lcd` 的流程（反汇编确认）：

```text
if (gui_is_user_psram()) {
    buf = gui_flush_lcd_buff_get();
    if (buf) {
        tft_spi_send_all(buf, gui_lcd_width(), gui_lcd_height());
        gui_flush_lcd_buff_put(buf);
    }
}
```

拿不到缓冲就不会推屏，表现就是白屏但系统还活着。

当前结论：360×360 QSPI 屏保持 `GUI_SPU_PSRAM = 0`。如果要开整帧 PSRAM 推屏，需要先把 `gui_flush_lcd_buff_get/put` 的"占用/释放"协议补全（`idx == 0` 表示空闲）。

### 5.2 PSRAM 容量配置偏小

日志显示颗粒是 `aps6408l`（8MB），而 `NOC_PSRAM_SIZE` 是 `FSIZE_4M`。

- 目前 `GUI_SPU_PSRAM = 0` 时 PSRAM 段约 3.5MB，4M 够用
- 如果以后打开 `GUI_SPU_PSRAM`，PSRAM 段会到约 4.08MB，4M 只剩约 100KB 余量
- 建议那时把 `NOC_PSRAM_SIZE` 改为 `FSIZE_8M`

## 6. 调试手段（当前代码中仍保留）

为了方便定位，加了 `[DBG]` 前缀的打印，确认稳定后可以全局搜索 `[DBG]` 删除：

- `projects/watch320/main.c`
  - `main()` 第一行 `uart0_mapping_sel()` + `[DBG] main enter`
  - 原因：`PRINTF_PB3` 的引脚映射是在 `bsp_io_init()` 里才建立的，`main()` 开头默认打在 PA7 上；提前映射可以保证最早的打印可见
- `platform/bsp/bsp_sys.c`
  - 各初始化阶段打点：`io init` / `var init` / `rtc pwd calib` / `pmu` / `clock` / `param` / `xosc` / `rtc_init` / `noc_init` / `gui_init` / `customer_heap_init`
  - `noc_init` 后做 PSRAM 读写自检（写 `0x20000000` / `0x20000004`，读回校验后清零）

调试打印的关键判据：

| 最后停在哪 | 结论 |
|---|---|
| 没有 `[DBG] main enter` | 没进 app/main，查镜像、启动 header、复位/供电 |
| `before noc_init` 后没有 `after noc_init` | PSRAM DDR 校准卡住，查封装/容量/供电 |
| 有 `psram test start` 没有 `psram rw` | PSRAM 访问异常 |
| `rw` 值不等于 `0x5a5aa5a5 / 0x12345678` | PSRAM 读写不可靠 |
| `before gui_init` 后卡住 | GUI 初始化/PSRAM 帧缓冲路径 |

## 7. 当前有效配置

`projects/watch320/config.h`（调试状态）：

```c
#define CHIP_PACKAGE_SUPPORT_PSRAM   1
#define NOC_PSRAM_EN                 1
#define NOC_PSRAM_SIZE               FSIZE_4M
#define GUI_USE_SCREENSHOOT          1   // 由 CHIP_PACKAGE_SUPPORT_PSRAM 乘出
#define GUI_USE_BLUR                 1   // 由 CHIP_PACKAGE_SUPPORT_PSRAM 乘出
#define GUI_SPU_PSRAM                0   // 整帧 PSRAM 推屏暂不开
```

## 8. 涉及文件

- `projects/watch320/config.h`：PSRAM/GUI 开关，调试期间被改为打开状态
- `platform/header/config_extra.h`：PSRAM 开关之间的强制联动关系
- `platform/bsp/bsp_sys.c`：`noc_init` 调用点、调试打印
- `projects/watch320/main.c`：UART0 提前映射、调试打印
- `platform/bsp/bsp_video/bsp_video_play.c`：`disp_buf` 修复
- `platform/gui/gui.c`：`GUI_SPU_PSRAM` 的帧缓冲 get/put（待修复）
- `projects/watch320/ram.ld`：`.psram_*` 段、`__camera_heap_start`、`__dynamic_pool_*` 布局
- `platform/bsp/bsp_mem/heap.c`：`customer_heap_init()` 的堆初始化

## 9. 打开 PSRAM 后转场方式变化 + 切换撕裂

### 9.1 现象

打开 PSRAM 后，主界面左右切换的动画从原来的"左右缩放"变成了 3D 旋转，并且动画过程中屏幕撕裂。

### 9.2 原因

`CHIP_PACKAGE_SUPPORT_PSRAM=1` 会打开 `GUI_USE_SCREENSHOOT`，而转场表按这个宏分叉：

`projects/watch320/functions/common/func_switching.c` 的 `tbl_switching_mode[]`：

- 不截图时：索引 0 = `FUNC_SWITCH_LR_ZOOM_RIGHT/LEFT`（普通左右缩放）
- 截图时：索引 0 = `FUNC_SWITCH3D_LR_RIGHT/LEFT`（3D 旋转），并额外增加 FLIP/PLATE/CUBE/ROTA/FOLD/DRIFT 等条目

`sys_cb.nav_index` 默认是 `NAV_ZOOM(0)`，于是同一个索引指向了新的 3D 动画；设置里的"导航方式"列表也同步多出了 3D 项。

撕裂的原因：

- 3D 转场先用 `func_switch_screenshot()` 把前后两屏各截一帧到 PSRAM（`cur_scbuf` / `next_scbuf`）
- 每一帧动画都是整屏 3D 变换后的**全屏重绘**，源图在 PSRAM
- JD9855 QSPI 驱动配置为 20MHz（`tft_360_jd9855.c` 的 `speed1_mhz/speed2_mhz`），360×360×2 = 259,200 字节，整屏推送约 26ms，大于 TE 周期 16.7ms
- 一帧没推完下一帧就开始了，所以撕裂；`DEFAULT_TE_MODE=1` 还会在 TE 后延迟约 5.5ms 才开始刷，进一步压缩可用时间

### 9.3 处理

恢复原来的转场方式（保留 PSRAM、截图、模糊功能，但不走 3D）。3D 转场代码不删除，用宏屏蔽：

1. `projects/watch320/config.h` 新增开关（默认 0 = 屏蔽 3D 转场，代码保留）：

```c
#define GUI_USE_SWITCH3D                0   //是否使用3D转场动画(需要截图), 0:屏蔽3D转场(代码保留)
```

2. `func_switching.c`：`tbl_switching_mode[]` 的 3D 表项改为由 `GUI_USE_SWITCH3D && GUI_USE_SCREENSHOOT` 控制；关闭时 ZOOM 用原来的左右缩放，DRIFT 用直接切换：

```c
#if GUI_USE_SWITCH3D && GUI_USE_SCREENSHOOT
    {FUNC_SWITCH3D_LR_RIGHT, FUNC_SWITCH3D_LR_LEFT},   //ZOOM
#else
    {FUNC_SWITCH_LR_ZOOM_RIGHT, FUNC_SWITCH_LR_ZOOM_LEFT}, //ZOOM
#endif
```

3. `func_get_switching_mode_byidx()` 增加越界保护，索引超过表长时回落到 `NAV_ZOOM`。
4. `func_setting_sub_menu_navigation.c`：导航方式列表的 3D 选项也用同一个宏屏蔽，保持与转场表一一对应。关闭时列表为 ZOOM / FADE / SHIFT / DRIFT / NONE，打开时恢复 FLIP / PLATE / CUBE / ICUBE / ROTA / FOLD。

需要重新启用 3D 时，只把 `GUI_USE_SWITCH3D` 改回 1 即可（前提是 `GUI_USE_SCREENSHOOT` 也为 1）。

如果以后仍想用 3D 转场且不撕裂，可选：

- 把 JD9855 QSPI 时钟提高到驱动注释里的 `41/31MHz`（整屏约 12.6ms，可放进一个 TE 周期）
- 去掉 3D 的倒影/镜像渲染，或降低动画帧率（`SWITCHING_TICK_EXPIRE` 调大）
- 把 `DEFAULT_TE_MODE` 从 1 改成 0，让刷新在 TE 到来时立即开始
