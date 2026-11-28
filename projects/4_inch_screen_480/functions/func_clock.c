#include "include.h"
#include "func.h"
#include "func_clock.h"
#include "user_ui/func_user_home.h"

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

#define DIALPLATE_NUM               (sizeof(dialplate_info) / sizeof(u32))
#define DIALPLATE_BTF_IDX           DIALPLATE_NUM - 1        //蝴蝶表盘默认最后一个
#define DIALPLATE_CUBE_IDX          DIALPLATE_NUM - 2        //立方体表盘默认倒数第二个
#define DIALPLATE_WINDMILL_IDX      DIALPLATE_NUM - 3        //风车表盘默认倒数第三个
#define DIALPLATE_FISH_IDX          DIALPLATE_NUM - 4        //鱼表盘默认倒数第四个
#define DIALPLATE_HOURGLASS_IDX     DIALPLATE_NUM - 5        //沙漏表盘默认倒数第四个
#define DIALPLATE_COMPASS_IDX       DIALPLATE_NUM - 6        //指南针表盘默认倒数第六个
#define DIALPLATE_TIMEMAP_IDX       DIALPLATE_NUM - 8

const u32 dialplate_info[] = {
    UI_BUF_DIALPLATE_1_BIN,

//    UI_BUF_DIALPLATE_2_BIN,
//    UI_BUF_DIALPLATE_5_BIN,
//    UI_BUF_DIALPLATE_4_BIN,
//    UI_BUF_DIALPLATE_7_BIN,
     UI_BUF_DIALPLATE_8_BIN,                //精准时图

#if AVI_DIALPLATE_EN && VIDEO_PLAY_EN
    UI_BUF_DIALPLATE_AVI_2_BIN,
#endif // AVI_DIALPLATE_EN

    UI_BUF_DIALPLATE_COMPASS_BIN,         //指南针
    UI_BUF_DIALPLATE_HOURGLASS_BIN,       //沙漏
    UI_BUF_DIALPLATE_FISH_BIN,            //鱼
    UI_BUF_DIALPLATE_WINDMILL_BIN,        //风车
    UI_BUF_DIALPLATE_CUBE_BIN,
    UI_BUF_DIALPLATE_BTF_BIN,

};

//表盘快捷按钮编号表
const u8 quick_btn_tbl[] =
{
    FUNC_NULL,
    FUNC_HEARTRATE,
    FUNC_BT,
    FUNC_ALARM_CLOCK,
    FUNC_BLOOD_OXYGEN,
    FUNC_BLOODSUGAR,
    FUNC_BLOOD_PRESSURE,
    FUNC_BREATHE,
    FUNC_CALCULATOR,

    FUNC_CAMERA,
    FUNC_TIMER,
    FUNC_SLEEP,
    FUNC_STOPWATCH,
    FUNC_WEATHER,
    FUNC_GAME,
    FUNC_STYLE,
    FUNC_ALTITUDE,
    FUNC_MAP,
    FUNC_MESSAGE,
    FUNC_SCAN,
    FUNC_VOICE,
    FUNC_COMPASS,
    FUNC_ADDRESS_BOOK,
    FUNC_SPORT,
    FUNC_CALL,
    FUNC_FINDPHONE,
    FUNC_CALENDAER,
    FUNC_ACTIVITY,
    FUNC_FLASHLIGHT,
    FUNC_SETTING,
};

bool func_clock_time_map_swithing(void);
int compo_get_animation_id(void);
void compo_animation_manual_next(compo_animation_t *animation);
void func_switch_to(u8 sta, u16 switch_mode);
compo_form_t *func_clock_cube_form_create(void);
compo_form_t *func_clock_butterfly_form_create(void);
compo_form_t *func_clock_windmill_form_create(void);
compo_form_t *func_clock_fish_form_create(void);
compo_form_t *func_clock_hourglass_form_create(void);
compo_form_t *func_clock_compass_form_create(void);
void func_clock_cube_message(size_msg_t msg);
void func_clock_butterfly_message(size_msg_t msg);
void func_clock_windmill_message(size_msg_t msg);
void func_clock_fish_message(size_msg_t msg);
void func_clock_hourglass_message(size_msg_t msg);
void func_clock_compass_message(size_msg_t msg);
void func_clock_cube_process(void);
void func_clock_butterfly_process(void);
void func_clock_windmill_process(void);
void func_clock_fish_process(void);
void func_clock_hourglass_process(void);
void func_clock_windmill_pbubbles_destory(void);
void func_clock_compass_process(void);
void func_clock_time_map_init(void);

u8 func_clock_compass_init();
u16 func_clock_preview_get_type(void);

u8 func_clock_get_max_dialplate_num(void)
{
    return (sizeof(dialplate_info) / sizeof(u32));
}

u32 func_clock_get_dialplate_info(u8 index)
{
    return dialplate_info[index];
}

u32 func_clock_get_dialplate_cube_idx(void)
{
    return DIALPLATE_CUBE_IDX;
}

u32 func_clock_get_dialplate_butterfly_idx(void)
{
    return DIALPLATE_BTF_IDX;
}

//创建主页窗体（原时钟表盘已被用户主页替代）
compo_form_t *func_clock_form_create(void)
{
    return func_user_home_form_create();
}

#if GUI_USE_BLUR
//截图
compo_form_t *func_clock_form_create_by_screenshoot(void)
{
    compo_form_t *frm = compo_form_create(true);
        //新建图像
    compo_picturebox_t *pic = compo_picturebox_create(frm, 0);
    compo_picturebox_set_pos(pic, GUI_SCREEN_CENTER_X, GUI_SCREEN_CENTER_Y);
    gui_set_ram_check(cur_scbuf, __func__);      //检测ram对不对
    compo_picturebox_set_ram(pic, cur_scbuf);
    f_clock_t *f_clk = (f_clock_t *)func_cb.f_cb;
    f_clk->blur_pic = pic;

    return frm;
}
#endif

//子功能公共事件处理
void func_clock_sub_process(void)
{
    func_process();                                     //刷新UI
}

//子功能公共消息处理
void func_clock_sub_message(size_msg_t msg)
{
    func_message(msg);
}

//主页功能事件处理
static void func_clock_process(void)
{
    func_process();                                  //刷新UI
}

//主页功能消息处理
static void func_clock_message(size_msg_t msg)
{
    switch (msg) {
    case MSG_CTP_SHORT_RIGHT:
        //主页右滑循环切到排序表中的最后一个功能页
        func_cb.flag_sort = true;
        func_switch_to(func_cb.tbl_sort[func_cb.sort_cnt - 1], func_get_switching_mode_byidx(sys_cb.nav_index, false));
        
        break;

    default:
        func_message(msg);
        break;
    }
}


//进入主页功能
void func_clock_enter(void)
{
    func_cb.flag_sort = false;                      //主页不属于快捷翻页页面
    func_cb.f_cb = func_zalloc(sizeof(f_clock_t));
    func_cb.frm_main = func_clock_form_create();
}

//退出主页功能
void func_clock_exit(void)
{
    func_cb.last = FUNC_CLOCK;
}

//主页功能
void func_clock(void)
{
    printf("%s\n", __func__);
    func_clock_enter();
    while (func_cb.sta == FUNC_CLOCK) {
        func_clock_process();
        func_clock_message(msg_dequeue());
    }
    func_clock_exit();
}
