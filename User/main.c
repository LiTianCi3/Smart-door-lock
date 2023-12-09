#include "config.h"
#include "oled.h"
#include "bmp.h"
#include "GPIO.h"
#include "UART.h"
#include "NVIC.h"
#include "Switch.h"
#include "MatrixKeys.h"
#include "RTX51TNY.H"
#include <string.h>
#include "STC8H_PWM.h"
#include "FPM383C.h"
#include "Delay.h"
#include "EEPROM.h"
#include "Ultrasonic.h"

#define MAX_PASSWORD_LENGTH 6
int user_password[MAX_PASSWORD_LENGTH] = {0}; // 定义一个储存用户输入密码的数组
char show_str[7] = {0};                       // spi屏幕上的隐藏密码
u8 password_length = 0;
u8 error = 1;
int temp_password[MAX_PASSWORD_LENGTH] = {0}; // 定义一个用户修改密码的暂存数组
u8 open = 0;
u8 lock = 1; // 标志是否开锁成功
//==========================eeprom
u16 addr = 0x0000;
int buf[6] = {0};

int password[MAX_PASSWORD_LENGTH] = {2, 3, 4, 5, 6, 7}; // 定义一个初始密码
//==========================eeprom
u8 action_state = 0;
u16 id = 0x0000;

