#include "include.h"
#include "func_user_scene.h"

//场景背景色：浅绿色 (RGB565)
#define SCENE_BG_COLOR          make_color(144, 238, 144)

//创建用户场景窗体：480*480 浅绿色背景
compo_form_t *func_user_scene_form_create(void)
{
    compo_form_t *frm = compo_form_create(true);

    //铺满整个屏幕的纯色背景
    compo_shape_t *bg = compo_shape_create(frm, COMPO_SHAPE_TYPE_RECTANGLE);
    compo_shape_set_color(bg, SCENE_BG_COLOR);
    compo_shape_set_location(bg, GUI_SCREEN_CENTER_X, GUI_SCREEN_CENTER_Y, GUI_SCREEN_WIDTH, GUI_SCREEN_HEIGHT);

    return frm;
}
