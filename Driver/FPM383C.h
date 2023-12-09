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

void FPM383_send_instruction(u8 *buffer, u8 cnt);  //传入指令集字节数做相应的操作

u8 FPM383_handle_instruction(u8 *buffer, u8 cnt); // 判断模块是否有应答

	

void FPM383_enroll(u16 id); // 注册指纹模板

void FPM383_identify(void);//进行指纹识别

u8 FPM383_cancel(void); 		// 取消自动验证、注册流程

void FPM383_get_enrolled(void); // 查询录入的指纹索引表

void FPM383_delete(u16 id); // 删除指纹模板
void FPM383_empty(void);  // 清空指纹库

extern void open_door();



#endif  
