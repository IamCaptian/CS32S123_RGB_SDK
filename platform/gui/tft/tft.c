#include "include.h"

#define TRACE_EN                1

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif


static tft_cb_t tft_cb;

#if TFT_RGB_DOUBLE_BUFFER_TEST
static u16 rgb_db_frame[2][GUI_SCREEN_WIDTH * GUI_SCREEN_HEIGHT]
    AT(.psram_buf.rgb_back) ALIGNED(64);

typedef enum {
    RGB_DB_OFF,
    RGB_DB_IDLE,
    RGB_DB_REQUEST,
    RGB_DB_DRAWING,
    RGB_DB_DRAWN,
} rgb_db_state_t;

extern void gpu_share_tft_init(bool rgbw_support, bool ramless_support,
                               void *ramless_buf, u16 ramless_line_wid);

static volatile rgb_db_state_t rgb_db_state;
static u32 rgb_db_frames[3];
static u32 rgb_db_front;
static volatile u32 rgb_db_scanning_next;
static volatile u32 rgb_db_ready;
static u32 rgb_db_render;

// PA14 receives the LCD VSYNC from PA5. TMR1 uses XOSC / 2.
#define RGB_VSYNC_DELAY_US 16500U
#define RGB_VSYNC_POLL_US 40U
#define RGB_VSYNC_POLL_COUNT 88U
#define RGB_VSYNC_CAPTURE_US 150U
#define RGB_VSYNC_TMR_MODE (BIT(7) | BIT(4) | BIT(2))
#define RGB_VSYNC_TMR_TICKS_PER_US (XOSC_CLK_HZ / 2000000U)

static bool rgb_vsync_enabled;
static u16 rgb_vsync_polls_left;

AT(.com_text.tft_spi)
static void tft_rgb_commit_frame(u32 pending)
{
    u32 address = rgb_db_ready;
    if (pending != BIT(31) || !address || rgb_db_scanning_next ||
        rgb_db_state == RGB_DB_OFF || !(LCDCON & BIT(16))) {
        return;
    }

    LCDADR = address;
    rgb_db_scanning_next = address;
    rgb_db_ready = 0;
}

AT(.com_text.tft_spi) __attribute__((noinline))
static void tft_rgb_commit_from_thread(void)
{
    GLOBAL_INT_DISABLE();
    tft_rgb_commit_frame(LCDPEND);
    GLOBAL_INT_RESTORE();
}

AT(.com_text.tft_spi) __attribute__((noinline))
static void tft_rgb_publish_drawn(void)
{
    GLOBAL_INT_DISABLE();
    rgb_db_ready = rgb_db_render;
    GLOBAL_INT_RESTORE();
}

AT(.com_text.tft_spi)
static u32 tft_rgb_capture_rollover(void)
{
    TMR1CON = 0;
    TMR1CNT = 0;
    TMR1PR = 0xffffU;
    TMR1CON = (RGB_VSYNC_TMR_MODE & ~BIT(7)) | BIT(0);

    u32 pending = 0;
    for (unsigned int sample = 0; sample < 12000U; sample++) {
        pending = LCDPEND;
        if (pending == BIT(31) ||
            TMR1CNT >= RGB_VSYNC_TMR_TICKS_PER_US * RGB_VSYNC_CAPTURE_US) {
            break;
        }
    }
    return pending;
}