// 定义一个4*4的矩阵键盘数组
char keypad[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

void GPIO_config()
{ // 准双向口	IPS屏幕
    P1_MODE_IO_PU(GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_6);
    P4_MODE_IO_PU(GPIO_Pin_7);
    P5_MODE_IO_PU(GPIO_Pin_0);

    // 准双向口	30 31 串口
    P3_MODE_IO_PU(GPIO_Pin_0 | GPIO_Pin_1);

    // 推挽输出	舵机 P21
    P2_MODE_OUT_PP(GPIO_Pin_1);

    // 准双向口	uart2: P10 P11
    P1_MODE_IO_PU(GPIO_Pin_0 | GPIO_Pin_1);

    // 推挽输出	蜂鸣器 P00
    P0_MODE_OUT_PP(GPIO_Pin_0);
}

void UART_config(void)
{
    // >>> 记得添加 NVIC.c, UART.c, UART_Isr.c <<<
    COMx_InitDefine COMx_InitStructure;             // 结构定义
    COMx_InitStructure.UART_Mode = UART_8bit_BRTx;  // 模式, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
    COMx_InitStructure.UART_BRT_Use = BRT_Timer1;   // 选择波特率发生器, BRT_Timer1, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
    COMx_InitStructure.UART_BaudRate = 115200ul;    // 波特率, 一般 110 ~ 115200
    COMx_InitStructure.UART_RxEnable = ENABLE;      // 接收允许,   ENABLE或DISABLE
    COMx_InitStructure.BaudRateDouble = DISABLE;    // 波特率加倍, ENABLE或DISABLE
    UART_Configuration(UART1, &COMx_InitStructure); // 初始化串口1 UART1,UART2,UART3,UART4

    NVIC_UART1_Init(ENABLE, Priority_1); // 中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
    UART1_SW(UART1_SW_P30_P31);          // 引脚选择, UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44

    // uart2
    COMx_InitStructure.UART_Mode = UART_8bit_BRTx;  // 模式, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
    COMx_InitStructure.UART_BRT_Use = BRT_Timer2;   // 选择波特率发生器, BRT_Timer1, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
    COMx_InitStructure.UART_BaudRate = 9600ul;      // 波特率, 一般 110 ~ 115200
    COMx_InitStructure.UART_RxEnable = ENABLE;      // 接收允许,   ENABLE或DISABLE
    COMx_InitStructure.BaudRateDouble = DISABLE;    // 波特率加倍, ENABLE或DISABLE
    UART_Configuration(UART2, &COMx_InitStructure); // 初始化串口1 UART1,UART2,UART3,UART4

    NVIC_UART2_Init(ENABLE, Priority_1); // 中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
    UART2_SW(UART2_SW_P10_P11);          // 引脚选择
}

#define HZ_VALUE 50 // 频率
#define PRE 10      // 分频系数

u16 duty_value = 500;
#define PERIOD (MAIN_Fosc / (HZ_VALUE * PRE)) // (HZ_VALUE * PRE) 要加(), 有优先级问题
void PWM_config()
{
    PWMx_InitDefine PWMx_InitStructure;

    // 配置PWM6
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO6P;      // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM6, &PWMx_InitStructure);  // 初始化PWM,  PWMA,PWMB

    // 配置PWMB
    PWMx_InitStructure.PWM_Period = PERIOD - 1;    // 周期时间,   0~65535
    PWMx_InitStructure.PWM_DeadTime = 0;           // 死区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 主输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;    // 使能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMB, &PWMx_InitStructure);  // 初始化PWM通用寄存器,  PWMA,PWMB

    // 分频 头文件515有声明
    PWMB_Prescaler(PRE - 1);

    // 切换PWM通道
    PWM6_SW(PWM6_SW_P21); // PWM6_SW_P21,PWM6_SW_P54,PWM6_SW_P01,PWM6_SW_P75

    // 初始化PWMB的中断
    NVIC_PWM_Init(PWMB, DISABLE, Priority_0);
}

void buzzer_init()
{
    u8 i;
    for (i = 0; i < 200; i++)
    {
        P00 = 1;
        delay_ms(1);
        P00 = 0;
        delay_ms(1);
    }
}

void show_str_init()
{
    u8 i;
    for (i = 0; i < 6; i++)
    { // 重置显示的密码
        show_str[i] = '\0';
    }
}

// 定义一个函数驱动舵机开门
void open_door()
{
    PWMx_Duty dutyB;
    // 开门
    float angle;
    error = 1;
    lock = 0;
    duty_value = 2000;
    dutyB.PWM6_Duty = duty_value * PERIOD / 20000.0;
    angle = (duty_value - 500) * 180.0 / 2000;
    printf("duty_value = %d, angle = %.2f °\n", (int)duty_value, angle);
    UpdatePwm(PWM6, &dutyB);
    buzzer_init();
    OLED_Clear();
    OLED_Display_GB2312_string(20, 2, "欢迎回家!!");
    delay_s(3);

    // 关门
    duty_value = 500;
    dutyB.PWM6_Duty = duty_value * PERIOD / 20000.0;
    angle = (duty_value - 500) * 180.0 / 2000;
    printf("duty_value = %d, angle = %.2f °\n", (int)duty_value, angle);
    UpdatePwm(PWM6, &dutyB);
    buzzer_init();
    IPS_Show();
}

// 比较密码
char check_password()
{
    u8 i;
    EEPROM_read_n(addr, buf, 12); // 读
    printf("密码");
    for (i = 0; i < 6; i++)
    { // 显示读取的密码
        printf(" %d,", buf[i]);
    }
    if (password_length != 6) // 检验密码长度是否为6
    {
        printf("密码长度错误\n");
        error++;
        password_length = 0;
        IPS_Init();
        OLED_Display_GB2312_string(0, 2, "密码错误");
        delay_s(1);
        IPS_Show();
        show_str_init();
        return 0;
    }
    if (open == 0)
    {
        for (i = 0; i < MAX_PASSWORD_LENGTH; i++)
        {
            if (buf[i] != user_password[i])
            {
                error++;
                printf("密码错误\n");
                password_length = 0;
                IPS_Init();
                OLED_Display_GB2312_string(0, 2, "密码错误");
                delay_s(1);
                IPS_Show();
                show_str_init();
                return 0;
            }
        }
    }
    else if (open == 3)
    {
        for (i = 0; i < MAX_PASSWORD_LENGTH; i++)
        {
            if (temp_password[i] != user_password[i])
            {
                printf("输入密码不一致\n");
                password_length = 0;
                IPS_Init();
                OLED_Display_GB2312_string(0, 2, "输入密码不一致");
                OLED_Display_GB2312_string(0, 4, "请再次输入");
                open = 2;
                delay_s(1);
                OLED_Clear();
                OLED_Display_GB2312_string(0, 0, "请输入新密码:");
                show_str_init();
                return 0;
            }
        }
    }
    show_str_init();
    return 1;
}

// 定义一个函数删除用户输入的字符  *
void delete_user_input_password()
{
    user_password[password_length] = NULL;
    show_str[password_length - 1] = '\0';
    password_length--;

    // 如果密码长度小于0,则让密码长度等于0
    if (password_length <= 0 || password_length > 6)
    {
        password_length = 0;
        IPS_Show();
    }
    if (open == 0 && password_length > 0)
    {
        IPS_Init();
    }
    else if (open == 1)
    {
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "请输入原密码:");
    }
    else if (open == 3 || open == 2)
    {
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "请输入新密码:");
    }
    OLED_Display_GB2312_string(0, 2, show_str);
}

