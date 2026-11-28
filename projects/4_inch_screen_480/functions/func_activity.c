#include "include.h"
#include "func.h"
#include "user_ui/func_user_scene.h"

//创建场景窗体（原活动记录页面已被用户场景页面替代）
compo_form_t *func_activity_form_create(void)
{
    return func_user_scene_form_create();
}

//场景功能事件处理
static void func_activity_process(void)
{
    func_process();                                  //刷新UI
}

//场景功能消息处理
static void func_activity_message(size_msg_t msg)
{
    func_message(msg);
}

//进入场景功能
void func_activity_enter(void)
{
    func_cb.frm_main = func_activity_form_create();
}

//退出场景功能
void func_activity_exit(void)
{
    func_cb.last = FUNC_ACTIVITY;
}

//场景功能
void func_activity(void)
{
    printf("%s\n", __func__);
    func_activity_enter();
    while (func_cb.sta == FUNC_ACTIVITY) {
        func_activity_process();
        func_activity_message(msg_dequeue());
    }
    func_activity_exit();
}
