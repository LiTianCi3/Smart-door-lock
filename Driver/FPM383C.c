#include "FPM383C.h"




// 自动校验指令码
#define INST_CODE_AUTO_DIENTIFY 0x32
// 自动注册指令码
#define INST_CODE_AUTO_ENROLL 0x31
// 取消注册和验证指令码
#define INST_CODE_CANCEL 0x30
// 读取录入的指纹索引表
#define INST_CODE_GET_ENROLLED 0x1F
// LED控制指令
#define INST_CODE_LED_CONTROL 0x3C
// 删除指定指纹
#define INST_CODE_DELETE_CHAR 0x0C
// 清空所有指纹
#define INST_CODE_EMPTY 0x0D

// 记录要进行的指令码
u8 instruction_code = 0x00;

// 等待上一个指令响应完成
#define WAIT_FOR_RESPONSE(wait_time) \
    while (instruction_code != 0x00 && wait_time-- > 0) \
    { \
        delay_ms(1); \
    } \

// 模块LED灯控制协议
// 蓝灯常亮
u8 code PS_BlueLEDBuffer[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, INST_CODE_LED_CONTROL,
    0x03, 0x01, 0x01, 0x00, 0x00, 0x49
};
// 红灯闪
u8 code PS_RedLEDBuffer[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, INST_CODE_LED_CONTROL,
    0x02, 0x04, 0x04, 0x02, 0x00, 0x50
};
// 绿灯闪
u8 code PS_GreenLEDBuffer[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, INST_CODE_LED_CONTROL,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x4C
};

// 自动验证指纹模板
u8 code PS_AutoIdentifyBuffer[17] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x08, // 包标识01H，包长度0008H
    INST_CODE_AUTO_DIENTIFY,
    0x01,                  // 分数等级
    0xFF, 0xFF,            // ID号 填写0xFFFF 则1：N全局搜索，否则是1：1单个查找
    0x00, 0x04, 0x02, 0x3E // 参数0x0004，校验和0x023E
};

// 自动注册指纹模板
u8 code PS_AutoEnroll[17] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x08,      // 包标识01H，包长度0008H (index：[6, 8])	 这里开始计算校验和，包含包标识
    INST_CODE_AUTO_ENROLL, // 指令码   index: [9]					0x31
    0x00, 0x05,            // ID号     index: [10, 11]      以0x00, 0x05为例，只修改第2个字节就够用，范围是[0,255]，最多也只是支持50个指纹。
    0x05,                  // 录入次数 index: [12]
    0x00, 0x1B,            // 参数     index: [13, 14] 0001 1011			 这里结束计算校验和，不包含校验和
    0x00, 0x5F             // 校验和   index: [15, 16]
};

// 取消自动验证、注册流程
u8 code PS_Cancel[12] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x03, // 包标识01H，包长度0003H (index：[6, 8])	 这里开始计算校验和，包含包标识
    INST_CODE_CANCEL, // 指令码   index: [9]					0x30
    0x00, 0x34        // 校验和   index: [10, 11]
};

// 读取已注册指纹索引位置
u8 code PS_ReadIndexTable[13] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x04,       // 包标识01H，包长度0004H (index：[6, 8])	 这里开始计算校验和
    INST_CODE_GET_ENROLLED, // 指令码   index: [9]					0x1F
    0x00,                   // 页码 index:      [10]
    0x00, 0x24              // 校验和   index: [11, 12]
};

// 删除指定ID号开始的N个指纹模板
u8 code PS_DeleteChar[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x07, // 包标识01H，包长度0007H (index：[6, 8])	 这里开始计算校验和
    INST_CODE_DELETE_CHAR,
    '\0', '\0',  // ID号     index: [10, 11]
    '\0', '\0',  // 删除个数 index: [12, 13]
    '\0', '\0',  // 校验和   index: [14, 15]
};

// 清空指纹库
u8 code PS_Empty[12] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x03, // 包标识01H，包长度0003H (index：[6, 8])	 这里开始计算校验和
    INST_CODE_EMPTY, // 指令码   index: [9]					0x20
    0x00, 0x11        // 校验和   index: [10, 11]
};


static void GPIO_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure; // 结构定义
    // UART4
    GPIO_InitStructure.Pin = GPIO_Pin_2 | GPIO_Pin_3; // 指定要初始化的IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;            // 指定IO的输入或输出方式,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P5, &GPIO_InitStructure);       // 初始化
}

