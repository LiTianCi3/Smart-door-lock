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
int user_password[MAX_PASSWORD_LENGTH] = {0}; // ����һ�������û��������������
char show_str[7] = {0};                       // spi��Ļ�ϵ���������
u8 password_length = 0;
u8 error = 1;
int temp_password[MAX_PASSWORD_LENGTH] = {0}; // ����һ���û��޸�������ݴ�����
u8 open = 0;
u8 lock = 1; // ��־�Ƿ����ɹ�
//==========================eeprom
u16 addr = 0x0000;
int buf[6] = {0};

int password[MAX_PASSWORD_LENGTH] = {2, 3, 4, 5, 6, 7}; // ����һ����ʼ����
//==========================eeprom
u8 action_state = 0;
u16 id = 0x0000;

// ����һ��4*4�ľ����������
char keypad[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

void GPIO_config()
{ // ׼˫���	IPS��Ļ
    P1_MODE_IO_PU(GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_6);
    P4_MODE_IO_PU(GPIO_Pin_7);
    P5_MODE_IO_PU(GPIO_Pin_0);

    // ׼˫���	30 31 ����
    P3_MODE_IO_PU(GPIO_Pin_0 | GPIO_Pin_1);

    // �������	��� P21
    P2_MODE_OUT_PP(GPIO_Pin_1);

    // ׼˫���	uart2: P10 P11
    P1_MODE_IO_PU(GPIO_Pin_0 | GPIO_Pin_1);

    // �������	������ P00
    P0_MODE_OUT_PP(GPIO_Pin_0);
}

void UART_config(void)
{
    // >>> �ǵ���� NVIC.c, UART.c, UART_Isr.c <<<
    COMx_InitDefine COMx_InitStructure;             // �ṹ����
    COMx_InitStructure.UART_Mode = UART_8bit_BRTx;  // ģʽ, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
    COMx_InitStructure.UART_BRT_Use = BRT_Timer1;   // ѡ�����ʷ�����, BRT_Timer1, BRT_Timer2 (ע��: ����2�̶�ʹ��BRT_Timer2)
    COMx_InitStructure.UART_BaudRate = 115200ul;    // ������, һ�� 110 ~ 115200
    COMx_InitStructure.UART_RxEnable = ENABLE;      // ��������,   ENABLE��DISABLE
    COMx_InitStructure.BaudRateDouble = DISABLE;    // �����ʼӱ�, ENABLE��DISABLE
    UART_Configuration(UART1, &COMx_InitStructure); // ��ʼ������1 UART1,UART2,UART3,UART4

    NVIC_UART1_Init(ENABLE, Priority_1); // �ж�ʹ��, ENABLE/DISABLE; ���ȼ�(�͵���) Priority_0,Priority_1,Priority_2,Priority_3
    UART1_SW(UART1_SW_P30_P31);          // ����ѡ��, UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44

    // uart2
    COMx_InitStructure.UART_Mode = UART_8bit_BRTx;  // ģʽ, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
    COMx_InitStructure.UART_BRT_Use = BRT_Timer2;   // ѡ�����ʷ�����, BRT_Timer1, BRT_Timer2 (ע��: ����2�̶�ʹ��BRT_Timer2)
    COMx_InitStructure.UART_BaudRate = 9600ul;      // ������, һ�� 110 ~ 115200
    COMx_InitStructure.UART_RxEnable = ENABLE;      // ��������,   ENABLE��DISABLE
    COMx_InitStructure.BaudRateDouble = DISABLE;    // �����ʼӱ�, ENABLE��DISABLE
    UART_Configuration(UART2, &COMx_InitStructure); // ��ʼ������1 UART1,UART2,UART3,UART4

    NVIC_UART2_Init(ENABLE, Priority_1); // �ж�ʹ��, ENABLE/DISABLE; ���ȼ�(�͵���) Priority_0,Priority_1,Priority_2,Priority_3
    UART2_SW(UART2_SW_P10_P11);          // ����ѡ��
}

#define HZ_VALUE 50 // Ƶ��
#define PRE 10      // ��Ƶϵ��

u16 duty_value = 500;
#define PERIOD (MAIN_Fosc / (HZ_VALUE * PRE)) // (HZ_VALUE * PRE) Ҫ��(), �����ȼ�����
void PWM_config()
{
    PWMx_InitDefine PWMx_InitStructure;

    // ����PWM6
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // ģʽ,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    PWMx_InitStructure.PWM_Duty = 0;               // PWMռ�ձ�ʱ��, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO6P;      // ���ͨ��ѡ��,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM6, &PWMx_InitStructure);  // ��ʼ��PWM,  PWMA,PWMB

    // ����PWMB
    PWMx_InitStructure.PWM_Period = PERIOD - 1;    // ����ʱ��,   0~65535
    PWMx_InitStructure.PWM_DeadTime = 0;           // ��������������, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // �����ʹ��, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;    // ʹ�ܼ�����, ENABLE,DISABLE
    PWM_Configuration(PWMB, &PWMx_InitStructure);  // ��ʼ��PWMͨ�üĴ���,  PWMA,PWMB

    // ��Ƶ ͷ�ļ�515������
    PWMB_Prescaler(PRE - 1);

    // �л�PWMͨ��
    PWM6_SW(PWM6_SW_P21); // PWM6_SW_P21,PWM6_SW_P54,PWM6_SW_P01,PWM6_SW_P75

    // ��ʼ��PWMB���ж�
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
    { // ������ʾ������
        show_str[i] = '\0';
    }
}

// ����һ�����������������
void open_door()
{
    PWMx_Duty dutyB;
    // ����
    float angle;
    error = 1;
    lock = 0;
    duty_value = 2000;
    dutyB.PWM6_Duty = duty_value * PERIOD / 20000.0;
    angle = (duty_value - 500) * 180.0 / 2000;
    printf("duty_value = %d, angle = %.2f ��\n", (int)duty_value, angle);
    UpdatePwm(PWM6, &dutyB);
    buzzer_init();
    OLED_Clear();
    OLED_Display_GB2312_string(20, 2, "��ӭ�ؼ�!!");
    delay_s(3);

    // ����
    duty_value = 500;
    dutyB.PWM6_Duty = duty_value * PERIOD / 20000.0;
    angle = (duty_value - 500) * 180.0 / 2000;
    printf("duty_value = %d, angle = %.2f ��\n", (int)duty_value, angle);
    UpdatePwm(PWM6, &dutyB);
    buzzer_init();
    IPS_Show();
}

// �Ƚ�����
char check_password()
{
    u8 i;
    EEPROM_read_n(addr, buf, 12); // ��
    printf("����");
    for (i = 0; i < 6; i++)
    { // ��ʾ��ȡ������
        printf(" %d,", buf[i]);
    }
    if (password_length != 6) // �������볤���Ƿ�Ϊ6
    {
        printf("���볤�ȴ���\n");
        error++;
        password_length = 0;
        IPS_Init();
        OLED_Display_GB2312_string(0, 2, "�������");
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
                printf("�������\n");
                password_length = 0;
                IPS_Init();
                OLED_Display_GB2312_string(0, 2, "�������");
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
                printf("�������벻һ��\n");
                password_length = 0;
                IPS_Init();
                OLED_Display_GB2312_string(0, 2, "�������벻һ��");
                OLED_Display_GB2312_string(0, 4, "���ٴ�����");
                open = 2;
                delay_s(1);
                OLED_Clear();
                OLED_Display_GB2312_string(0, 0, "������������:");
                show_str_init();
                return 0;
            }
        }
    }
    show_str_init();
    return 1;
}

