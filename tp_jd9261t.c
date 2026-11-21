// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <driver/int.h>
#include <os/mem.h>
#include <driver/tp.h>
#include <driver/tp_types.h>
#include "tp_sensor_devices.h"


#define TAG "jd9261t"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


// external statement.
extern void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);


// macro define
#define JD9261T_WRITE_ADDRESS     (0xD0)  //ok
#define JD9261T_READ_ADDRESS      (0xD1) //ok
#define JD9261T_PRODUCT_ID_CODE   (0x90) //ok

#define JD9261T_ADDR_LEN          (1)
#define JD9261T_REG_LEN           (4)//ok
#define JD9261T_MAX_TOUCH_NUM     (2)//ok - 支持2点触摸
#define JD9261T_POINT_INFO_NUM    (TP_SUPPORT_MAX_NUM)
#define JD9261T_POINT_INFO_SIZE   (5)  // 每个点5字节数据
#define JD9261T_POINT_INFO_TOTAL_SIZE  (JD9261T_POINT_INFO_NUM * JD9261T_POINT_INFO_SIZE)

// JD9261T 寄存器地址定义 (基于参考驱动)
#define JD9261T_REG_CHIP_ID       (0x40008076)  // 芯片ID寄存器
#define JD9261T_REG_SLEEP         (0x8040)      // 睡眠寄存器
#define JD9261T_REG_SENSOR_ID     (0x814A)      // 传感器ID寄存器  
#define JD9261T_REG_CONFIG_DATA   (0x8047)      // 配置数据寄存器
#define JD9261T_REG_VERSION       (0x8140)      // 版本寄存器
#define JD9261T_REG_READ_POINT    (0x8150)      // 读取点寄存器
#define JD9261T_REG_MODULE_SWITCH (0x804D)      // 模块切换寄存器
#define JD9261T_REG_CONFIG_CHKSUM (0x80FF)      // 配置校验和寄存器
#define JD9261T_REG_CONFIG_FRESH  (0x8100)      // 配置刷新寄存器
#define JD9261T_REG_COORD_ADDR    (0x20011120)  // 坐标读取地址 (4字节)

// 触摸状态寄存器
#define JD9261T_STATUS            (0x00)
#define JD9261T_GESTUREID         (0X01)
#define JD9261T_FINGERNUM         (0X02)
#define JD9261T_XPOSH             (0X03)
#define JD9261T_XPOSL             (0X04)
#define JD9261T_YPOSH             (0X05)
#define JD9261T_YPOSL             (0X06)

// 控制寄存器
#define JD9261T_MOTIONMASK        (0xEC)
#define JD9261T_PUSHTIMER         (0xEE)
#define JD9261T_AUTOSLEEEPTIME    (0xF9)
#define JD9261T_IRQCRTL           (0xFA)
#define JD9261T_AUTOSRESET        (0xFB)
#define JD9261T_LONGPRESSTIME     (0xFC)
#define JD9261T_DISAUTOSLEEP      (0xFE)

// 配置参数
#define JD9261T_CONFIG_MAX_LENGTH (186)  // 配置数据最大长度
#define JD9261T_X2Y_ENABLE        (0x08) // X/Y交换使能位
#define JD9261T_RESOLUTION_LOC    (3)    // 分辨率位置
#define JD9261T_TRIGGER_LOC       (8)    // 触发位置

// 默认分辨率 (可根据实际屏幕修改)
#define JD9261T_DEFAULT_MAX_X     (1024)
#define JD9261T_DEFAULT_MAX_Y     (600)
#define JD9261T_INT_TRIGGER       (1)    // 0:上升沿 1:下降沿

#define JD9261T_REGS_DEBUG_EN (0)