static void UART_config()
{
    COMx_InitDefine		COMx_InitStructure;					//结构定义
    COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//模式, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
    COMx_InitStructure.UART_BRT_Use   = BRT_Timer4;			//选择波特率发生器, BRT_Timer1, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
    COMx_InitStructure.UART_BaudRate  = 57600ul;			//波特率, 一般 110 ~ 115200
    COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
    COMx_InitStructure.BaudRateDouble = DISABLE;			//波特率加倍, ENABLE或DISABLE
    UART_Configuration(UART4, &COMx_InitStructure);		//初始化串口1 UART1,UART2,UART3,UART4

    NVIC_UART4_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
    UART4_SW(UART4_SW_P52_P53);
}
void FPM383_init(void)
{
    GPIO_config();
    UART_config();
}

/**
 * @brief FPM383指纹识别
 *
 * \param buffer
 * \param cnt
 * \return u8
 *
EF 01 FF FF FF FF 01 00 08 31 00 05 03 00 1A 00 5C //按 3 次，存 05 号
31		指令码
00 05	ID号
03		录入次数
00 1A	可以覆盖 ID 号，不允许重复注册, 每次采集要求手指离开

00 01 02 03 04 05 06 07 08   09 10 11   12 13	// index
                             确 参1参2
EF 01 FF FF FF FF 07 00 05 | 00 00 00 | 00 0C //指令合法性检测  	00 合法
EF 01 FF FF FF FF 07 00 05 | 00 01 01 | 00 0E //第1次采图结果 	  00 成功
EF 01 FF FF FF FF 07 00 05 | 00 02 01 | 00 0F //第1次生成特征结果  00成功
EF 01 FF FF FF FF 07 00 05 | 00 03 01 | 00 10 //第1次手指离开	00成功
EF 01 FF FF FF FF 07 00 05 | 00 01 02 | 00 0F //第2次采图结果
EF 01 FF FF FF FF 07 00 05 | 00 02 02 | 00 10 //第2次手指离开
EF 01 FF FF FF FF 07 00 05 | 00 03 02 | 00 11 //第2次手指离开
EF 01 FF FF FF FF 07 00 05 | 00 01 03 | 00 10 //第3次采图结果
EF 01 FF FF FF FF 07 00 05 | 00 02 03 | 00 11 //第3次手指离开
EF 01 FF FF FF FF 07 00 05 | 00 04 F0 | 01 00 //合并模板					00 成功
EF 01 FF FF FF FF 07 00 05 | 00 05 F1 | 01 02 //检测手指是否已注册
EF 01 FF FF FF FF 07 00 05 | 00 06 F2 | 01 04 //模板存储成功
 */
void printf_buffer(u8 *buffer, u8 cnt)
{
    u8 i;
    for (i = 0; i < cnt; i++)
    {
        printf("0x%02X ", (int)buffer[i]);
    }
    printf("\n");
}
u16 get_checksum(u8 *buffer, u8 cnt)
{
    // 校验和 = 第[7,cnt - 2]个字节的和 & 0xFFFF
    u16 sum = 0;
    u8 i;
    for (i = 6; i < cnt - 2; i++)
    {
        sum += buffer[i];
    }
    sum &= 0xFFFF;
    return sum;
}

// 检查校验和是否正确，正确返回0，错误返回1
u8 check_sum(u8 *buffer, u8 cnt)
{
    u16 actual_check_sum = get_checksum(buffer, cnt);
    // 获取末尾两个字节的值
    u16 except_check_sum = buffer[cnt - 2] << 8 | buffer[cnt - 1];
    if (actual_check_sum != except_check_sum)
    {
        printf("校验和错误! actual 0x%04X != except 0x%04X\n", (int)actual_check_sum, (int)except_check_sum);
        return 1;
    }
    return 0;
}