AT(.com_text.tft_spi)
static void tft_rgb_timer_isr(void)
{
    if (!(TMR1CON & BIT(9))) {
        return;
    }
    TMR1CPND = BIT(9);
    u32 pending = LCDPEND;
    u32 line = (pending >> 16) & 0x1ffU;
    bool near_rollover = pending == BIT(30) ||
                         (!(pending & (BIT(30) | BIT(31))) &&
                          line >= GUI_SCREEN_HEIGHT - 3U && line < GUI_SCREEN_HEIGHT);
    if (near_rollover || pending == BIT(31)) {
        if (near_rollover) {
            pending = tft_rgb_capture_rollover();
        }
        tft_rgb_commit_frame(pending);
        TMR1CON = 0;
    } else if (--rgb_vsync_polls_left == 0) {
        TMR1CON = 0;
    } else if (rgb_vsync_polls_left == RGB_VSYNC_POLL_COUNT - 1U) {
        TMR1CON = 0;
        TMR1CNT = 0;
        TMR1PR = RGB_VSYNC_TMR_TICKS_PER_US * RGB_VSYNC_POLL_US - 1U;
        TMR1CON = RGB_VSYNC_TMR_MODE | BIT(0);
    }
}

AT(.com_text.tft_spi)
static void tft_rgb_vsync_isr(void)
{
    const u32 irq_pending = BIT(16 + PORT_INT1_VECTOR);
    if (!(WKUPEDG & irq_pending)) {
        return;
    }
    WKUPCPND = irq_pending;
    TMR1CON = 0;
    TMR1CPND = BIT(9);
    TMR1CNT = 0;
    TMR1PR = RGB_VSYNC_TMR_TICKS_PER_US * RGB_VSYNC_DELAY_US - 1U;
    rgb_vsync_polls_left = RGB_VSYNC_POLL_COUNT;
    TMR1CON = RGB_VSYNC_TMR_MODE | BIT(0);
}

static void tft_rgb_vsync_start(void)
{
    if (TMR1CON & BIT(0)) {
        return;
    }
    CLKGAT0 |= BIT(1);
    sys_irq_init(IRQ_TMR1_VECTOR, 0, tft_rgb_timer_isr);
    port_gpio_set_in(IO_PA14, GPIOxPU200K);
    WKUPCPND = BIT(16 + PORT_INT1_VECTOR);
    port_irq_register(PORT_INT1_VECTOR, tft_rgb_vsync_isr);
    rgb_vsync_enabled = port_wakeup_init(IO_PA14, 1, 1);
    if (!rgb_vsync_enabled) {
        port_irq_free(PORT_INT1_VECTOR);
        PICEN &= ~BIT(IRQ_TMR1_VECTOR);
    }
}

static void tft_rgb_vsync_stop(void)
{
    if (rgb_vsync_enabled) {
        port_irq_free(PORT_INT1_VECTOR);
        port_wakeup_exit(IO_PA14);
        PICEN &= ~BIT(IRQ_TMR1_VECTOR);
        TMR1CON = 0;
        TMR1CPND = BIT(9);
        rgb_vsync_enabled = false;
    }
}

static bool tft_rgb_in_flip_window(bool allow_short_wait)
{
    const u32 rollover_pending = BIT(31);
    u32 pending = LCDPEND;
    u32 phase = (pending >> 16) & 0x1ffU;
    bool near_last_line = !(pending & (BIT(30) | BIT(31))) &&
                          phase >= GUI_SCREEN_HEIGHT - 32U && phase < GUI_SCREEN_HEIGHT;
    if (allow_short_wait && (near_last_line || pending == BIT(30))) {
        // Keep looking through bit 30 alone. Only the validated rollover permits a write.
        u32 start = tick_get();
        do {
            pending = LCDPEND;
            if (pending == rollover_pending) {
                return true;
            }
        } while (tick_get() - start < 2U);
    }
    return pending == rollover_pending;
}

static void tft_rgb_double_buffer_start(void)
{
    rgb_db_frames[0] = DMA_ADR(lcd_480_st7701s_drv.param.rgb_drv_param.refersh_dma_buf) | BIT(28);
    rgb_db_frames[1] = DMA_ADR(rgb_db_frame[0]) | BIT(28);
    rgb_db_frames[2] = DMA_ADR(rgb_db_frame[1]) | BIT(28);
    rgb_db_front = rgb_db_frames[0];
    rgb_db_scanning_next = 0;
    rgb_db_ready = 0;
    rgb_db_state = RGB_DB_IDLE;
}

