#include "include.h"
// 360 * 360

#if (GUI_SELECT == GUI_TFT_360_JD9855)

static void tft_360_jd9855_init(void)
{
    printf("tft_360_jd9855_init\n");

    delay_ms(10);

    WriteComm(0xde);
    WriteData(0x00);

    WriteComm(0xdf);
    WriteData(0x98);
    WriteData(0x55);

    WriteComm(0xce);        // QSPI Read lane option, 00=D1 read; 40=D0 read
    WriteData(0x0d);
    WriteData(0x00);

    WriteComm(0xb2);        // FMA=6%
    WriteData(0x1f);

    WriteComm(0xb7);
    WriteData(0x01);
    WriteData(0x2d);
    WriteData(0x01);
    WriteData(0x55);

    WriteComm(0xbb);        // VGH=12V, VGL=-11.5V
    WriteData(0x1b);
    WriteData(0x64);
    WriteData(0xc4);
    WriteData(0x0e);
    WriteData(0x3e);
    WriteData(0xf5);

    WriteComm(0xbc);
    WriteData(0x03);
    WriteData(0x22);
    WriteData(0xf3);
    WriteData(0xc0);

    WriteComm(0xc0);
    WriteData(0x22);
    WriteData(0xa4);

    WriteComm(0xc3);
    WriteData(0x00);
    WriteData(0x02);
    WriteData(0x2a);
    WriteData(0x0b);
    WriteData(0x08);
    WriteData(0x48);
    WriteData(0x08);
    WriteData(0x04);
    WriteData(0x62);
    WriteData(0x30);
    WriteData(0x30);

    WriteComm(0xc4);
    WriteData(0x40);
    WriteData(0x00);
    WriteData(0xad);
    WriteData(0x68);
    WriteData(0x43);
    WriteData(0x07);
    WriteData(0x04);
    WriteData(0x16);
    WriteData(0x43);
    WriteData(0x07);
    WriteData(0x04);

    WriteComm(0xc8);
    for (int i = 0; i < 2; i++) {
        WriteData(0x3f);
        WriteData(0x31);
        WriteData(0x28);
        WriteData(0x25);
        WriteData(0x25);
        WriteData(0x27);
        WriteData(0x22);
        WriteData(0x22);
        WriteData(0x20);
        WriteData(0x1f);
        WriteData(0x1c);
        WriteData(0x12);
        WriteData(0x0f);
        WriteData(0x0b);
        WriteData(0x02);
        WriteData(0x02);
    }

    WriteComm(0xd3);
    WriteData(0x28);
    WriteData(0x13);

    WriteComm(0xd7);
    WriteData(0x00);
    WriteData(0x30);

    WriteComm(0xd9);        // OTP_PRO_TIMES
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0xff);
    WriteData(0x00);
    WriteData(0xf0);
    WriteData(0x00);

    // page1
    WriteComm(0xde);
    WriteData(0x01);

    WriteComm(0xb7);
    WriteData(0x13);
    WriteData(0xe7);
    WriteData(0x64);
    WriteData(0x39);
    WriteData(0x06);
    WriteData(0x36);
    WriteData(0x19);
    WriteData(0x1c);

    WriteComm(0xbe);
    WriteData(0x00);

    WriteComm(0xc1);        // SD_V0
    WriteData(0x05);
    WriteData(0x4a);
    WriteData(0x80);

    WriteComm(0xc2);
    WriteData(0x00);
    WriteData(0x16);
    WriteData(0xda);
    WriteData(0xe7);

    WriteComm(0xc7);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x38);
    WriteData(0x08);
    WriteData(0x08);
    WriteData(0x00);
    WriteData(0x01);

    WriteComm(0xc8);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x15);
    WriteData(0x3b);

    WriteComm(0xc9);
    WriteData(0x06);
    WriteData(0x04);
    WriteData(0x0a);
    WriteData(0x08);
    WriteData(0x1e);

    WriteComm(0xca);
    WriteData(0x00);
    WriteData(0x35);
    WriteData(0x15);
    WriteData(0x1f);
    WriteData(0x1f);

    WriteComm(0xcb);
    WriteData(0x07);
    WriteData(0x05);
    WriteData(0x0b);
    WriteData(0x09);
    WriteData(0x1e);

    WriteComm(0xcc);
    WriteData(0x01);
    WriteData(0x35);
    WriteData(0x15);
    WriteData(0x1f);
    WriteData(0x1f);

    WriteComm(0xcd);
    WriteData(0x09);
    WriteData(0x0b);
    WriteData(0x05);
    WriteData(0x07);
    WriteData(0x1e);

    WriteComm(0xce);
    WriteData(0x01);
    WriteData(0x35);
    WriteData(0x1f);
    WriteData(0x15);
    WriteData(0x1f);

    WriteComm(0xcf);
    WriteData(0x08);
    WriteData(0x0a);
    WriteData(0x04);
    WriteData(0x06);
    WriteData(0x1e);

    WriteComm(0xd0);
    WriteData(0x00);
    WriteData(0x35);
    WriteData(0x1f);
    WriteData(0x15);
    WriteData(0x1f);

    WriteComm(0xd1);
    WriteData(0x02);
    WriteData(0x30);

    WriteComm(0xd2);
    WriteData(0x02);
    WriteData(0x03);
    WriteData(0x52);
    WriteData(0xdf);
    WriteData(0xdd);

    WriteComm(0xd3);
    WriteData(0x3b);
    WriteData(0x04);
    WriteData(0x48);

    WriteComm(0xd5);
    WriteData(0x10);
    WriteData(0x10);
    WriteData(0x07);
    WriteData(0x07);
    WriteData(0x0f);
    WriteData(0x94);
    WriteData(0x26);

    WriteComm(0xd6);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x40);

    WriteComm(0xd7);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x20);

    // page2
    WriteComm(0xde);
    WriteData(0x02);

    WriteComm(0xb6);
    WriteData(0x1c);

    // page0
    WriteComm(0xde);
    WriteData(0x00);

    // 清内部RAM
    WriteComm(0x4d);
    WriteData(0x00);

    WriteComm(0x4e);
    WriteData(0x00);

    WriteComm(0x4f);
    WriteData(0x00);

    WriteComm(0x4c);
    WriteData(0x01);

    delay_ms(10);

    WriteComm(0x4c);
    WriteData(0x00);

    WriteComm(0x36);        // 00=FW; 03=BW
    WriteData(0x00);

    WriteComm(0x35);        // TEON
    WriteData(0x00);

    WriteComm(0x2a);        // Start_X=0, End_X=360
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x01);
    WriteData(0x68);

    WriteComm(0x2b);        // Start_Y=0, End_Y=360
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x01);
    WriteData(0x68);

    WriteComm(0x3a);        // 55=RGB565
    WriteData(0x55);

    WriteComm(0x11);
    CommEnd();
    delay_ms(120);

    WriteComm(0x29);
    CommEnd();
    delay_ms(20);
}