u8 handle_auto_enroll_result(u8 *buffer, u8 cnt)
{
    u8 confirm, arg1, arg2;
    // EF 01 FF FF FF FF 07 00 08 09 05 FF FF 00 00 02 1B
    if (cnt != 14)
    {
        printf("自动注册返回长度错误! %d\n", (int)cnt);
        // 打印错误的二进制数据
        printf_buffer(buffer, cnt);
        return 1;
    }
    confirm = buffer[9];
    arg1 = buffer[10];
    arg2 = buffer[11];
    printf("自动注册返回: 0x%02X 0x%02X 0x%02X -> ", (int)confirm, (int)arg1, (int)arg2);

    if (arg1 == 0x00 && arg2 == 0x00)
    {   // 指令合法性检测
        printf(confirm == 0x00 ? "指令合法性检测成功!\n" : "指令合法性检测失败!\n");
    }
    else if (arg1 == 0x01)
    {
        printf((confirm == 0x00 ? "第%d次采图成功!\n" : "第%d次采图失败!\n"), (int)arg2);
    }
    else if (arg1 == 0x02)
    {
        printf((confirm == 0x00 ? "第%d次生成特征成功!\n" : "第%d次生成特征失败!\n"), (int)arg2);
    }
    else if (arg1 == 0x03)
    {
        printf((confirm == 0x00 ? "第%d次手指离开!\n" : "第%d次手指未离开!\n"), (int)arg2);
    }
    else if (arg1 == 0x04)
    {
        printf((confirm == 0x00 ? "指纹模板合并成功!\n" : "指纹模板合并失败!\n"));
    }
    else if (arg1 == 0x05) // 注册校验
    {
        if (confirm == 0x27)
        {
            printf(">> 指纹已存在 << ");
        }
        printf((confirm == 0x00 ? "指纹已注册成功!\n" : "指纹注册失败!\n"));
    }
    else if (arg1 == 0x06 && arg2 == 0xF2)
    {
        printf((confirm == 0x00 ? "恭喜，指纹模板存储成功!!!\n" : "抱歉，指纹模板存储失败!\n"));
        return 2;
    }
    else
    {
        printf("未知返回!\n");
    }
    if (confirm != 0x00)
    {
        switch (confirm)
        {
        case 0x0B:
            printf(">> 指定存储ID无效 << ");
            break;
        case 0x25:
            printf(">> 录入次数配置错误 << ");
            break;
        case 0x1F:
            printf(">> 指纹库已满 << ");
            break;
        case 0x22:
            printf(">> 指定ID号已存在指纹 << ");
            break;
        default:
            break;
        }
        printf("自动注册失败! 错误确认码：0x%02X \n", (int)confirm);
    }

    return confirm == 0x00 ? 0 : 1;
}

u8 handle_auto_identify_result(u8 *buffer, u8 cnt)
{
    u16 id, score;
    u8 confirm_code;
    u16 sum = 0;
    // EF 01 FF FF FF FF 07 00 08 09 05 FF FF 00 00 02 1B
    if (cnt != 17)
    {
        printf("自动识别返回长度错误! %d\n", (int)cnt);
        return 1;
    }

    // 处理校验结果，共17个字节，最后两位是校验和
    // 校验和 = 第[7,15]个字节的和 & 0xFFFF
    if (check_sum(buffer, cnt) != 0)
    {
        return 1;
    }
    confirm_code = buffer[9];

    switch (confirm_code)
    {
    case 0x00:
        // 取出 第12,13指纹ID，14,15得分
        id = buffer[11] << 8 | buffer[12];
        score = buffer[13] << 8 | buffer[14];
        printf("指纹验证成功 指纹ID:%d 得分:%d !", id, score);
        break;
    case 0x26:
        printf("指纹验证失败, 反馈超时! -> %d\n", (int)confirm_code);
        return 1;
    case 0x02:
        printf("指纹验证失败, 传感器上未检测到手指! -> %d\n", (int)confirm_code);
        return 1;
    case 0x01:
    case 0x07:
    case 0x09:
    default:
        if (confirm_code == 0x09)
        {
            printf(">> 未搜索到指纹 << ");
        }
        printf("指纹验证失败! -> %02X\n", (int)confirm_code);
        return 1;
    }


    return 0;
}

u8 handle_cancel_result(u8 *buffer, u8 cnt)
{
    u8 confirm_code;
    if (cnt != 12)
    {
        printf("取消指令返回长度错误! %d\n", (int)cnt);
        return 1;
    }

    confirm_code = buffer[9];
    if (confirm_code != 0x00)
    {
        printf("取消指令失败! -> %d\n", (int)confirm_code);
        return 0;
    }

    printf("取消指令成功!\n");
    return 0;
}