// ����һ������ɾ���û�������ַ�  *
void delete_user_input_password()
{
    user_password[password_length] = NULL;
    show_str[password_length - 1] = '\0';
    password_length--;

    // ������볤��С��0,�������볤�ȵ���0
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
        OLED_Display_GB2312_string(0, 0, "������ԭ����:");
    }
    else if (open == 3 || open == 2)
    {
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "������������:");
    }
    OLED_Display_GB2312_string(0, 2, show_str);
}

// ���û�������ַ�ƴ�ӳ��ַ���
void user_input_password(char key)
{
    u8 i;
    int num = key - '0';
    if (password_length == 0 && num == 0)
    {
        printf("Error: First digit cannot be 0!\n");
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "��λ����Ϊ0");
        delay_s(1);
        IPS_Show();
    }
    else
    {
        if (password_length >= 6)
        {
            printf("Error: Password length cannot be greater than 6!\n");
            OLED_Display_GB2312_string(0, 2, "          ");
            OLED_Display_GB2312_string(0, 2, "�������");
            delay_s(1);
            IPS_Show();
            password_length = 0;
            for (i = 0; i < 6; i++)
            { // ������ʾ������
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
                OLED_Display_GB2312_string(0, 0, "������ԭ����:");
            }
            else if (open == 3 || open == 2)
            {
                OLED_Clear();
                OLED_Display_GB2312_string(0, 0, "������������:");
            }

            OLED_Display_GB2312_string(0, 2, show_str);
        }
    }
}

