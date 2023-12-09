#ifndef __FPM383C_H__
#define __FPM383C_H__

#include "config.h"
#include "GPIO.h"
#include "Delay.h"
#include <stdio.h>
#include <string.h>
#include "UART.h"
#include "NVIC.h"
#include "Switch.h"
#include "oled.h"

	
void FPM383_init(void);

void FPM383_send_instruction(u8 *buffer, u8 cnt);  //����ָ��ֽ�������Ӧ�Ĳ���

u8 FPM383_handle_instruction(u8 *buffer, u8 cnt); // �ж�ģ���Ƿ���Ӧ��

	

void FPM383_enroll(u16 id); // ע��ָ��ģ��

void FPM383_identify(void);//����ָ��ʶ��

u8 FPM383_cancel(void); 		// ȡ���Զ���֤��ע������

void FPM383_get_enrolled(void); // ��ѯ¼���ָ��������

void FPM383_delete(u16 id); // ɾ��ָ��ģ��
void FPM383_empty(void);  // ���ָ�ƿ�

extern void open_door();



#endif  
