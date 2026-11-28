#include "include.h"
#include "func_user_home.h"

//创建用户主页窗体：使用 UI_BUF_BGIMG_TEX_BIN 作为全屏背景
compo_form_t *func_user_home_form_create(void)
{
    compo_form_t *frm = compo_form_create(true);

    //背景图片（按图片原始尺寸居中显示）
    compo_picturebox_t *bg = compo_picturebox_create(frm, UI_BUF_BGIMG_TEX_BIN);
    compo_picturebox_set_pos(bg, GUI_SCREEN_CENTER_X, GUI_SCREEN_CENTER_Y);

    return frm;
}
