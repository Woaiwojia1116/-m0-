#ifdef KEY_TEST

#include "ti_msp_dl_config.h"
#include "serial.h"
#include "test_key.h"

/**
 * @brief  测试模式入口
 * @note   - 仅编译于 KEY_TEST 模式下，替代 empty.c 的 main()
 *         - 仅初始化系统时钟和串口，不初始化电机/传感器等硬件
 *         - 运行全部按键测试用例，结果通过 UART0 输出
 */
int main(void)
{
    SYSCFG_DL_init();

    serial_printf("\r\n=====================================\r\n");
    serial_printf("  KEY MODULE UNIT TEST (short press)  \r\n");
    serial_printf("=====================================\r\n");

    test_key_run_all();

    serial_printf("\r\nTEST DONE. Halting.\r\n");

    while (1)
    {
        /* 停机等待观察 */
    }
}

#endif /* KEY_TEST */