#define SENSOR_I2C_READ(reg, buff, len)  cb->read_uint8((JD9261T_WRITE_ADDRESS >> 1), reg, buff, len)
#define SENSOR_I2C_WRITE(reg, buff, len)  cb->write_uint8((JD9261T_WRITE_ADDRESS >> 1), reg, buff, len)
#define SENSOR_I2C_READ32(reg, buff, len)  cb->read_uint32((JD9261T_WRITE_ADDRESS >> 1), reg, buff, len)
#define SENSOR_I2C_WRITE_READ(cmd, cmd_len, buff, len) cb->write_read((JD9261T_WRITE_ADDRESS >> 1), cmd, cmd_len, buff, len)


bool jd9261t_detect(const tp_i2c_callback_t *cb)
{
    if (NULL == cb)
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return false;
    }

    uint8_t product_id[2];

    if (BK_OK != SENSOR_I2C_READ32(JD9261T_REG_CHIP_ID, (uint8_t *)(&product_id), sizeof(product_id)))
    {
        LOGE("%s, read product id reg fail!\r\n", __func__);
        return false;
    }

    LOGI("%s, product id: 0X%02X\r\n", __func__, product_id);

   os_printf("product_id = %X%X%X%X\n", product_id[0], product_id[1], product_id[2], product_id[3]);
   
   if (JD9261T_PRODUCT_ID_CODE ==  product_id[1])
    {
        os_printf("jd9261t_detect success\n");
        LOGI("%s success\n", __func__);
        return true;
    }

    return false;
}

// 校验和计算函数 (8位反码和)
static uint8_t jd9261t_calc_checksum(uint8_t *data, uint32_t length)
{
    uint32_t sum = 0;
    uint8_t checksum = 0;
    uint32_t i;
    
    for (i = 0; i < length; i++)
    {
        sum += data[i];
    }
    checksum = (uint8_t)(sum & 0xFF);
    checksum = (~checksum) + 1;  // 取反加1
    
    return checksum;
}

// JD9261T 上电时序 (参考驱动实现)
static int jd9261t_power_on_sequence(const tp_i2c_callback_t *cb, tp_sensor_user_config_t *config)
{
    if ((NULL == cb) || (NULL == config))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    return BK_OK;
}

// JD9261T 初始化编程 (配置寄存器)
static int jd9261t_init_program(const tp_i2c_callback_t *cb, tp_sensor_user_config_t *config)
{
    if ((NULL == cb) || (NULL == config))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }
    
    uint8_t ret;
    uint8_t write_cfg_config[JD9261T_CONFIG_MAX_LENGTH];
    uint8_t rd_cfg_buf[2];
    
    LOGI("%s: Start init program\r\n", __func__);
    
    // 读取配置数据 (从寄存器 0x8047 开始)
    rd_cfg_buf[0] = (JD9261T_REG_CONFIG_DATA >> 8) & 0xFF;   // 0x80
    rd_cfg_buf[1] = JD9261T_REG_CONFIG_DATA & 0xFF;          // 0x47
    
    os_memset(write_cfg_config, 0, JD9261T_CONFIG_MAX_LENGTH);
    ret = SENSOR_I2C_WRITE_READ(rd_cfg_buf, 2, write_cfg_config, JD9261T_CONFIG_MAX_LENGTH);
    
    if (ret != BK_OK)
    {
        LOGE("%s: Read config data fail\r\n", __func__);
        return BK_FAIL;
    }
    
#if JD9261T_REGS_DEBUG_EN
    LOGI("Current configuration:\r\n");
    for (uint32_t i = 0; i < JD9261T_CONFIG_MAX_LENGTH; i++)
    {
        LOGI("0x%04X: 0x%02X\r\n", JD9261T_REG_CONFIG_DATA + i, write_cfg_config[i]);
    }
