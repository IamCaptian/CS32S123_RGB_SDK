#include "include.h"
#include "func_user_ac.h"

//空调页背景色：浅蓝色 (RGB565)
#define AC_BG_COLOR             make_color(173, 216, 230)

//创建用户空调窗体：480*480 浅蓝色背景
compo_form_t *func_user_ac_form_create(void)
{
    compo_form_t *frm = compo_form_create(true);

    //铺满整个屏幕的纯色背景
    compo_shape_t *bg = compo_shape_create(frm, COMPO_SHAPE_TYPE_RECTANGLE);
    compo_shape_set_color(bg, AC_BG_COLOR);
    compo_shape_set_location(bg, GUI_SCREEN_CENTER_X, GUI_SCREEN_CENTER_Y, GUI_SCREEN_WIDTH, GUI_SCREEN_HEIGHT);

    return frm;
}