static void tft_rgb_start_next_draw(void)
{
#if GUI_USE_SCREENSHOOT
    if (gui_get_screenshot()) {
        return;
    }
#endif
    // The ISR moves ready to scanning_next, so read the two in this order.
    u32 ready = rgb_db_ready;
    u32 scanning_next = rgb_db_scanning_next;
    for (unsigned int i = 0; i < sizeof(rgb_db_frames) / sizeof(rgb_db_frames[0]); i++) {
        u32 address = rgb_db_frames[i];
        if (address == rgb_db_front || address == scanning_next || address == ready) {
            continue;
        }

        rgb_db_render = address;
        gpu_share_tft_init(false, true, (void *)address, GUI_SCREEN_WIDTH);
        gui_refresh();
        rgb_db_state = RGB_DB_REQUEST;
        os_gui_draw_force();
        break;
    }
}

static void tft_rgb_try_submit(bool allow_short_wait)
{
    if (!rgb_db_ready || rgb_db_scanning_next ||
        !tft_rgb_in_flip_window(allow_short_wait)) {
        return;
    }

    tft_rgb_commit_from_thread();
}

void tft_rgb_submit_before_gui(void)
{
    if (rgb_db_state != RGB_DB_OFF && (LCDCON & BIT(16))) {
        tft_rgb_try_submit(false);
    }
}

void tft_rgb_double_buffer_poll(void)
{
    if (rgb_db_state == RGB_DB_OFF || !(LCDCON & BIT(16))) {
        return;
    }

    u32 scanning_next = rgb_db_scanning_next;
    if (scanning_next && LCDADR == scanning_next) {
        rgb_db_front = scanning_next;
        rgb_db_scanning_next = 0;
    }

    if (rgb_db_state == RGB_DB_DRAWN) {
        // Keep the most recent complete frame while the LCD waits for a flip.
        tft_rgb_publish_drawn();
        rgb_db_state = RGB_DB_IDLE;
    }

    tft_rgb_try_submit(true);

    if (rgb_db_state == RGB_DB_IDLE && (rgb_db_scanning_next || LCDADR == rgb_db_front)) {
        tft_rgb_start_next_draw();
    }
}
#endif

tft_cb_t* tft_get_tft_cb(void)
{
    return &tft_cb;
}

AT(.com_text.tft_spi)
static void tft_te_refresh(void)
{
    TICK0CNT = 0;
    compo_cb.rtc_cnt = RTCCNT;
    compo_cb.rtc_cnt2 = RTCCON2;
    compo_cb.rtc_update = true;
#if !TFT_RGB_DOUBLE_BUFFER_TEST
#if GUI_USE_SCREENSHOOT
    if (!gui_get_screenshot())
#endif
    {
        os_gui_draw();
    }
#endif
}

AT(.com_text.tft_spi)
void tft_bglight_en(void)
{
    LCD_BL_EN();            //LCD BL(开背光)
}

AT(.com_text.tft_spi)
void tft_te_isr(void)
{
#if (PORT_TFT_INT != IO_NONE)
    if (WKUPEDG & BIT(16+PORT_TFT_INT_VECTOR)) {
        WKUPCPND = BIT(16+PORT_TFT_INT_VECTOR);

        bool flag_mode_nochange = true;
        if (tft_cb.te_mode != tft_cb.te_mode_next) {
            tft_cb.te_mode = tft_cb.te_mode_next;
            tft_cb.despi_baud = (tft_cb.te_mode > 0) ? tft_cb.despi_baud1 : tft_cb.despi_baud2;
            if (tft_cb.flag_in_frame) {
                tft_cb.tft_set_baud_kick = true;
            } else {
                LCDSPIBAUD = tft_cb.despi_baud;
            }
            flag_mode_nochange = false;
        }
        if (tft_cb.te_mode == 1) {
            //1TE MODE
            TICK1CNT = 0;
            TICK1CON |= BIT(0);         //TICK EN
        } else {
            //>1TE MODE, 如果Mode change停一个TE
            if (flag_mode_nochange) {
                tft_te_refresh();
            }
        }
        //延时打开背光
        if (tft_cb.te_bglight_cnt > 0) {
            tft_cb.te_bglight_cnt--;
            if (tft_cb.te_bglight_cnt == 0) {
                tft_cb.tft_bglight_first_set = true;
            }
        }
    }
#endif
}