u8 handle_get_enrolled_result(u8 *buffer, u8 cnt)
{
    u8 i, j;
    u16 ids_mask;
    u16 sum = 0;
    u8 confirm_code = buffer[9];

    if (check_sum(buffer, cnt) != 0)
    {
        return 1;
    }

    if (confirm_code != 0x00)
    {
        printf("获取已注册指纹数量失败! -> %d\n", (int)confirm_code);
        return 1;
    }

    printf("已注册指纹-> ");
    // 从索引10开始，取出共32bytes
    for (i = 0; i < 32; i++)
    {
        ids_mask = buffer[i + 10];
        // printf("%02X ", (int)ids_mask);
        if (ids_mask == 0x00)
        {
            continue;
        }
        // 打印一个字节中值为1的位置索引 (共32bytes * 8 = 256个位)
        for (j = 0; j < 8; j++)
        {
            if (ids_mask & 0x01)
            {
                printf("%d ", (int)(i * 8 + j));
            }
            ids_mask >>= 1;
        }
    }
    printf("\n");

    return 0;
}

void FPM383_send_instruction(u8 *dat, u8 cnt)
{

    u8 i;
    if (cnt > 9)
    {
        // 记录指令码，用于收到应答后判断做什么操作
        instruction_code = dat[9];
    }

    for (i = 0; i < cnt; i++)
    {
        TX4_write2buff(dat[i]);
    }
}

u8 FPM383_handle_instruction(u8 *buffer, u8 cnt)
{
    u8 result;

    // 判断是否是模块的应答
    if (buffer[0] != 0xEF || buffer[1] != 0x01)
    {
        return 1;
    }

    // if(instruction_code != INST_CODE_LED_CONTROL){ // 除了LED控制指令，其他都打印
    //     // 打印16进制1个字节的指令码instruction_code
    //     printf("当前指令码:0x%02X ", (int)instruction_code);
    // }

    switch (instruction_code)
    {
    case INST_CODE_AUTO_DIENTIFY: // 指纹校验
        // printf("收到【自动识别指令】应答\n");
        instruction_code = 0x00;
        result = handle_auto_identify_result(buffer, cnt);
        if (result == 0)
        {   // success
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//绿灯闪
            open_door();
        }
        else
        {   // failure
            FPM383_send_instruction(PS_RedLEDBuffer, 16);//红灯闪
        }
        break;
    case INST_CODE_AUTO_ENROLL: // 指纹注册
        // printf("收到【自动注册指令】应答\n");
        result = handle_auto_enroll_result(buffer, cnt);
        if (result == 2)
        {   // success
            instruction_code = 0x00;
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//绿灯闪
            OLED_Clear();
            OLED_Display_GB2312_string(20, 2, "注册成功!!");           
            delay_s(3);
            OLED_Clear();
        }
        else if (result == 1)
        {   // failure
            instruction_code = 0x00;
            FPM383_send_instruction(PS_RedLEDBuffer, 16);//红灯闪
        }
        break;
    case INST_CODE_CANCEL: // 取消指纹注册
        // printf("收到【取消指令】应答\n");
        result = handle_cancel_result(buffer, cnt);
        if (result == 0)
        {
            // success
            instruction_code = 0x00;
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//绿灯闪

        }
        break;
    case INST_CODE_GET_ENROLLED: // 获取已录入指纹索引
        // printf("收到【获取已注册指纹】指令应答\n");
        result = handle_get_enrolled_result(buffer, cnt);
        if (result == 0)
        {
            // success
            instruction_code = 0x00;
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//绿灯闪
        }
        else
        {   // failure
            instruction_code = 0x00;
            FPM383_send_instruction(PS_RedLEDBuffer, 16);
        }
        break;
    case INST_CODE_DELETE_CHAR:
        instruction_code = 0x00;
        if (buffer[9] == 0x00) {
            printf("指定指纹删掉成功!\n");
        }
        break;
    case INST_CODE_EMPTY:
        instruction_code = 0x00;
        if (buffer[9] == 0x00) {
            printf("所有指纹清空成功!\n");
        }
        break;
    case INST_CODE_LED_CONTROL: // LED控制
        // printf("收到【LED控制指令】应答\n");
        instruction_code = 0x00;
        break;
    default:
        // printf("收到未知指令应答，未知指令如下：\n");
        // printf_buffer(buffer, cnt);
        instruction_code = 0x00;
        break;
    }
    return 0;
}