#endif
    
    // 解析 X/Y 分辨率
    uint16_t max_x = (write_cfg_config[2] << 8) | write_cfg_config[1];
    uint16_t max_y = (write_cfg_config[4] << 8) | write_cfg_config[3];
    
    LOGI("CFG_RawX=%d, CFG_RawY=%d\r\n", max_x, max_y);
    
    // 可选: 强制设置分辨率 (取消注释以启用)
    #if 0
    {
        uint8_t wCmd[4];
        uint8_t wxbuf[2] = {0x20, 0x03};  // X分辨率 0x0320 = 800
        uint8_t wybuf[2] = {0xE0, 0x01};  // Y分辨率 0x01E0 = 480
        uint8_t checksum;
        
        // 设置 X 分辨率 (寄存器 0x8048-0x8049)
        wCmd[0] = 0x80;
        wCmd[1] = 0x48;
        wCmd[2] = 0x20;
        wCmd[3] = 0x03;
        SENSOR_I2C_WRITE(wCmd[1], &wCmd[2], 2);
        
        // 设置 Y 分辨率 (寄存器 0x804A-0x804B)
        wCmd[0] = 0x80;
        wCmd[1] = 0x4A;
        wCmd[2] = 0xE0;
        wCmd[3] = 0x01;
        SENSOR_I2C_WRITE(wCmd[1], &wCmd[2], 2);
        
        // 设置模块切换 (寄存器 0x804D)
        wCmd[0] = 0x80;
        wCmd[1] = 0x4D;
        wCmd[2] = 0x8C;
        SENSOR_I2C_WRITE(wCmd[1], &wCmd[2], 1);
        
        // 更新配置数组中的值
        write_cfg_config[1] = 0x20;
        write_cfg_config[2] = 0x03;
        write_cfg_config[3] = 0xE0;
        write_cfg_config[4] = 0x01;
        write_cfg_config[6] = 0x8C;
        
        // 计算校验和 (0x8047~0x80FE)
        checksum = jd9261t_calc_checksum(write_cfg_config, JD9261T_CONFIG_MAX_LENGTH - 2);
        
        // 写入校验和 (寄存器 0x80FF)
        wCmd[0] = 0x80;
        wCmd[1] = 0xFF;
        wCmd[2] = checksum;
        SENSOR_I2C_WRITE(wCmd[1], &wCmd[2], 1);
        
        // 写入配置刷新标志 (寄存器 0x8100)
        wCmd[0] = 0x81;
        wCmd[1] = 0x00;
        wCmd[2] = 0x01;
        SENSOR_I2C_WRITE(wCmd[1], &wCmd[2], 1);
        
        LOGI("Write checksum=%02x\r\n", checksum);
    }
    #endif
    
    // 再次读取配置验证
    os_memset(write_cfg_config, 0, JD9261T_CONFIG_MAX_LENGTH);
    ret = SENSOR_I2C_WRITE_READ(rd_cfg_buf, 2, write_cfg_config, JD9261T_CONFIG_MAX_LENGTH);
    
    if (ret != BK_OK)
    {
        LOGE("%s: Re-read config data fail\r\n", __func__);
        return BK_FAIL;
    }
    
    // 检测 INT 触发方式 (寄存器 0x804D bit[1:0])
    uint8_t trigger_mode = write_cfg_config[6] & 0x03;
    if (trigger_mode == 0x00 || trigger_mode == 0x02)
    {
        LOGI("INT Active State: HIGH\r\n");
        config->int_type = TP_INT_TYPE_HIGH_LEVEL;
    }
    else
    {
        LOGI("INT Active State: LOW\r\n");
        config->int_type = TP_INT_TYPE_LOW_LEVEL;
    }
    
    // 检测 X/Y 是否交换 (寄存器 0x804D bit3)
    if (write_cfg_config[6] & JD9261T_X2Y_ENABLE)
    {
        LOGI("X/Y coordinate swapped\r\n");
    }
    
    LOGI("%s: Init program completed\r\n", __func__);
    
    return BK_OK;
}