AT(.com_text.tft_spi)
static void tft_360_jd9855_set_window(u16 x0, u16 y0, u16 x1, u16 y1)
{
    x0 += GUI_SCREEN_OFS_X;
    x1 += GUI_SCREEN_OFS_X;
    y0 += GUI_SCREEN_OFS_Y;
    y1 += GUI_SCREEN_OFS_Y;

    tft_write_cmd(0x2A);        //TFT_CASET
    tft_write_data(BYTE1(x0));
    tft_write_data(BYTE0(x0));
    tft_write_data(BYTE1(x1));
    tft_write_data(BYTE0(x1));

    tft_write_cmd(0x2B);        //TFT_PASET
    tft_write_data(BYTE1(y0));
    tft_write_data(BYTE0(y0));
    tft_write_data(BYTE1(y1));
    tft_write_data(BYTE0(y1));
    tft_write_end();
}

//0x0b: QSPI寄存器读
static void tft_read_id_cmd(u8 cmd)
{
    TFT_SPI_CS_DIS();
    delay_us(1);
    TFT_SPI_CS_EN();
    LCDSPICON = (LCDSPICON & ~(0x03<<2)) | (0x00<<2);       //1data in 1data out
    u8 lcdcon = LCDCON;
    LCDCON = lcdcon | BIT(21);
    tft_spi_sendbyte(0x0b);
    tft_spi_sendbyte(0x00);
    tft_spi_sendbyte(cmd);
    tft_spi_sendbyte(0x00);
}

static uint32_t tft_read_id(void)
{
    u32 lcdcon_bak = LCDCON;
    u32 lcdspicon_bak = LCDSPICON;
    uint32_t id = 0;
    tft_read_id_cmd(0x04);
    id = tft_spi_getbyte();
    id = (id << 8) + tft_spi_getbyte();
    id = (id << 8) + tft_spi_getbyte();
    id = (id << 8) + tft_spi_getbyte();
    tft_write_end();
    LCDCON = lcdcon_bak;
    LCDSPICON = lcdspicon_bak;
    return id;
}

static void tft_360_jd9855_set_brightness(uint8_t brightness)
{
    tft_cb_t* tft_get_tft_cb(void);
    tft_cb_t *tft_cb = tft_get_tft_cb();

    int8_t  base_duty = 0;  //根据限流电阻调整占空比增益
    int8_t  duty = 0;

    if(100 < brightness)
    {
        brightness = 100;
    }
    duty = base_duty + brightness;
    tft_cb->tft_bglight_duty = duty;

    if (tft_cb->tft_bglight_last_duty != tft_cb->tft_bglight_duty)
    {
        bsp_pwm_duty_set(PORT_TFT_BL, tft_cb->tft_bglight_duty, false);
        tft_cb->tft_bglight_last_duty = tft_cb->tft_bglight_duty;
    }
}

lcd_drv_t lcd_360_jd9855_drv = {
    .io = {
        .spi_drv_io = {
            .data = {
                .dio = {
                    .d0 = PORT_TFT_LCD_D0,
                    .d1 = PORT_TFT_LCD_D1,
                    .d2 = PORT_TFT_LCD_D2,
                    .d3 = PORT_TFT_LCD_D3,
                    .d4 = PORT_TFT_LCD_D4,
                    .d5 = PORT_TFT_LCD_D5,
                    .d6 = PORT_TFT_LCD_D6,
                    .d7 = PORT_TFT_LCD_D7,
                }
            },

            .control = {
                .cio = {
                    .dc = PORT_TFT_DC,
                    .cs = PORT_TFT_CS,
                    .rst = PORT_TFT_RST,
                    .te = PORT_TFT_INT,
                    .scl = PORT_TFT_LCD_SCL,
                }
            },
        },
    },

    .io_type        = LCD_IO_QSPI,
    .param          = {
        .spi_drv_param = {
            .clk_select = LCD_SELECT_PLL0_DIV2,
            .clk_div = 0,
            .speed1_mhz = 20,        //QSPI降频验证: 294.5/(35+1)=8.2MHz
            .speed2_mhz = 20,


            // .speed1_mhz = 41,       //JD9855 QSPI最大50M
            // .speed2_mhz = 31,
        },
    },
    .tft_reg_init   = tft_360_jd9855_init,
    .tft_set_window = tft_360_jd9855_set_window,
    .tft_read_id    = tft_read_id,
    .tft_set_brightness = tft_360_jd9855_set_brightness,
    .tft_te_isr     = tft_te_isr,
};
#endif
