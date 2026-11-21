#include "include.h"

#if (CTP_SELECT == CTP_JD9261T)

// JD9261T 触控芯片驱动
// I2C地址: 0x68 (7bit), 写0xD0 / 读0xD1
// 读报点: 先发 0xF3 + 32bit寄存器地址 + 读命令, 再连读上报数据
// 报文: [0]状态(bit[3:0]=触摸点数) [1]手势 [2]点数 [3~]点数据
//   每点5字节: XH XL YH YL P, 最多2点
// 芯片ID: 32bit寄存器 0x40008076, 第2字节为0x90表示JD9261T

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
#define JD9261T_REG_COORD       0x20011120u

#define JD9261T_READ_LEN        40                  //坐标报文读取长度
#define JD9261T_POINT_START     3                   //点数据起始偏移
#define JD9261T_MAX_TOUCH_NUM   2                   //支持2点触摸

// 控制寄存器(8bit寄存器地址)
#define JD9261T_PUSHTIMER       0xEE                //上报周期 = 值 * 10ms
#define JD9261T_IRQCRTL         0xFA                //中断控制
#define JD9261T_DISAUTOSLEEP    0xFE                //禁止自动休眠

#define JD9261T_REFRESH_VAL     3                   //30Hz
#define JD9261T_IRQ_CTRL_VAL    0x60

// 坐标方向调整(按实际屏幕方向修改)
#define JD9261T_XY_SWAP         0
#define JD9261T_X_REVERSE       0
#define JD9261T_Y_REVERSE       0

#define JD9261T_I2C_WADDR       ((u8)((TP_IIC_ADDR) << 1))
#define JD9261T_I2C_RADDR       ((u8)(((TP_IIC_ADDR) << 1) | 1))

static u8 jd9261t_buf[JD9261T_READ_LEN];
static u8 jd9261t_cmd[6];

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

// 写一段数据: 首字节作为寄存器/命令字节, 其余字节走DMA, len>=2
static bool jd9261t_write_raw(const u8 *buf, u16 len, bool stop)
{
    if ((NULL == buf) || (len < 2)) {
        return false;
    }

    CTP_IIC->sfr->IICxCMDA = ((u32)buf[0] << 8) | JD9261T_I2C_WADDR;
    CTP_IIC->sfr->IICxCON1 = BIT(10) | BIT(5) | BIT(4) | BIT(3) |
                             (stop ? BIT(11) : 0);       //WDATA|REG_ADDR_0|DEV_ADDR0|START|STOP
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(&buf[1]);
    CTP_IIC->sfr->IICxDMACNT = ((u32)(len - 2) << 16) | BIT(1) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK

    return jd9261t_i2c_wait_done();
}

// 发送 0xF3 + 32bit寄存器地址 + 读命令, 不发STOP, 等待随后的重复起始读操作
static bool jd9261t_send_cmd(u32 reg)
{
    jd9261t_cmd[0] = JD9261T_CMD;
    jd9261t_cmd[1] = (reg >> 24) & 0xFF;
    jd9261t_cmd[2] = (reg >> 16) & 0xFF;
    jd9261t_cmd[3] = (reg >> 8) & 0xFF;
    jd9261t_cmd[4] = reg & 0xFF;
    jd9261t_cmd[5] = JD9261T_CMD_TAIL;

    return jd9261t_write_raw(jd9261t_cmd, sizeof(jd9261t_cmd), false);
}

// KICK读数据, 完成后在I2C中断中解析
AT(.com_text.ctp)
static void jd9261t_read_start(u8 *buf, u16 len)
{
    CTP_IIC->sfr->IICxCON1 = BIT(12) | BIT(11) | BIT(9) | BIT(8) | BIT(7);   //NACK|STOP|RDATA|DEV_ADDR1|START
    CTP_IIC->sfr->IICxCMDA = (u32)JD9261T_I2C_RADDR << 24;
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(buf);
    CTP_IIC->sfr->IICxDMACNT = ((len - 1) << 16) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK
}

