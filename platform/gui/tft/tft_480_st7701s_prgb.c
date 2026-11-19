#include "include.h"
// ST7701S 480 * 480 RGB565(PRGB16) 屏
// 屏幕固件已完成初始化，SDK 只需发送 Sleep Out / Display On

#if (GUI_SELECT == GUI_LCD_480_ST7701S)

#ifndef PORT_LCD_RST
#define PORT_LCD_RST                    IO_NONE
#endif
#ifndef PORT_LCD_SPI_CS
#define PORT_LCD_SPI_CS                 IO_NONE
#endif
#ifndef PORT_LCD_SPI_SCL
#define PORT_LCD_SPI_SCL                IO_NONE
#endif
#ifndef PORT_LCD_SPI_SDA
#define PORT_LCD_SPI_SDA                IO_NONE
#endif

#if (PORT_LCD_RST == IO_NONE) || (PORT_LCD_SPI_CS == IO_NONE) \
    || (PORT_LCD_SPI_SCL == IO_NONE) || (PORT_LCD_SPI_SDA == IO_NONE)
#error "ST7701S: please config PORT_LCD_RST/PORT_LCD_SPI_CS/SCL/SDA in config.h"
#endif

static u8 refersh_dma_buf[GUI_SCREEN_WIDTH*GUI_SCREEN_HEIGHT*GUI_COLOR_DEPTH] AT(.psram_buf.lcd);

static void st7701s_io_out_init(u8 io_num)
{
    gpio_t gpio;

    gpio_cfg_init(&gpio, io_num);
    gpio.sfr[GPIOxFEN] &= ~BIT(gpio.num);
    gpio.sfr[GPIOxDE]  |= BIT(gpio.num);
    gpio.sfr[GPIOxDIR] &= ~BIT(gpio.num);
}

static void st7701s_io_out(u8 io_num, bool level)
{
    gpio_t gpio;

    gpio_cfg_init(&gpio, io_num);
    if (level) {
        gpio.sfr[GPIOxSET] = BIT(gpio.num);
    } else {
        gpio.sfr[GPIOxCLR] = BIT(gpio.num);
    }
}

// 3-wire 9bit SPI: 第9位为DC(0:cmd, 1:data), MSB first
static void st7701s_spi_write9(u8 dc, u8 data)
{
    u16 val = ((u16)dc << 8) | data;

    st7701s_io_out(PORT_LCD_SPI_SCL, 0);

    for (u8 i = 0; i < 9; i++) {
        st7701s_io_out(PORT_LCD_SPI_SDA, (val & 0x100) ? 1 : 0);
        delay_us(1);
        st7701s_io_out(PORT_LCD_SPI_SCL, 1);
        delay_us(1);
        st7701s_io_out(PORT_LCD_SPI_SCL, 0);
        val <<= 1;
    }
}

#define SPI_WriteComm(c)                st7701s_spi_write9(0, (c))

static void tft_480_st7701s_init(void)
{
    printf("tft_480_st7701s_init\n");

    st7701s_io_out_init(PORT_LCD_RST);
    st7701s_io_out_init(PORT_LCD_SPI_CS);
    st7701s_io_out_init(PORT_LCD_SPI_SCL);
    st7701s_io_out_init(PORT_LCD_SPI_SDA);

    st7701s_io_out(PORT_LCD_SPI_CS, 1);
    st7701s_io_out(PORT_LCD_SPI_SCL, 0);

    //--------------------------------------ST7701 Reset Sequence---------------------------------------//
    delay_ms(10);
    st7701s_io_out(PORT_LCD_RST, 0);
    delay_ms(10);
    st7701s_io_out(PORT_LCD_RST, 1);
    delay_ms(10);

    st7701s_io_out(PORT_LCD_SPI_CS, 0);

    SPI_WriteComm(0x11);                            //Sleep Out
    delay_ms(120);

    SPI_WriteComm(0x29);                            //Display On
    delay_ms(20);

    st7701s_io_out(PORT_LCD_SPI_CS, 1);
}

lcd_drv_t lcd_480_st7701s_drv = {
    .io = {
        .rgb_drv_io = {
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
                    .d8 = PORT_TFT_LCD_D8,
                    .d9 = PORT_TFT_LCD_D9,
                    .d10 = PORT_TFT_LCD_D10,
                    .d11 = PORT_TFT_LCD_D11,
                    .d12 = PORT_TFT_LCD_D12,
                    .d13 = PORT_TFT_LCD_D13,
                    .d14 = PORT_TFT_LCD_D14,
                    .d15 = PORT_TFT_LCD_D15,
                }
            },

            .control = {
                .cio = {
                    .display = PORT_LCD_DISPLAY,
                    .de = PORT_LCD_DE,
                    .clk = PORT_TFT_LCD_SCL,
                    .vsync = PORT_LCD_VSYNC,
                    .hsync = PORT_LCD_HSYNC,
                }
            },
        },
    },

    .io_type = LCD_IO_PRGB16,
    .param = {
        .rgb_drv_param = {
            .clk_select     = LCD_SELECT_XOSC_X2CLK,
            .clk_div        = 1,                    //48M/2 = 24MHz
            .in_rgb         = LCD_IN_RGB565,
            .out_rgb        = LCD_OUT_RGB888,
            .vsync_width    = 2,
            .vfront_proch   = 108,
            .vback_porch    = 10,
            .hsync_width    = 3,
            .hfront_porch   = 200,
            .hback_porch    = 200,
            .refersh_dma_buf = (lcd_color_t*)refersh_dma_buf,
            .refersh_dma_buf_size = GUI_SCREEN_WIDTH*GUI_SCREEN_HEIGHT*GUI_COLOR_DEPTH,
        },
    },

    .tft_reg_init = tft_480_st7701s_init,
    .tft_set_window = NULL,
    .tft_read_id = NULL,
    .tft_set_brightness = NULL,
    .tft_te_isr = NULL,
};

#endif