int jd9261t_init(const tp_i2c_callback_t *cb, tp_sensor_user_config_t *config)
{
    if ((NULL == cb) || (NULL == config))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    LOGI("%s: Start JD9261T initialization\r\n", __func__);
    
    // 步骤1: 执行上电时序
    int ret = jd9261t_power_on_sequence(cb, config);
    if (ret != BK_OK)
    {
        LOGE("%s: Power-on sequence fail!\r\n", __func__);
        return BK_FAIL;
    }
    
    // 步骤2: 执行初始化编程 (配置寄存器)
    ret = jd9261t_init_program(cb, config);
    if (ret != BK_OK)
    {
        LOGE("%s: Init program fail!\r\n", __func__);
        return BK_FAIL;
    }
    
    // 步骤3: 配置基本参数 (原有的初始化代码)
    uint8_t mode = config->refresh_rate / 10;
    if (mode == 0)
    {
        mode = 3;  // 默认 30Hz
    }
    SENSOR_I2C_WRITE(JD9261T_PUSHTIMER, &mode, 1);

    mode = 0x60;  // 中断控制配置
    SENSOR_I2C_WRITE(JD9261T_IRQCRTL, &mode, 1);

    mode = 0x01;  // 禁用自动休眠
    SENSOR_I2C_WRITE(JD9261T_DISAUTOSLEEP, &mode, 1);

    LOGI("%s: JD9261T initialization completed\r\n", __func__);

    return BK_OK;
}

int jd9261t_read_status(const tp_i2c_callback_t *cb, uint8_t *status)
{
    if ((NULL == cb) || (NULL == status))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    if (BK_OK != SENSOR_I2C_READ(JD9261T_STATUS, (uint8_t *)(status), 1))
    {
        LOGE("%s, read status reg fail!\r\n", __func__);
        return BK_FAIL;
    }

    LOGI("%s, status=0x%02X\r\n", __func__, *status);

    return BK_OK;
}

// jd9261t get tp info - 支持多点触摸解析
void jd9261t_read_point(uint8_t *input_buff, void *buf, uint8_t num)
{
	if ((NULL == input_buff) || (NULL == buf) || (0 == num))
	{
		return;
	}

	tp_data_t *read_data = (tp_data_t *)buf;

	static uint8_t pre_touch = 0;
	static uint16_t pre_x[JD9261T_MAX_TOUCH_NUM] = {0};
	static uint16_t pre_y[JD9261T_MAX_TOUCH_NUM] = {0};

	// 解析触摸点数量
	uint8_t finger_num = input_buff[0] & 0x0F;
	if (finger_num > JD9261T_MAX_TOUCH_NUM)
	{
		finger_num = JD9261T_MAX_TOUCH_NUM;
	}

	uint8_t touch_num = finger_num;
	
	// 解析每个触摸点的数据 (每个点5字节)
	for (uint8_t i = 0; i < finger_num; i++)
	{
		uint8_t *pCoord = input_buff + 3 + (i * 5);  // 从偏移3开始，每点5字节
		
		uint16_t input_x = ((uint16_t)pCoord[0] << 8) | pCoord[1];
		uint16_t input_y = ((uint16_t)pCoord[2] << 8) | pCoord[3];
		
		// 检查坐标是否有效
		if ((input_x == 0 && input_y == 0) || 
		    (input_x == 0xFFFF && input_y == 0xFFFF))
		{
			// 无效坐标，报告抬起事件
			if (pre_touch > 0)
			{
				read_data[i].event = TP_EVENT_TYPE_UP;
				read_data[i].timestamp = rtos_get_time();
				read_data[i].width = 0;
				read_data[i].x_coordinate = pre_x[i];
				read_data[i].y_coordinate = pre_y[i];
				read_data[i].track_id = i;
			}
			else
			{
				read_data[i].event = TP_EVENT_TYPE_UP;
				read_data[i].timestamp = rtos_get_time();
				read_data[i].width = 0;
				read_data[i].x_coordinate = 0;
				read_data[i].y_coordinate = 0;
				read_data[i].track_id = i;
			}
		}
		else
		{
			// 有效坐标，报告按下或移动事件
			if (pre_touch > 0)
			{
				read_data[i].event = TP_EVENT_TYPE_MOVE;
			}
			else
			{
				read_data[i].event = TP_EVENT_TYPE_DOWN;
			}
			
			read_data[i].timestamp = rtos_get_time();
			read_data[i].width = 0;
			read_data[i].x_coordinate = input_x;
			read_data[i].y_coordinate = input_y;
			read_data[i].track_id = i;
			
			pre_x[i] = input_x;
			pre_y[i] = input_y;
		}
	}
	
	// 处理所有点都抬起的情况
	if (touch_num == 0 && pre_touch > 0)
	{
		for (uint8_t i = 0; i < JD9261T_MAX_TOUCH_NUM; i++)
		{
			read_data[i].event = TP_EVENT_TYPE_UP;
			read_data[i].timestamp = rtos_get_time();
			read_data[i].width = 0;
			read_data[i].x_coordinate = pre_x[i];
			read_data[i].y_coordinate = pre_y[i];
			read_data[i].track_id = i;
		}
	}

	pre_touch = touch_num;
}

