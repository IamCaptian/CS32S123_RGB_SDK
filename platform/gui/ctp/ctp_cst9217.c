#include "include.h"

#if (CTP_SELECT == CTP_CST9217)

// CST9217 触控芯片驱动
// I2C地址: 0x5A (7bit)
// 读报点: 从寄存器 0xD000 连读 7 字节(单点), 读完后向 0xD000 写 0xAB 应答
// 报文: 点0[5] + 点数[1] + 校验[1]
//   点0[0]: bit[7:4]=id, bit[3:0]=event(0x06=按下, 0x07=持续, 0x00=抬起)
//   点0[1]: x 高8bit  点0[2]: y 高8bit
//   点0[3]: bit[7:4]=x 低4bit, bit[3:0]=y 低4bit
//   点数[0]: bit[6:0]=触摸点数  校验: 固定 0xAB

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

#define CST9217_REG_READ        0xD000
#define CST9217_REG_CMD         0xD1
#define CST9217_ACK             0xAB
#define CST9217_READ_LEN        7                   //点0(5) + 点数(1) + 校验(1)

#define CST9217_EVENT_DOWN      0x06
#define CST9217_EVENT_MOVE      0x07
#define CST9217_IS_PRESSED(e)   (((e) == CST9217_EVENT_DOWN) || ((e) == CST9217_EVENT_MOVE))

// 原始报点 Y 方向与屏幕相反，需要上下翻转
#define CST9217_Y_REVERSE       1

#define CST9217_I2C_WADDR       ((u8)((TP_IIC_ADDR) << 1))
#define CST9217_I2C_RADDR       ((u8)(((TP_IIC_ADDR) << 1) | 1))

static u8 ctp_cst9217_buf[CST9217_READ_LEN];

extern i2c_t *CTP_IIC;
extern bool ctp_kick_status;

// 写寄存器: reg_len = 寄存器地址字节数(命令0xD1用1字节, 报点0xD000用2字节), 先发高字节
// 注意: 中断里也会调用(回写0xAB应答), 所以直接操作IIC寄存器并轮询, 不使用带OS锁的接口
static bool ctp_cst9217_write_regs(u16 reg, u8 reg_len, const u8 *data, u16 len)
{
    u32 ticks;

    if ((reg_len != 1 && reg_len != 2) || (len == 0)) {
        return false;
    }

    if (reg_len == 2) {
        CTP_IIC->sfr->IICxCMDA = ((u32)(reg & 0xFF) << 16) | ((u32)(reg >> 8) << 8) | CST9217_I2C_WADDR;
    } else {
        CTP_IIC->sfr->IICxCMDA = ((u32)(reg & 0xFF) << 8) | CST9217_I2C_WADDR;
    }
    CTP_IIC->sfr->IICxCON1 = BIT(11) | BIT(10) | BIT(5) | BIT(4) | BIT(3) |
                             ((reg_len == 2) ? BIT(6) : 0);       //STOP|WDATA|REG_ADDR_x|DEV_ADDR0|START
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(data);
    CTP_IIC->sfr->IICxDMACNT = ((len - 1) << 16) | BIT(1) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK

    ticks = tick_get();
    while (!(CTP_IIC->sfr->IICxCON0 & BIT(31))) {
        if (tick_check_expire(ticks, 20)) {
            return false;
        }
    }
    CTP_IIC->sfr->IICxCON0 |= BIT(29);                //Clear Pending

    return true;
}

bool ctp_cst9217_init(void)
{
    u8 cmd = 0x01;                                  //0xD1写0x01, 芯片有ACK即认为存在
    int i;

    for (i = 0; i < 5; i++) {
        if (ctp_cst9217_write_regs(CST9217_REG_CMD, 1, &cmd, 1)) {
            TRACE("CTP CST9217 init ok\n");
            return true;
        }
    }
    TRACE("CTP CST9217 init fail, i2c addr 0x%02X no ack\n", TP_IIC_ADDR);
    return false;
}

// 中断里KICK读报点(16bit寄存器地址0xD000: addr0=0xD0, addr1=0x00)
AT(.com_text.ctp)
void ctp_cst9217_readkick(void)
{
    CTP_IIC->sfr->IICxCMDA = ((u32)CST9217_I2C_RADDR << 24) |
                             ((u32)(CST9217_REG_READ & 0xFF) << 16) |
                             ((u32)(CST9217_REG_READ >> 8) << 8) |
                             CST9217_I2C_WADDR;
    CTP_IIC->sfr->IICxCON1 = BIT(12) | BIT(11) | BIT(9) | BIT(8) | BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3);
    CTP_IIC->sfr->IICxDMAADR = DMA_ADR(ctp_cst9217_buf);
    CTP_IIC->sfr->IICxDMACNT = ((CST9217_READ_LEN - 1) << 16) | BIT(0);
    CTP_IIC->sfr->IICxCON0 |= BIT(28);                //KICK
    ctp_kick_status = true;
}

AT(.com_text.ctp)
bool ctp_cst9217_get_point(s32 *x, s32 *y)
{
    u8 id;
    u8 event;
    u8 touch_num;
    u8 ack = CST9217_ACK;
    bool press = false;

    //报文有效: buf[0]!=0xAB(有新报文) 且 buf[6]==0xAB(校验通过)
    if ((ctp_cst9217_buf[0] != CST9217_ACK) && (ctp_cst9217_buf[6] == CST9217_ACK)) {
        id = ctp_cst9217_buf[0] >> 4;
        event = ctp_cst9217_buf[0] & 0x0F;
        touch_num = ctp_cst9217_buf[5] & 0x7F;

        if ((id == 0) && (touch_num > 0) && CST9217_IS_PRESSED(event)) {
            *x = ((s32)ctp_cst9217_buf[1] << 4) | (ctp_cst9217_buf[3] >> 4);
            *y = ((s32)ctp_cst9217_buf[2] << 4) | (ctp_cst9217_buf[3] & 0x0F);
#if CST9217_Y_REVERSE
            *y = (GUI_SCREEN_HEIGHT - 1) - *y;      //上下翻转，与屏幕方向一致
#endif
            press = true;
        }
    }

    //读报文后必须回写0xAB应答, 否则芯片要等内部超时才准备下一帧
    (void)ctp_cst9217_write_regs(CST9217_REG_READ, 2, &ack, 1);

    return press;
}

#endif //CTP_CST9217
