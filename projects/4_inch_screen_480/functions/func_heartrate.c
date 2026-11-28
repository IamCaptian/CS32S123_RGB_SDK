#include "include.h"
#include "func.h"
#include "user_ui/func_user_device.h"

//创建设备窗体（原心率页面已被用户设备页面替代）
compo_form_t *func_heartrate_form_create(void)
{
    return func_user_device_form_create();
}

//设备功能事件处理
static void func_heartrate_process(void)
{
    func_process();                                  //刷新UI
}

//设备功能消息处理
static void func_heartrate_message(size_msg_t msg)
{
    func_message(msg);
}

//进入设备功能
void func_heartrate_enter(void)
{
    func_cb.frm_main = func_heartrate_form_create();
    bsp_sensor_hr_init(0);                           //保持原有心率测量行为
}

//退出设备功能
void func_heartrate_exit(void)
{
    bsp_sensor_hr_stop();
}

//设备功能
void func_heartrate(void)
{
    printf("%s\n", __func__);
    func_heartrate_enter();
    while (func_cb.sta == FUNC_HEARTRATE) {
        func_heartrate_process();
        func_heartrate_message(msg_dequeue());
    }
    func_heartrate_exit();
}