AT(.com_text.tft_spi)
void tick_te_isr(void)
{
    if (TICK1CON & BIT(9)) {
        TICK1CON &= ~BIT(0);
        TICK1CPND = BIT(9);
        tft_te_refresh();
#if (PORT_TFT_INT == IO_NONE)
        TICK1CON |= BIT(0);
        //延时打开背光
        if (tft_cb.te_bglight_cnt > 0) {
            tft_cb.te_bglight_cnt--;
            if (tft_cb.te_bglight_cnt == 0) {
                tft_cb.tft_bglight_first_set = true;
            }
        }
#endif
    }
}

//将TE量化到0~100, 由于TE中断不及时，可能偶尔会超过100
AT(.com_text.tft_spi)
int tft_te_getnorm(void)
{
#if (PORT_TFT_INT == IO_NONE)
    return 0;
#else
    return TICK0CNT * 100 * 64 / (int)((XOSC_CLK_HZ / 1000) * TFT_TE_CYCLE);
#endif
}

//设置1TE / 2TE的波特率
void tft_set_baud(u8 baud1, u8 baud2)
{
    tft_cb.despi_baud1 = baud1;
    tft_cb.despi_baud2 = baud2;
    tft_cb.despi_baud = tft_cb.te_mode ? tft_cb.despi_baud1 : tft_cb.despi_baud2;
}

AT(.com_text.tft_spi)
void tft_frame_start(void)
{
    if (LCDCON == 0) {
        return;
    }

    tft_cb.flag_in_frame = true;
#if TFT_RGB_DOUBLE_BUFFER_TEST
    if (rgb_db_state == RGB_DB_REQUEST) {
        rgb_db_state = RGB_DB_DRAWING;
    }
#endif
    tft_write_data_start();
}

AT(.com_text.tft_spi)
void tft_frame_end(void)
{
    if (LCDCON == 0) {
        return;
    }

    tft_write_end();
    tft_cb.flag_in_frame = false;
    if (tft_cb.tft_bglight_kick) {
        tft_cb.tft_bglight_kick = false;
        tft_cb.te_bglight_cnt = 3; //3TE后打开背光
    }
    if (tft_cb.tft_set_baud_kick) {
        tft_cb.tft_set_baud_kick = false;
        LCDSPIBAUD = tft_cb.despi_baud;
    }
#if TFT_RGB_DOUBLE_BUFFER_TEST
    if (rgb_db_state == RGB_DB_DRAWING) {
        rgb_db_state = RGB_DB_DRAWN;
    }
#endif
}

//背光亮度初始设置检测
void tft_bglight_frist_set_check(void)
{
    if(!tft_cb.tft_bglight_first_set) {
        return ;
    }
    tft_cb.tft_bglight_first_set = false;

    tft_cb.tft_bglight_last_duty = 0;
    //todo:后续根据客户定制调整
    if(0 == tft_cb.tft_bglight_duty)
    {
        tft_cb.tft_bglight_duty = GUI_DEFAULT_BK;
        sys_cb.light_level = 5;
    }
    lcd_drv_set_brightness(tft_cb.tft_bglight_duty);
}

//设置TE MODE
void tft_set_temode(u8 mode)
{
    tft_cb.te_mode_next = mode;
}


