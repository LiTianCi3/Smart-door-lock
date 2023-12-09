#ifndef __MATRIX_KEYS__
#define __MATRIX_KEYS__

#include "config.h"

// 初始化
void MK_init();

// 扫描按键
void MK_scan();

// 只是做声明，用户使用的时候，再定义
// 按下的回调函数
void MK_on_keydown(u8 row, u8 col);

// 抬起的回调函数
void MK_on_keyup(u8 row, u8 col);

#endif