// 将用户输入的字符拼接成字符串
void user_input_password(char key)
{
    u8 i;
    int num = key - '0';
    if (password_length == 0 && num == 0)
    {
        printf("Error: First digit cannot be 0!\n");
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "首位不能为0");
        delay_s(1);
        IPS_Show();
    }
    else
    {
        if (password_length >= 6)
        {
            printf("Error: Password length cannot be greater than 6!\n");
            OLED_Display_GB2312_string(0, 2, "          ");
            OLED_Display_GB2312_string(0, 2, "密码错误");
            delay_s(1);
            IPS_Show();
            password_length = 0;
            for (i = 0; i < 6; i++)
            { // 重置显示的密码
                show_str[i] = '\0';
            }
        }
        else
        {
            user_password[password_length] = num;
            show_str[password_length] = '*';
            printf("user_password[%d] = %d\n", (int)password_length, user_password[password_length]);
            password_length++;
            if (open == 0)
            {
                IPS_Init();
            }
            else if (open == 1)
            {
                OLED_Clear();
                OLED_Display_GB2312_string(0, 0, "请输入原密码:");
            }
            else if (open == 3 || open == 2)
            {
                OLED_Clear();
                OLED_Display_GB2312_string(0, 0, "请输入新密码:");
            }

            OLED_Display_GB2312_string(0, 2, show_str);
        }
    }
}

void handleHashKey()
{
    u8 i;
    // 如果密码输错3次，则锁定
    printf("error = %d\n", (int)error);
    if (error > 3)
    {   lock=1;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "密码多次输错");
        OLED_Display_GB2312_string(0, 2, "请联系管理员");
        OLED_Display_GB2312_string(0, 4, "重置!!");
        for (i = 0; i < 10; i++)
        {
            buzzer_init();
            delay_ms(250);
            delay_ms(250);
        }
        while (lock);      
    }

    if (open == 0 || open == 1)
    {
        if (check_password())
        {
            password_length = 0;
            printf("密码正确，开门\n");
            OLED_Clear();
            OLED_Display_GB2312_string(0, 2, "   ok!");
            delay_s(1);
            if (open == 0)
            { // 开门
                open_door();
            }
            else if (open == 1)
            { // 修改密码
                OLED_Clear();
                OLED_Display_GB2312_string(0, 0, "请输入新密码:");
                for (i = 0; i < 6; i++)
                { // 重置显示的密码
                    show_str[i] = '\0';
                }
                open = 2;
            }
        }
    }
    else if (open == 2)
    { // 修改密码
        for (i = 0; i < 6; i++)
        {
            temp_password[i] = user_password[i]; // 保存用户输入的密码
            printf("t=%d\n", temp_password[i]);
        }
        password_length = 0;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "请再次输入");
        for (i = 0; i < 6; i++)
        { // 重置显示的密码
            show_str[i] = '\0';
        }
        open = 3;
        printf("open = %d", (int)open);
    }
    else if (open == 3)
    { // 修改密码
        if (check_password())
        {
            EEPROM_SectorErase(addr);                // 擦除
            EEPROM_write_n(addr, temp_password, 12); // 写
            password_length = 0;
            printf("密码正确，修改密码\n");
            OLED_Clear();
            OLED_Display_GB2312_string(2, 0, "修改成功!");
            delay_s(1);
            IPS_Show();
            for (i = 0; i < 6; i++)
            { // 重置显示的密码
                show_str[i] = '\0';
            }
            open = 0;
        }
    }
}