void tft_init(void)
{

#if (PORT_TFT_INT != IO_NONE)
    CLKGAT0 |= BIT(30);                                 //TICK0
    delay_us(1);                                        //set CLKGAT0需要时间生效
    TICK0CON = BIT(6) | BIT(5) | BIT(2);                //div64[6:4], xosc26m[3:1]
    TICK0PR = 0xFFFF;
    TICK0CNT = 0;
    TICK0CON |= BIT(0);                                 //TICK EN

    CLKGAT0 |= BIT(31);                                 //TICK1
    TICK1CON = BIT(7) | BIT(6) | BIT(5) | BIT(2);       //TIE, div64[6:4], xosc26m[3:1]
    TICK1PR = (int)((XOSC_CLK_HZ / 1000) * TFT_TE_CYCLE_DELAY) / 64;
    TICK1CNT = 0;
    sys_irq_init(IRQ_TE_TICK_VECTOR, 0, tick_te_isr);

    tft_cb.te_mode = 0;                                 //初始化
    tft_cb.te_mode_next = 0;
    tft_set_temode(DEFAULT_TE_MODE);
#else
    CLKGAT0 |= BIT(31);                                 //TICK1
    TICK1CON = BIT(7) | BIT(6) | BIT(5) | BIT(2);       //TIE, div64[6:4], xosc26m[3:1]
    TICK1PR = (int)((XOSC_CLK_HZ / 1000) * TFT_TE_CYCLE) / 64;
    TICK1CNT = 0;
    TICK1CON |= BIT(0);
    sys_irq_init(IRQ_TE_TICK_VECTOR, 0, tick_te_isr);
#endif

#if (GUI_SELECT == GUI_TFT_320_ST77916)
    lcd_drv_register(&lcd_320_st77916_drv);
#elif (GUI_SELECT == GUI_OLED_466_ICNA3310B)
    lcd_drv_register(&lcd_oled_466_icna3310b_drv);
#elif (GUI_SELECT == GUI_OLED_368_ST7801N)
    lcd_drv_register(&lcd_oled_368_st7801n_drv);
#elif (GUI_SELECT == GUI_TFT_240_ST789_i80)
    lcd_drv_register(&lcd_240_st7789V3_i80_drv);
#elif (GUI_SELECT == GUI_LCD_480_ST7283)
    lcd_drv_register(&lcd_480_st7283_drv);
#elif (GUI_SELECT == GUI_LCD_480_ST7701S)
    lcd_drv_register(&lcd_480_st7701s_drv);
#elif (GUI_SELECT == GUI_LCD_800_ST7265)
    lcd_drv_register(&lcd_800_st7265_drv);
#elif (GUI_SELECT == GUI_TFT_360_JD9855)
    lcd_drv_register(&lcd_360_jd9855_drv);
#elif (GUI_SELECT == GUI_VGS_640)
    lcd_drv_register(&lcd_vga012a_drv);
#endif

    LCDSPIBAUD = 30;      //读ID建议在20M以内
    TRACE("TFT ID: %x\n", lcd_drv_read_id());
    LCDSPIBAUD = tft_cb.despi_baud;
    lcd_drv_init();
    lcd_drv_set_window(0, 0, GUI_SCREEN_WIDTH - 1, GUI_SCREEN_HEIGHT - 1);
    tft_cb.tft_bglight_kick = true; //延时打开背光
#if TFT_RGB_DOUBLE_BUFFER_TEST
    tft_rgb_double_buffer_start();
    tft_rgb_vsync_start();
#endif
}

void tft_exit(void)
{
#if TFT_RGB_DOUBLE_BUFFER_TEST
    tft_rgb_vsync_stop();
    TICK1CON = 0;
    rgb_db_state = RGB_DB_OFF;
    os_gui_draw_w4_done();
#endif
    if(!vddio_sleep_level) {
//        bsp_pwm_disable(PORT_TFT_BL); //关背光
        lcd_drv_set_brightness(0);
//    printf("%s:%d\n",__func__,__LINE__);
    }

    lcd_drv_deregister();

    TICK0CON = 0;
    TICK1CON = 0;
}