void handleHashKey()
{
    u8 i;
    // ����������3�Σ�������
    printf("error = %d\n", (int)error);
    if (error > 3)
    {   lock=1;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "���������");
        OLED_Display_GB2312_string(0, 2, "����ϵ����Ա");
        OLED_Display_GB2312_string(0, 4, "����!!");
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
            printf("������ȷ������\n");
            OLED_Clear();
            OLED_Display_GB2312_string(0, 2, "   ok!");
            delay_s(1);
            if (open == 0)
            { // ����
                open_door();
            }
            else if (open == 1)
            { // �޸�����
                OLED_Clear();
                OLED_Display_GB2312_string(0, 0, "������������:");
                for (i = 0; i < 6; i++)
                { // ������ʾ������
                    show_str[i] = '\0';
                }
                open = 2;
            }
        }
    }
    else if (open == 2)
    { // �޸�����
        for (i = 0; i < 6; i++)
        {
            temp_password[i] = user_password[i]; // �����û����������
            printf("t=%d\n", temp_password[i]);
        }
        password_length = 0;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "���ٴ�����");
        for (i = 0; i < 6; i++)
        { // ������ʾ������
            show_str[i] = '\0';
        }
        open = 3;
        printf("open = %d", (int)open);
    }
    else if (open == 3)
    { // �޸�����
        if (check_password())
        {
            EEPROM_SectorErase(addr);                // ����
            EEPROM_write_n(addr, temp_password, 12); // д
            password_length = 0;
            printf("������ȷ���޸�����\n");
            OLED_Clear();
            OLED_Display_GB2312_string(2, 0, "�޸ĳɹ�!");
            delay_s(1);
            IPS_Show();
            for (i = 0; i < 6; i++)
            { // ������ʾ������
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
        OLED_Display_GB2312_string(0, 0, "������ԭ����:");
        open = 1;
    }
    else if (key == 'B')
    {
        OLED_Clear();
        OLED_Display_GB2312_string(0, 2, "�밴��ָ��");
        if (FPM383_cancel())
        {
            IPS_Show();
            return;
        }
        FPM383_identify(); // ָ��ʶ��
    }
}

// ���µĻص�����
void MK_on_keydown(u8 row, u8 col)
{
    int ROW, COL;
    char key;
    ROW = (int)row;
    COL = (int)col;
    key = keypad[ROW][COL];
    printf("key=%c\n", key);
    key_value(key);    // ����һ���������������º����Ϣ����ȥ
    // printf("��%d�е�%d�а�������\n", ROW + 1, COL + 1);
}
// ̧��Ļص�����
void MK_on_keyup(u8 row, u8 col)
{
    // printf("��%d�е�%d�а���̧��\n", (int)(row + 1), (int)(col + 1));
}

//����һ����ʼ������ĺ���
void password_init()
{   int init_password[6] = {2, 3, 4, 5, 6, 7}; // ����һ����ʼ����
    EEPROM_SectorErase(addr); // ����
    EEPROM_write_n(addr, init_password, 12); // д
    OLED_Clear();
    OLED_Display_GB2312_string(0, 0, "��ʼ�������");
    OLED_Display_GB2312_string(0, 2, "��!!!");
    delay_s(2);
    IPS_Show();
}

void do_work(u8 dat)
{
    if (dat == 0x44)// ȡ��
    { 
        action_state = 0;
        IPS_Show();
        if (FPM383_cancel())
        {
            return;
        }
    }
    else if (dat == 0x01)
    { // ע��ָ��
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "ע��ָ��");
        OLED_Display_GB2312_string(0, 2, "��������5��ָ��");
        if (FPM383_cancel())
        {
            return;
        }
        id += 0x0001;
        FPM383_enroll(id);
    }
    else if (dat == 0x02)
    { // ɾ������ָ��
        if (action_state == 1)
        {
            OLED_Clear();
            OLED_Display_GB2312_string(0, 0, "��ɾ����ǰָ");
            OLED_Display_GB2312_string(0, 2, "��!!");
            FPM383_delete(id);
            action_state = 0;
            delay_s(2);
            IPS_Show();
        }
        else if (action_state == 2)
        {
            OLED_Clear();
            OLED_Display_GB2312_string(0, 0, "��ɾ��ȫ��ָ");
            OLED_Display_GB2312_string(0, 2, "��!!");
            FPM383_empty();
            id = 0x0000;
            action_state = 0;
            delay_s(2);
            IPS_Show();
        }else if(action_state == 3){
            OLED_Clear();
            OLED_Display_GB2312_string(0, 0, "�ѳ�ʼ������");
            OLED_Display_GB2312_string(0, 2, "�ɹ�!!");
            password_init();
            action_state = 0;
        }else
        {
            IPS_Show();
        }
    }
    else if (dat == 0x21)
    { // ����
        open_door();
    }else if (dat == 0x05)
    {
        action_state = 1;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "�Ƿ�ȷ��ɾ��");
        OLED_Display_GB2312_string(0, 2, "��ǰָ��?");
        printf("�Ƿ�ȷ��ɾ��ָ��ָ�� %d ? \n");
    }
    else if (dat == 0x06)
    {
        action_state = 2;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "�Ƿ�ȷ�����");
        OLED_Display_GB2312_string(0, 2, "ȫ��ָ��?");
        printf("�Ƿ�ȷ�����ȫ��ָ��? \n");
    }else if(dat == 0x11){
        action_state = 3;
        OLED_Clear();
        OLED_Display_GB2312_string(0, 0, "�Ƿ�ȷ�ϳ�ʼ");
        OLED_Display_GB2312_string(0, 2, "������?");
    }
}

