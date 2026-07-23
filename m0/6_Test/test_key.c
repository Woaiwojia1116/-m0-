#ifdef KEY_TEST

#include "test_key.h"
#include "key.h"
#include "mock_gpio.h"
#include "serial.h"
#include <string.h>

/* ---------------- 测试辅助函数 ---------------- */

/**
 * @brief  让指定按键保持某电平并扫描若干次
 * @param  idx    按键下标
 * @param  state  电平（0=按下，1=释放）
 * @param  cycles 扫描次数
 * @note   每次扫描前重新设置电平，模拟真实波形
 */
static void key_hold(uint8_t idx, uint8_t state, uint16_t cycles)
{
    KeyEvent_t ev[3];
    uint16_t i;

    for (i = 0; i < cycles; i++)
    {
        mock_key_set_pin(idx, state);
        memset(ev, 0, sizeof(ev));
        key_scan(ev);
    }
}

/**
 * @brief  模拟一次完整短按（按下 → 释放），返回释放后产生的事件
 * @param  idx 按键下标
 * @return 最后一次扫描产生的事件（应为 KEY_EVENT_SHORT_PRESS）
 * @note   按下和释放各持续 10ms（远大于消抖阈值 3），确保稳定触发
 */
static KeyEvent_t simulate_short_press(uint8_t idx)
{
    KeyEvent_t ev[3] = {0};

    /* 阶段 1：确保起始为释放态，扫描几次稳定在 IDLE */
    key_hold(idx, 1, 5);

    /* 阶段 2：按下并保持 10 次扫描（10ms），跨越消抖阈值 */
    key_hold(idx, 0, 10);

    /* 阶段 3：释放并保持 10 次扫描，最后一次应产生 SHORT_PRESS */
    key_hold(idx, 1, 10);

    mock_key_set_pin(idx, 1);
    memset(ev, 0, sizeof(ev));
    key_scan(ev);

    return ev[idx];
}

/**
 * @brief  断言并输出单条用例结果
 * @param  name   用例名称
 * @param  expr   判定表达式（非0为通过）
 * @param  result 结果统计结构体
 * @return 1=通过, 0=失败
 */
static uint8_t test_assert(const char *name, uint8_t expr, TestResult *result)
{
    result->total++;
    if (expr)
    {
        result->passed++;
        serial_printf("[PASS] %s\r\n", name);
        return 1;
    }
    else
    {
        result->failed++;
        serial_printf("[FAIL] %s\r\n", name);
        return 0;
    }
}

/* ---------------- 具体测试用例 ---------------- */

/* 用例 1：按键 0 短按应产生 KEY_EVENT_SHORT_PRESS */
static void test_key0_short_press(TestResult *r)
{
    KeyEvent_t ev = simulate_short_press(0);
    test_assert("key0 short press event", ev == KEY_EVENT_SHORT_PRESS, r);
}

/* 用例 2：按键 1 短按应产生 KEY_EVENT_SHORT_PRESS */
static void test_key1_short_press(TestResult *r)
{
    KeyEvent_t ev = simulate_short_press(1);
    test_assert("key1 short press event", ev == KEY_EVENT_SHORT_PRESS, r);
}

/* 用例 3：按键 2 短按应产生 KEY_EVENT_SHORT_PRESS */
static void test_key2_short_press(TestResult *r)
{
    KeyEvent_t ev = simulate_short_press(2);
    test_assert("key2 short press event", ev == KEY_EVENT_SHORT_PRESS, r);
}

/* 用例 4：仅释放（无按下）不应产生任何事件 */
static void test_no_press_no_event(TestResult *r)
{
    KeyEvent_t ev;
    key_hold(0, 1, 10);     /* 全程释放 */
    ev = simulate_short_press(0);   /* 先制造一次有效按下，消耗后回到 IDLE */
    (void)ev;

    /* 现在再次只释放扫描，确认无事件 */
    {
        KeyEvent_t ev2[3] = {0};
        key_hold(0, 1, 5);
        mock_key_set_pin(0, 1);
        key_scan(ev2);
        test_assert("no press → no event", ev2[0] == KEY_EVENT_NONE, r);
    }
}

/* 用例 5：抖动（按下不足消抖阈值即释放）不应产生事件 */
static void test_bounce_no_event(TestResult *r)
{
    KeyEvent_t ev;
    key_hold(0, 1, 5);      /* 稳定释放 */
    key_hold(0, 0, 1);      /* 仅按下 1 次（< 消抖阈值 3） */
    key_hold(0, 1, 5);      /* 释放 */

    mock_key_set_pin(0, 1);
    {
        KeyEvent_t ev2[3] = {0};
        key_scan(ev2);
        ev = ev2[0];
    }
    test_assert("bounce → no event", ev == KEY_EVENT_NONE, r);
}

/* 用例 6：连续两次短按应产生两次事件 */
static void test_double_short_press(TestResult *r)
{
    KeyEvent_t ev1 = simulate_short_press(0);
    KeyEvent_t ev2 = simulate_short_press(0);
    uint8_t ok = (ev1 == KEY_EVENT_SHORT_PRESS) && (ev2 == KEY_EVENT_SHORT_PRESS);
    test_assert("double short press → 2 events", ok, r);
}

/* 用例 3 个按键独立：按下 key0 不影响 key1/key2 事件 */
static void test_keys_independent(TestResult *r)
{
    KeyEvent_t ev[3];
    key_hold(0, 1, 5);
    key_hold(1, 1, 5);
    key_hold(2, 1, 5);

    /* 仅按下 key1 */
    key_hold(0, 1, 10);
    key_hold(1, 0, 10);
    key_hold(2, 1, 10);

    key_hold(0, 1, 10);
    key_hold(1, 1, 10);
    key_hold(2, 1, 10);

    mock_key_set_pin(0, 1);
    mock_key_set_pin(1, 1);
    mock_key_set_pin(2, 1);
    memset(ev, 0, sizeof(ev));
    key_scan(ev);

    uint8_t ok = (ev[0] == KEY_EVENT_NONE) &&
                 (ev[1] == KEY_EVENT_SHORT_PRESS) &&
                 (ev[2] == KEY_EVENT_NONE);
    test_assert("keys independent (only key1 fires)", ok, r);
}

/* ---------------- 测试入口 ---------------- */

/**
 * @brief  运行全部按键短按测试用例
 */
TestResult test_key_run_all(void)
{
    TestResult result = {0, 0, 0};

    serial_printf("\r\n========== KEY TEST START ==========\r\n");

    mock_key_reset();

    test_key0_short_press(&result);
    test_key1_short_press(&result);
    test_key2_short_press(&result);
    test_no_press_no_event(&result);
    test_bounce_no_event(&result);
    test_double_short_press(&result);
    test_keys_independent(&result);

    serial_printf("========== KEY TEST END ==========\r\n");
    serial_printf("TOTAL: %u  PASS: %u  FAIL: %u\r\n",
                  result.total, result.passed, result.failed);

    return result;
}

#endif /* KEY_TEST */