void key_value(char key)
{
    u8 i;
    if (key >= '0' && key <= '9')
    {
        user_input_password(key);
    }
    else if (key == '#')
    {
        handleHashKey();
    }
    else if (key == '*')
    {
        delete_user_input_password();
    }
    else if (key == 'A')
    {
        if (open == 1)
        {
            open = 0;
            IPS_Show();
            return;
        }
        password_length = 0;
        show_str_init();
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "请输入原密码:");
        open = 1;
    }
    else if (key == 'B')
    {
        OLED_Clear();
        OLED_Display_GB2312_string(0, 2, "请按下指纹");
        if (FPM383_cancel())
        {
            IPS_Show();
            return;
        }
        FPM383_identify(); // 指纹识别
    }
}

// 按下的回调函数
void MK_on_keydown(u8 row, u8 col)
{
    int ROW, COL;
    char key;
    ROW = (int)row;
    COL = (int)col;
    key = keypad[ROW][COL];
    printf("key=%c\n", key);
    key_value(key);    // 定义一个函数将按键按下后的信息传进去
    // printf("第%d行第%d列按键按下\n", ROW + 1, COL + 1);
}
// 抬起的回调函数
void MK_on_keyup(u8 row, u8 col)
{
    // printf("第%d行第%d列按键抬起\n", (int)(row + 1), (int)(col + 1));
}

//定义一个初始化密码的函数
void password_init()
{   int init_password[6] = {2, 3, 4, 5, 6, 7}; // 定义一个初始密码
    EEPROM_SectorErase(addr); // 擦除
    EEPROM_write_n(addr, init_password, 12); // 写
    OLED_Clear();
    OLED_Display_GB2312_string(0, 0, "初始化密码成");
    OLED_Display_GB2312_string(0, 2, "功!!!");
    delay_s(2);
    IPS_Show();
}

void do_work(u8 dat)
{
    if (dat == 0x44)// 取消
    { 
        action_state = 0;
        IPS_Show();
        if (FPM383_cancel())
        {
            return;
        }
    }
    else if (dat == 0x01)
    { // 注册指纹
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "注册指纹");
        OLED_Display_GB2312_string(0, 2, "连续按下5次指纹");
        if (FPM383_cancel())
        {
            return;
        }
        id += 0x0001;
        FPM383_enroll(id);
    }
    else if (dat == 0x02)
    { // 删除当期指纹
        if (action_state == 1)
        {
            OLED_Clear();
            OLED_Display_GB2312_string(0, 0, "已删掉当前指");
            OLED_Display_GB2312_string(0, 2, "纹!!");
            FPM383_delete(id);
            action_state = 0;
            delay_s(2);
            IPS_Show();
        }
        else if (action_state == 2)
        {
            OLED_Clear();
            OLED_Display_GB2312_string(0, 0, "已删掉全部指");
            OLED_Display_GB2312_string(0, 2, "纹!!");
            FPM383_empty();
            id = 0x0000;
            action_state = 0;
            delay_s(2);
            IPS_Show();
        }else if(action_state == 3){
            OLED_Clear();
            OLED_Display_GB2312_string(0, 0, "已初始化密码");
            OLED_Display_GB2312_string(0, 2, "成功!!");
            password_init();
            action_state = 0;
        }else
        {
            IPS_Show();
        }
    }
    else if (dat == 0x21)
    { // 开门
        open_door();
    }else if (dat == 0x05)
    {
        action_state = 1;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "是否确认删掉");
        OLED_Display_GB2312_string(0, 2, "当前指纹?");
        printf("是否确认删掉指定指纹 %d ? \n");
    }
    else if (dat == 0x06)
    {
        action_state = 2;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "是否确认清空");
        OLED_Display_GB2312_string(0, 2, "全部指纹?");
        printf("是否确认清空全部指纹? \n");
    }else if(dat == 0x11){
        action_state = 3;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "是否确认初始");
        OLED_Display_GB2312_string(0, 2, "化密码?");
    }
}