void sys_init()
{
    EAXSFR(); /* ��չ�Ĵ�������ʹ�� */
    GPIO_config();
    UART_config();
    PWM_config();
    // ��ʼ��
    MK_init();
    FPM383_init();
    Ultrasonic_init();//������
    EA = 1;
    //EEPROM_SectorErase(addr); // ����
    //EEPROM_write_n(addr, password, 12); // д
}

// �������
void start() _task_ 0
{
    sys_init();

    // ��������1
    os_create_task(1);
    // ��������2
    os_create_task(2);
    // ��������3
    os_create_task(3);
    // ��������4
    os_create_task(4);
    // ��������5
    os_create_task(5);
    // ɾ������0
    os_delete_task(0);
}
void task_1() _task_ 1
{
    OLED_Init();
    OLED_ColorTurn(0);   // 0������ʾ��1 ��ɫ��ʾ
    OLED_DisplayTurn(0); // 0������ʾ 1 ��Ļ��ת��ʾ
    IPS_Show();

    while (1)
    {
        // ɨ�谴��
        MK_scan();
        os_wait2(K_TMO, 2); // ˯2����λ5ms
    }
}

void task_uart1() _task_ 2
{
    u8 i;
    while (1)
    {
        if (COM1.RX_TimeOut > 0)
        {
            // ��ʱ����
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
        // ����ҵ� 0xEF 0x01
        if (RX4_Buffer[i] == 0xEF && RX4_Buffer[i + 1] == 0x01)
        {
            // ���������Ƿ�Ϸ�
            length = (RX4_Buffer[i + 7] << 8) | RX4_Buffer[i + 8]; // ����7,8�����ݰ�����λ
            package_len = 9 + length;                              // 9������2����ͷ��4���豸�룬1����ʶ��2������
            if (length <= 0 || (i + package_len) > COM4.RX_Cnt)
            {
                // �����Ȳ��Ϸ���������һ������
                break;
            }
            // ��������
            FPM383_handle_instruction(&RX4_Buffer[i], package_len);
            // �����Ѿ����͵�����
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
            // ��ʱ����
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
            // ��ʱ����
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
        //printf("����Ϊ��%.2f cm\n", distance);
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