#define JD9261T_REPORT_LEN (40)
static uint8_t read_buff[JD9261T_REPORT_LEN];
int jd9261t_read_tp_info(const tp_i2c_callback_t *cb, uint8_t max_num, uint8_t *buff)
{
    if ((NULL == cb) || (NULL == buff))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    if ((0 == max_num) || (max_num > JD9261T_POINT_INFO_NUM))
    {
        LOGE("%s, max_num %d is out range!\r\n", __func__, max_num);
        return BK_FAIL;
    }

    int ret = BK_OK;
    uint8_t gesture_status = 0;
    uint8_t finger_status = 0;
    uint8_t temp_status = 0;
    uint8_t cmd[6] = {0xF3, 
                      (JD9261T_REG_COORD_ADDR >> 24) & 0xFF,  // 0x20
                      (JD9261T_REG_COORD_ADDR >> 16) & 0xFF,  // 0x01
                      (JD9261T_REG_COORD_ADDR >> 8) & 0xFF,   // 0x11
                      JD9261T_REG_COORD_ADDR & 0xFF,          // 0x20
                      0x03};  // 读取长度

    os_memset(read_buff, 0x00, sizeof(read_buff));
    if (BK_OK != SENSOR_I2C_WRITE_READ(cmd, sizeof(cmd), (uint8_t *)read_buff, sizeof(read_buff)))
    {
        LOGE("%s, read tp info fail!\r\n", __func__);
        ret = BK_FAIL;
        goto exit_;
    }

    gesture_status = read_buff[1];
    finger_status = read_buff[2];
    temp_status = read_buff[0];

    // original registers datas.
    LOGD("%s, gesture_status=0x%02X, finger_status=0x%02X, temp_status=0x%02X\r\n", __func__, gesture_status, finger_status, temp_status);

#if (JD9261T_REGS_DEBUG_EN > 0)
    bk_mem_dump_ex("jd9261t", (unsigned char *)(read_buff), sizeof(read_buff));
#endif

    jd9261t_read_point(read_buff, buff, 1);

exit_:

    return ret;
}

const tp_sensor_config_t tp_sensor_jd9261t =
{
	.name = "jd9261t",
	// default config
	.def_ppi = PPI_400X400,
	.def_int_type = TP_INT_TYPE_FALLING_EDGE,
	.def_refresh_rate = 30,  // 默认 30Hz
	.def_tp_num = 2,  // 支持2点触摸
	// capability config
	.id = TP_ID_JD9261T,
	.address = (JD9261T_WRITE_ADDRESS >> 1),
	.detect = jd9261t_detect,
	.init = jd9261t_init,
	.read_tp_info = jd9261t_read_tp_info,
};
