#ifndef __MATRIX_KEYS__
#define __MATRIX_KEYS__

#include "config.h"

// ��ʼ��
void MK_init();

// ɨ�谴��
void MK_scan();

// ֻ�����������û�ʹ�õ�ʱ���ٶ���
// ���µĻص�����
void MK_on_keydown(u8 row, u8 col);

// ̧��Ļص�����
void MK_on_keyup(u8 row, u8 col);

#endif