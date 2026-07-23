#ifndef TEST_KEY_H
#define TEST_KEY_H

#ifdef KEY_TEST

#include <stdint.h>

/* 测试结果统计 */
typedef struct {
    uint16_t passed;    /* 通过用例数 */
    uint16_t failed;    /* 失败用例数 */
    uint16_t total;     /* 总用例数 */
} TestResult;

/**
 * @brief  运行全部按键短按测试用例
 * @return 测试结果统计
 * @note   通过 serial_printf 输出每条用例的 PASS/FAIL 详情
 */
TestResult test_key_run_all(void);

#endif /* KEY_TEST */

#endif /* TEST_KEY_H */
