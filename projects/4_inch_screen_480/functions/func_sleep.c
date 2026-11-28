#include "include.h"
#include "func.h"
#include "user_ui/func_user_ac.h"

//创建空调窗体（原睡眠页面已被用户空调页面替代）
compo_form_t *func_sleep_form_create(void)
{
    return func_user_ac_form_create();
}

//空调功能事件处理
static void func_sleep_process(void)
{
    func_process();                                  //刷新UI
}

//空调功能消息处理
static void func_sleep_message(size_msg_t msg)
{
    func_message(msg);
}

//进入空调功能
void func_sleep_enter(void)
{
    func_cb.frm_main = func_sleep_form_create();
}

//退出空调功能
void func_sleep_exit(void)
{
    func_cb.last = FUNC_SLEEP;
}

//空调功能
void func_sleep(void)
{
    printf("%s\n", __func__);
    func_sleep_enter();
    while (func_cb.sta == FUNC_SLEEP) {
        func_sleep_process();
        func_sleep_message(msg_dequeue());
    }
    func_sleep_exit();
}
