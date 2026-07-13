#include "garyscale.h"
#include "allcontrol.h"
#include "ti_msp_dl_config.h"
uint8_t NOLineFlag=1;

#define STEER_GAIN 0.8

// 定义偏差-控制量映射表，用于线性插值
// 偏差位置范围假设为 -4.5 到 4.5，与传感器布局对应
typedef struct {
    float position; // 偏差位置（例如：-4.5 表示在最左边，0表示正中，4.5在最右边）
    float output;   // 对应的PID控制量（可以是P项输出或最终修正量）
} MapEntry_t;


// 映射表（查找表），需要根据实际传感器布局和测试结果细化调整
const MapEntry_t mapTable[] = {
    {-4.5, -4.5},
    {-1.5, -1.5},
    {-1.0, -1.0},
    {-0.5, -0.5},
    { 0.0,  0.0},
    { 0.5,  0.5},
    { 1.0,  1.0},
    { 1.5,  1.5},
    { 4.5,  4.5}
};
const int mapSize = sizeof(mapTable) / sizeof(MapEntry_t);


uint8_t gw_gray_serial_read()
{
	uint8_t ret = 0; /* 把8个bit塞进这个变量里 */
		/* 循环八次 */
		for (uint8_t i = 0; i < 8; ++i) {
			//输出下降沿
			DL_GPIO_clearPins(GrayScale_CLK_PORT,GrayScale_CLK_PIN_28_PIN);
			//延时
			delay_us(4);
			ret |=(!DL_GPIO_readPins(GrayScale_DAT_PORT,GrayScale_DAT_PIN_31_PIN))<<i;
			DL_GPIO_setPins(GrayScale_CLK_PORT,GrayScale_CLK_PIN_28_PIN);
			delay_us(4);
		}		
		ret = ~ret;
	return ret;  
}


/**
 * @brief 线性插值计算函数
 * @param x 输入的偏差位置
 * @return 插值后的控制输出
 */
float LinearInterpolation(float x) {
    if (x <= mapTable[0].position) return mapTable[0].output;
    if (x >= mapTable[mapSize - 1].position) return mapTable[mapSize - 1].output;

    for (int i = 0; i < mapSize - 1; i++) {
        if (x >= mapTable[i].position && x < mapTable[i+1].position) {
            float x0 = mapTable[i].position;
            float y0 = mapTable[i].output;
            float x1 = mapTable[i+1].position;
            float y1 = mapTable[i+1].output;
            // 线性插值公式：y = y0 + (y1 - y0) * (x - x0) / (x1 - x0)
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
        }
    }
    return 0.0; // 正常情况下不会执行到这里
}

// 改造后的循迹函数（带失线主动找回）
void XunJi(uint8_t *data) {
    static float last_valid_deviation = 0.0;  // 保存上一次有效偏差（用于失线时判断方向）
    float deviation = 0.0;                    // 当前计算出的连续偏差值

    // // --- 1. 失线检测（简化：立即响应，无延迟） ---
    // if (data[0]==0 && data[1]==0 && data[2]==0 && data[3]==0 &&
    //     data[4]==0 && data[5]==0 && data[6]==0 && data[7]==0) {
    //     // 全0 → 失线
    //     NOLineFlag = 1;
    // } else {
    //     // 有任何一个传感器亮 → 有线
    //     NOLineFlag = 0;
    // }

    // --- 2. 计算连续偏差值 deviation ---
    float sumPosition = 0.0;
    int activeCount = 0;
    const float sensorPositions[8] = {-4.5, -3.0, -1.5, -0.5, 0.5, 1.5, 3.0, 4.5};

    for (int i = 0; i < 8; i++) {
        if (data[i] == 1) {
            sumPosition += sensorPositions[i];
            activeCount++;
        }
    }

    if (activeCount > 0) {
        // 有线：计算平均位置作为偏差，并保存为有效值
        deviation = sumPosition / activeCount;
        last_valid_deviation = deviation;   // 更新上一次有效偏差
    } else {
        // 失线：根据上一次有效偏差的方向，强制输出最大反向偏差（用于找回赛道）
        if (last_valid_deviation > 0) {
            // 上一次偏差为正（偏右），说明小车在赛道右侧丢失 → 应向左猛转（输出负值）
            deviation = -4.5;
        } else if (last_valid_deviation < 0) {
            // 上一次偏差为负（偏左），说明小车在赛道左侧丢失 → 应向右猛转（输出正值）
            deviation = 4.5;
        }
    }

    // --- 3. 线性插值映射为PID控制量（乘以增益） ---
    xunji_PID.Actual = STEER_GAIN * LinearInterpolation(deviation);

}