void sys_init()
{
    EAXSFR(); /* 扩展寄存器访问使能 */
    GPIO_config();
    UART_config();
    PWM_config();
    // 初始化
    MK_init();
    FPM383_init();
    Ultrasonic_init();//超声波
    EA = 1;
    //EEPROM_SectorErase(addr); // 擦除
    //EEPROM_write_n(addr, password, 12); // 写
}

// 程序入口
void start() _task_ 0
{
    sys_init();

    // 创建任务1
    os_create_task(1);
    // 创建任务2
    os_create_task(2);
    // 创建任务3
    os_create_task(3);
    // 创建任务4
    os_create_task(4);
    // 创建任务5
    os_create_task(5);
    // 删除任务0
    os_delete_task(0);
}
void task_1() _task_ 1
{
    OLED_Init();
    OLED_ColorTurn(0);   // 0正常显示，1 反色显示
    OLED_DisplayTurn(0); // 0正常显示 1 屏幕翻转显示
    IPS_Show();

    while (1)
    {
        // 扫描按键
        MK_scan();
        os_wait2(K_TMO, 2); // 睡2个单位5ms
    }
}

void task_uart1() _task_ 2
{
    u8 i;
    while (1)
    {
        if (COM1.RX_TimeOut > 0)
        {
            // 超时计数
            if (--COM1.RX_TimeOut == 0)
            {
                if (COM1.RX_Cnt > 0)
                {
                    for (i = 0; i < COM1.RX_Cnt; i++)
                    {
                        TX2_write2buff(RX1_Buffer[i]);
                        FPM383_send_instruction(RX1_Buffer, COM1.RX_Cnt);
                    }
                }
                COM1.RX_Cnt = 0;
            }
        }
        os_wait2(K_TMO, 2); // 5ms * 2 = 10ms
    }
}

void on_uart4_receive()
{
    u8 length = 0, package_len = 0;
    u8 i;

    for (i = 0; i < COM4.RX_Cnt - 8; i++)
    {
        // 如果找到 0xEF 0x01
        if (RX4_Buffer[i] == 0xEF && RX4_Buffer[i + 1] == 0x01)
        {
            // 检查包长度是否合法
            length = (RX4_Buffer[i + 7] << 8) | RX4_Buffer[i + 8]; // 索引7,8是数据包长度位
            package_len = 9 + length;                              // 9：包含2个包头，4个设备码，1包标识，2包长度
            if (length <= 0 || (i + package_len) > COM4.RX_Cnt)
            {
                // 包长度不合法，跳过这一段数据
                break;
            }
            // 发送数据
            FPM383_handle_instruction(&RX4_Buffer[i], package_len);
            // 跳过已经发送的数据
            i += package_len - 1;
        }
    }
}

void task3() _task_ 3
{
    u8 i;
    while (1)
    {
        if (COM4.RX_TimeOut > 0)
        {
            // 超时计数
            if (--COM4.RX_TimeOut == 0)
            {
                if (COM4.RX_Cnt > 0)
                {
                    for (i = 0; i < COM4.RX_Cnt; i++)
                    {
                        on_uart4_receive();
                    }
                }
                COM4.RX_Cnt = 0;
            }
        }
        os_wait2(K_TMO, 2);
    }
}

void task_4() _task_ 4
{
    u8 i;
    while (1)
    {
        if (COM2.RX_TimeOut > 0)
        {
            // 超时计数
            if (--COM2.RX_TimeOut == 0)
            {
                if (COM2.RX_Cnt > 0)
                {
                    for (i = 0; i < COM2.RX_Cnt; i++)
                    {
                        do_work(RX2_Buffer[i]);
                        TX1_write2buff(RX2_Buffer[i]);
                    }
                }
                COM2.RX_Cnt = 0;
            }
        }
        os_wait2(K_TMO, 2);
    }
}

void task_5() _task_ 5
{   
    char res;
    float distance;
    while(1){
        os_wait2(K_TMO, 50);
        res = Ultrasonic_get_distance(&distance);
        if (res == 0) {
        //printf("距离为：%.2f cm\n", distance);
        if(distance < 8){
            os_wait2(K_TMO, 150);
            if(distance < 8){
                
            open_door();
            }
            }
        } else {
        printf("res = %d\n", (int)res);
        } 

}

}