// 阻塞读: 发命令后读取数据
static bool jd9261t_read(u32 reg, u8 *buf, u16 len)
{
    if (!jd9261t_send_cmd(reg)) {
        return false;
    }
    jd9261t_read_start(buf, len);
    return jd9261t_i2c_wait_done();
}

// 直接4字节寄存器地址读(不带0xF3命令), 用于兼容不同的ID读取时序
static bool jd9261t_read_plain(u32 reg, u8 *buf, u16 len)
{
    jd9261t_cmd[0] = (reg >> 24) & 0xFF;
    jd9261t_cmd[1] = (reg >> 16) & 0xFF;
    jd9261t_cmd[2] = (reg >> 8) & 0xFF;
    jd9261t_cmd[3] = reg & 0xFF;

    if (!jd9261t_write_raw(jd9261t_cmd, 4, true)) {
        return false;
    }
    jd9261t_read_start(buf, len);
    return jd9261t_i2c_wait_done();
}

// 写8bit寄存器
static bool jd9261t_write_reg(u8 reg, u8 val)
{
    CTP_IIC->sfr->IICxCMDA = ((u32)reg << 8) | JD9261T_I2C_WADDR;
    CTP_IIC->sfr->IICxCON1 = BIT(11) | BIT(10) | BIT(5) | BIT(4) | BIT(3);
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(&val);
    CTP_IIC->sfr->IICxDMACNT = BIT(1) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK

    return jd9261t_i2c_wait_done();
}

bool ctp_jd9261t_init(void)
{
    u8 id[2] = {0};
    u8 id_plain[2] = {0};
    bool found = false;
    int i;

    //复位后芯片需要稳定时间; 读ID匹配不上时换一种时序再试
    for (i = 0; i < 5; i++) {
        memset(id, 0, sizeof(id));
        if (jd9261t_read(JD9261T_REG_CHIP_ID, id, sizeof(id)) && (id[1] == JD9261T_CHIP_ID_CODE)) {
            found = true;
            break;
        }

        memset(id_plain, 0, sizeof(id_plain));
        if (jd9261t_read_plain(JD9261T_REG_CHIP_ID, id_plain, sizeof(id_plain)) &&
            (id_plain[1] == JD9261T_CHIP_ID_CODE)) {
            memcpy(id, id_plain, sizeof(id));
            found = true;
            break;
        }
    }

    TRACE("CTP JD9261T id: %02X %02X, plain id: %02X %02X\n",
          id[0], id[1], id_plain[0], id_plain[1]);
    if (!found) {
        TRACE("CTP JD9261T not found, i2c addr 0x%02X\n", TP_IIC_ADDR);
        return false;
    }

    jd9261t_write_reg(JD9261T_PUSHTIMER, JD9261T_REFRESH_VAL);
    jd9261t_write_reg(JD9261T_IRQCRTL, JD9261T_IRQ_CTRL_VAL);
    jd9261t_write_reg(JD9261T_DISAUTOSLEEP, 0x01);

    TRACE("CTP JD9261T init ok\n");
    return true;
}

//中断里发读报点命令并KICK读(中断上下文禁止打印日志)
AT(.com_text.ctp)
void ctp_jd9261t_readkick(void)
{
    //阻塞发命令期间关闭IIC中断, 避免命令阶段完成标志误触发解析
    sys_ctp_irq_disable(IRQ_I2C_VECTOR);
    if (jd9261t_send_cmd(JD9261T_REG_COORD)) {
        jd9261t_read_start(jd9261t_buf, JD9261T_READ_LEN);
        ctp_kick_status = true;
    } else {
        ctp_kick_status = false;
    }
    sys_ctp_irq_enble(IRQ_I2C_VECTOR);
}

AT(.com_text.ctp)
bool ctp_jd9261t_get_point(s32 *x, s32 *y)
{
    u8 finger_num;
    u8 *p;
    s32 px, py;

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
