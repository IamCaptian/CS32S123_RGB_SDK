#include "include.h"

#if (CTP_SELECT == CTP_JD9261T)

// JD9261T 触控芯片驱动 (对齐原生驱动 tp_jd9261t.c)
// I2C地址: 0x68 (7bit), 写0xD0 / 读0xD1
// 读报点: 先写 0xF3 + 32bit寄存器地址 + 0x03 命令, 再连读40字节
// 芯片ID: 直接发 32bit地址 0x40008076(不加0xF3/0x03) 读2字节, 第2字节为0x90
// 报文: [0]=状态 [1]=手势 [2]=手指状态, 从偏移3开始每点5字节: XH XL YH YL

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

#define JD9261T_CMD             0xF3
#define JD9261T_CMD_TAIL        0x03                //参考驱动中固定的读命令尾字节
#define JD9261T_REG_CHIP_ID     0x40008076u
#define JD9261T_CHIP_ID_CODE    0x90
#define JD9261T_REG_COORD       0x20011120u         //坐标读取地址

// 运行参数寄存器(与原生驱动 jd9261t_init 一致)
#define JD9261T_REG_PUSHTIMER       0xEE            //报点周期
#define JD9261T_REG_IRQCRTL         0xFA            //中断控制
#define JD9261T_REG_DISAUTOSLEEP    0xFE            //禁止自动休眠
#define JD9261T_REFRESH_RATE        30              //报点率(Hz), 与原生默认值一致

#define JD9261T_ID_RETRY_CNT        10              //复位后读ID重试次数
#define JD9261T_ID_RETRY_MS         20              //读ID重试间隔(ms)

#define JD9261T_READ_LEN        40                  //坐标报文读取长度(原生驱动同为40字节)
#define JD9261T_POINT_START     3                   //点数据起始偏移
#define JD9261T_MAX_TOUCH_NUM   2                   //支持2点触摸

#define JD9261T_READ_WAIT       0x5A                //多步流程的中间态返回值(仅kick,不解析)

// 坐标方向调整(按实际屏幕方向修改)
#define JD9261T_XY_SWAP         0
#define JD9261T_X_REVERSE       0
#define JD9261T_Y_REVERSE       0

#define JD9261T_I2C_WADDR       ((u8)((TP_IIC_ADDR) << 1))
#define JD9261T_I2C_RADDR       ((u8)(((TP_IIC_ADDR) << 1) | 1))

#define JD9261T_STAGE_IDLE      0
#define JD9261T_STAGE_CMD       1                   //命令已KICK
#define JD9261T_STAGE_DATA      2                   //数据已KICK

static u8 jd9261t_buf[JD9261T_READ_LEN];
static u8 jd9261t_cmd[6];
static u8 jd9261t_stage;
static u32 jd9261t_stage_tick;

extern i2c_t *CTP_IIC;
extern bool ctp_kick_status;

// 等待本次IIC传输完成并清中断标志
static bool jd9261t_i2c_wait_done(void)
{
    u32 ticks = tick_get();

    while (!(CTP_IIC->sfr->IICxCON0 & BIT(31))) {
        if (tick_check_expire(ticks, 20)) {
            return false;
        }
    }
    CTP_IIC->sfr->IICxCON0 |= BIT(29);                //Clear Pending

    return true;
}

// 1字节寄存器地址写1字节数据(阻塞), 对应原生驱动的 SENSOR_I2C_WRITE
static bool jd9261t_reg8_write(u8 reg, u8 val)
{
    static u8 wdat;

    wdat = val;
    CTP_IIC->sfr->IICxCMDA = ((u32)reg << 8) | JD9261T_I2C_WADDR;
    CTP_IIC->sfr->IICxCON1 = BIT(11) | BIT(10) | BIT(5) | BIT(4) | BIT(3); //STOP|WDATA|REG_ADDR_0|DEV_ADDR0|START
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(&wdat);
    CTP_IIC->sfr->IICxDMACNT = BIT(1) | BIT(0);        //写1字节
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK

    return jd9261t_i2c_wait_done();
}