// -----------------------------------------------------------------以下是模块指令调用区代码

void FPM383_enroll(u16 id)
{
    u16 wait_time = 1000;
    u16 checksum;
    u8 idata PS_AutoEnroll_Copy[17];
    printf("进行指纹注册，请按下抬起5次手指！\n");
    // 蓝灯常亮
    FPM383_send_instruction(PS_BlueLEDBuffer, 16);

    WAIT_FOR_RESPONSE(wait_time);

    // 将PS_AutoEnroll复制一份
    memcpy(PS_AutoEnroll_Copy, PS_AutoEnroll, 17);

    // 修改[10, 11]为要录入位置的ID号
    PS_AutoEnroll_Copy[10] = (u8)(id >> 8);
    PS_AutoEnroll_Copy[11] = (u8)(id & 0xFF);

    // 修改末尾两位校验和
    checksum = get_checksum(PS_AutoEnroll_Copy, 17);
    PS_AutoEnroll_Copy[15] = (u8)(checksum >> 8);
    PS_AutoEnroll_Copy[16] = (u8)(checksum & 0xFF);

    // printf_buffer(PS_AutoEnroll_Copy, 17);

    // 自动注册指纹PS_AutoEnroll
    FPM383_send_instruction(PS_AutoEnroll_Copy, 17);
}

void FPM383_identify()
{
    u16 wait_time = 1000;
    printf("进行指纹识别！\n");
    // 蓝灯常亮
    FPM383_send_instruction(PS_BlueLEDBuffer, 16);

    WAIT_FOR_RESPONSE(wait_time);

    // 自动验证指纹PS_AutoIdentifyBuffer
    FPM383_send_instruction(PS_AutoIdentifyBuffer, 17);
}

u8 FPM383_cancel(void)
{
    if (instruction_code == INST_CODE_AUTO_ENROLL)  //自动注册指令码
    {
        printf("取消指纹注册流程！\n");
        FPM383_send_instruction(PS_Cancel, 12);// S_Cancel[12]:取消自动验证、注册流程
        return 1;
    }
    else if (instruction_code == INST_CODE_AUTO_DIENTIFY)// 自动校验指令码
    {
        printf("取消指纹识别流程！\n");
        FPM383_send_instruction(PS_Cancel, 12);
        return 1;
    }
    return 0;
}

void FPM383_get_enrolled(void)
{
    u8 pageId = 0x00;
    u16 checksum;
    u8 idata PS_ReadIndexTable_Copy[13];
    // 将PS_ReadIndexTable拷贝到PS_ReadIndexTable_Copy
    memcpy(PS_ReadIndexTable_Copy, PS_ReadIndexTable, 13);
    // 将pageId写入PS_ReadIndexTable_Copy的10位
    PS_ReadIndexTable_Copy[10] = pageId;
    // 将校验和写入到[11, 12]位
    checksum = get_checksum(PS_ReadIndexTable_Copy, 13);
    PS_ReadIndexTable_Copy[11] = checksum >> 8;
    PS_ReadIndexTable_Copy[12] = checksum & 0xFF;
    // 发送指令
    FPM383_send_instruction(PS_ReadIndexTable_Copy, 13);//红灯闪
}
void FPM383_delete(u16 id) {
    u16 checksum;
    // 复制PS_DeleteChar
    u8 idata PS_DeleteChar_Copy[16];
    memcpy(PS_DeleteChar_Copy, PS_DeleteChar, 16);
    // 修改[10, 11]为要删除位置的ID号
    PS_DeleteChar_Copy[10] = (u8)(id >> 8);
    PS_DeleteChar_Copy[11] = (u8)(id & 0xFF);
    // 填入删除个数0x0001
    PS_DeleteChar_Copy[12] = 0x00;
    PS_DeleteChar_Copy[13] = 0x01;
    // 计算校验和
    checksum = get_checksum(PS_DeleteChar_Copy, 16);
    PS_DeleteChar_Copy[14] = checksum >> 8;
    PS_DeleteChar_Copy[15] = checksum & 0xFF;

    // 发送指令
    FPM383_send_instruction(PS_DeleteChar_Copy, 16);//删除指定指纹

}
void FPM383_empty(void) {   // 清空指纹库
    FPM383_send_instruction(PS_Empty, 12);
}