// 组包: 0xF3 + 32bit寄存器地址 + 读命令
static void jd9261t_build_cmd(u32 reg)
{
    jd9261t_cmd[0] = JD9261T_CMD;
    jd9261t_cmd[1] = (reg >> 24) & 0xFF;
    jd9261t_cmd[2] = (reg >> 16) & 0xFF;
    jd9261t_cmd[3] = (reg >> 8) & 0xFF;
    jd9261t_cmd[4] = reg & 0xFF;
    jd9261t_cmd[5] = JD9261T_CMD_TAIL;
}

// 只KICK写命令, 不等待完成
AT(.com_text.ctp)
static void jd9261t_cmd_write_kick(void)
{
    CTP_IIC->sfr->IICxCMDA = ((u32)jd9261t_cmd[0] << 8) | JD9261T_I2C_WADDR;
    CTP_IIC->sfr->IICxCON1 = BIT(10) | BIT(5) | BIT(4) | BIT(3);  //WDATA|REG_ADDR_0|DEV_ADDR0|START
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(&jd9261t_cmd[1]);
    CTP_IIC->sfr->IICxDMACNT = ((u32)(sizeof(jd9261t_cmd) - 2) << 16) | BIT(1) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK
}

// KICK读数据, 完成后在I2C中断中解析
AT(.com_text.ctp)
static void jd9261t_data_read_kick(u8 *buf, u16 len)
{
    memset(buf, 0, len);                                                      //与原生驱动一致: 读前清零, 未读到的字节保持0
    CTP_IIC->sfr->IICxCON1 = BIT(12) | BIT(11) | BIT(9) | BIT(8) | BIT(7);   //NACK|STOP|RDATA|DEV_ADDR1|START
    CTP_IIC->sfr->IICxCMDA = (u32)JD9261T_I2C_RADDR << 24;
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(buf);
    CTP_IIC->sfr->IICxDMACNT = ((len - 1) << 16) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK
}

// 阻塞读(初始化用): 发命令后读数据
static bool jd9261t_blocking_read(u32 reg, u8 *buf, u16 len)
{
    jd9261t_build_cmd(reg);
    jd9261t_cmd_write_kick();
    if (!jd9261t_i2c_wait_done()) {
        return false;
    }
    jd9261t_data_read_kick(buf, len);
    return jd9261t_i2c_wait_done();
}

// 按指定命令读数据: cmd[0]作为寄存器地址字节, cmd[1..]由DMA发出, dma_field = DMA字节数-1
static bool jd9261t_cmd_read(u8 *cmd, u8 dma_field, u8 *buf, u16 len)
{
    CTP_IIC->sfr->IICxCMDA = ((u32)cmd[0] << 8) | JD9261T_I2C_WADDR;
    CTP_IIC->sfr->IICxCON1 = BIT(10) | BIT(5) | BIT(4) | BIT(3);  //WDATA|REG_ADDR_0|DEV_ADDR0|START
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(&cmd[1]);
    CTP_IIC->sfr->IICxDMACNT = ((u32)dma_field << 16) | BIT(1) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK
    if (!jd9261t_i2c_wait_done()) {
        return false;
    }
    jd9261t_data_read_kick(buf, len);
    return jd9261t_i2c_wait_done();
}

// 32bit寄存器地址读(对应平台 read_uint32): 直接发4字节地址, 不加0xF3/0x03
// 实测只有这种格式能读到芯片ID(0x62 0x90)
static bool jd9261t_read_reg32(u32 reg, u8 *buf, u16 len)
{
    jd9261t_cmd[0] = (reg >> 24) & 0xFF;
    jd9261t_cmd[1] = (reg >> 16) & 0xFF;
    jd9261t_cmd[2] = (reg >> 8) & 0xFF;
    jd9261t_cmd[3] = reg & 0xFF;
    return jd9261t_cmd_read(jd9261t_cmd, 2, buf, len);
}

bool ctp_jd9261t_init(void)
{
    u8 id[2] = {0};
    u8 dump[8] = {0};
    bool found = false;
    int i;

    //复位后芯片需要稳定时间, 读ID重试(每次之间留出间隔, 避免都在未就绪窗口里连打)
    for (i = 0; i < JD9261T_ID_RETRY_CNT; i++) {
        memset(id, 0, sizeof(id));
        if (jd9261t_read_reg32(JD9261T_REG_CHIP_ID, id, sizeof(id)) && (id[1] == JD9261T_CHIP_ID_CODE)) {
            found = true;
            break;
        }
        if (i < (JD9261T_ID_RETRY_CNT - 1)) {
            delay_ms(JD9261T_ID_RETRY_MS);
        }
    }
    TRACE("CTP JD9261T id: %02X %02X%s\n", id[0], id[1], found ? "" : " (mismatch)");

    //诊断: 打印一帧坐标寄存器原始数据
    memset(dump, 0, sizeof(dump));
    if (jd9261t_blocking_read(JD9261T_REG_COORD, dump, sizeof(dump))) {
        TRACE("CTP JD9261T coord dump: %02X %02X %02X %02X %02X %02X %02X %02X\n",
              dump[0], dump[1], dump[2], dump[3], dump[4], dump[5], dump[6], dump[7]);
    }

    //按原生驱动 jd9261t_init 配置: 报点周期 / 中断控制 / 关闭自动休眠
    bool cfg_ok;
    cfg_ok  = jd9261t_reg8_write(JD9261T_REG_PUSHTIMER, JD9261T_REFRESH_RATE / 10);  //0xEE = 3 (30Hz)
    cfg_ok &= jd9261t_reg8_write(JD9261T_REG_IRQCRTL, 0x60);                        //0xFA
    cfg_ok &= jd9261t_reg8_write(JD9261T_REG_DISAUTOSLEEP, 0x01);                   //0xFE
    TRACE("CTP JD9261T cfg: push=%d irq=0x60 nosleep=1 %s\n",
          JD9261T_REFRESH_RATE / 10, cfg_ok ? "" : "(write fail)");

    TRACE("CTP JD9261T init ok\n");
    return true;
}

//中断里KICK命令写, 后续在I2C中断里继续
AT(.com_text.ctp)
void ctp_jd9261t_readkick(void)
{
    //上一帧未处理完(总线异常)时复位, 避免卡在中间态
    if (jd9261t_stage != JD9261T_STAGE_IDLE) {
        if (!tick_check_expire(jd9261t_stage_tick, 50)) {
            return;
        }
        jd9261t_stage = JD9261T_STAGE_IDLE;
    }

    jd9261t_build_cmd(JD9261T_REG_COORD);
    jd9261t_cmd_write_kick();
    jd9261t_stage_tick = tick_get();
    jd9261t_stage = JD9261T_STAGE_CMD;
    ctp_kick_status = true;
}

AT(.com_text.ctp)
u8 ctp_jd9261t_get_point(s32 *x, s32 *y)
{
    u8 finger_num;
    u8 *p;
    s32 px, py;

    if (jd9261t_stage == JD9261T_STAGE_CMD) {
        //命令写完成, 接着KICK读
        jd9261t_data_read_kick(jd9261t_buf, JD9261T_READ_LEN);
        jd9261t_stage = JD9261T_STAGE_DATA;
        return JD9261T_READ_WAIT;
    }
    if (jd9261t_stage != JD9261T_STAGE_DATA) {
        return false;
    }
    jd9261t_stage = JD9261T_STAGE_IDLE;

    finger_num = jd9261t_buf[0] & 0x0F;
    if (finger_num > JD9261T_MAX_TOUCH_NUM) {
        finger_num = JD9261T_MAX_TOUCH_NUM;
    }
    if (finger_num == 0) {
        return false;
    }

    p = &jd9261t_buf[JD9261T_POINT_START];
    px = ((s32)p[0] << 8) | p[1];
    py = ((s32)p[2] << 8) | p[3];

    //无效坐标
    if (((px == 0) && (py == 0)) || ((px == 0xFFFF) && (py == 0xFFFF))) {
        return false;
    }

#if JD9261T_XY_SWAP
    {
        s32 tmp = px;
        px = py;
        py = tmp;
    }
#endif
#if JD9261T_X_REVERSE
    px = (GUI_SCREEN_WIDTH - 1) - px;
#endif
#if JD9261T_Y_REVERSE
    py = (GUI_SCREEN_HEIGHT - 1) - py;
#endif

    *x = px;
    *y = py;

    return true;
}

#endif //CTP_JD9261T
