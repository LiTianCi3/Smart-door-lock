#include "FPM383C.h"




// �Զ�У��ָ����
#define INST_CODE_AUTO_DIENTIFY 0x32
// �Զ�ע��ָ����
#define INST_CODE_AUTO_ENROLL 0x31
// ȡ��ע�����ָ֤����
#define INST_CODE_CANCEL 0x30
// ��ȡ¼���ָ��������
#define INST_CODE_GET_ENROLLED 0x1F
// LED����ָ��
#define INST_CODE_LED_CONTROL 0x3C
// ɾ��ָ��ָ��
#define INST_CODE_DELETE_CHAR 0x0C
// �������ָ��
#define INST_CODE_EMPTY 0x0D

// ��¼Ҫ���е�ָ����
u8 instruction_code = 0x00;

// �ȴ���һ��ָ����Ӧ���
#define WAIT_FOR_RESPONSE(wait_time) \
    while (instruction_code != 0x00 && wait_time-- > 0) \
    { \
        delay_ms(1); \
    } \

// ģ��LED�ƿ���Э��
// ���Ƴ���
u8 code PS_BlueLEDBuffer[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, INST_CODE_LED_CONTROL,
    0x03, 0x01, 0x01, 0x00, 0x00, 0x49
};
// �����
u8 code PS_RedLEDBuffer[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, INST_CODE_LED_CONTROL,
    0x02, 0x04, 0x04, 0x02, 0x00, 0x50
};
// �̵���
u8 code PS_GreenLEDBuffer[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, INST_CODE_LED_CONTROL,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x4C
};

// �Զ���ָ֤��ģ��
u8 code PS_AutoIdentifyBuffer[17] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x08, // ����ʶ01H��������0008H
    INST_CODE_AUTO_DIENTIFY,
    0x01,                  // �����ȼ�
    0xFF, 0xFF,            // ID�� ��д0xFFFF ��1��Nȫ��������������1��1��������
    0x00, 0x04, 0x02, 0x3E // ����0x0004��У���0x023E
};

// �Զ�ע��ָ��ģ��
u8 code PS_AutoEnroll[17] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x08,      // ����ʶ01H��������0008H (index��[6, 8])	 ���￪ʼ����У��ͣ���������ʶ
    INST_CODE_AUTO_ENROLL, // ָ����   index: [9]					0x31
    0x00, 0x05,            // ID��     index: [10, 11]      ��0x00, 0x05Ϊ����ֻ�޸ĵ�2���ֽھ͹��ã���Χ��[0,255]�����Ҳֻ��֧��50��ָ�ơ�
    0x05,                  // ¼����� index: [12]
    0x00, 0x1B,            // ����     index: [13, 14] 0001 1011			 �����������У��ͣ�������У���
    0x00, 0x5F             // У���   index: [15, 16]
};

// ȡ���Զ���֤��ע������
u8 code PS_Cancel[12] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x03, // ����ʶ01H��������0003H (index��[6, 8])	 ���￪ʼ����У��ͣ���������ʶ
    INST_CODE_CANCEL, // ָ����   index: [9]					0x30
    0x00, 0x34        // У���   index: [10, 11]
};

// ��ȡ��ע��ָ������λ��
u8 code PS_ReadIndexTable[13] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x04,       // ����ʶ01H��������0004H (index��[6, 8])	 ���￪ʼ����У���
    INST_CODE_GET_ENROLLED, // ָ����   index: [9]					0x1F
    0x00,                   // ҳ�� index:      [10]
    0x00, 0x24              // У���   index: [11, 12]
};

// ɾ��ָ��ID�ſ�ʼ��N��ָ��ģ��
u8 code PS_DeleteChar[16] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x07, // ����ʶ01H��������0007H (index��[6, 8])	 ���￪ʼ����У���
    INST_CODE_DELETE_CHAR,
    '\0', '\0',  // ID��     index: [10, 11]
    '\0', '\0',  // ɾ������ index: [12, 13]
    '\0', '\0',  // У���   index: [14, 15]
};

// ���ָ�ƿ�
u8 code PS_Empty[12] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x03, // ����ʶ01H��������0003H (index��[6, 8])	 ���￪ʼ����У���
    INST_CODE_EMPTY, // ָ����   index: [9]					0x20
    0x00, 0x11        // У���   index: [10, 11]
};


static void GPIO_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure; // �ṹ����
    // UART4
    GPIO_InitStructure.Pin = GPIO_Pin_2 | GPIO_Pin_3; // ָ��Ҫ��ʼ����IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;            // ָ��IO������������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P5, &GPIO_InitStructure);       // ��ʼ��
}

static void UART_config()
{
    COMx_InitDefine		COMx_InitStructure;					//�ṹ����
    COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//ģʽ, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
    COMx_InitStructure.UART_BRT_Use   = BRT_Timer4;			//ѡ�����ʷ�����, BRT_Timer1, BRT_Timer2 (ע��: ����2�̶�ʹ��BRT_Timer2)
    COMx_InitStructure.UART_BaudRate  = 57600ul;			//������, һ�� 110 ~ 115200
    COMx_InitStructure.UART_RxEnable  = ENABLE;				//��������,   ENABLE��DISABLE
    COMx_InitStructure.BaudRateDouble = DISABLE;			//�����ʼӱ�, ENABLE��DISABLE
    UART_Configuration(UART4, &COMx_InitStructure);		//��ʼ������1 UART1,UART2,UART3,UART4

    NVIC_UART4_Init(ENABLE,Priority_1);		//�ж�ʹ��, ENABLE/DISABLE; ���ȼ�(�͵���) Priority_0,Priority_1,Priority_2,Priority_3
    UART4_SW(UART4_SW_P52_P53);
}
void FPM383_init(void)
{
    GPIO_config();
    UART_config();
}

/**
 * @brief FPM383ָ��ʶ��
 *
 * \param buffer
 * \param cnt
 * \return u8
 *
EF 01 FF FF FF FF 01 00 08 31 00 05 03 00 1A 00 5C //�� 3 �Σ��� 05 ��
31		ָ����
00 05	ID��
03		¼�����
00 1A	���Ը��� ID �ţ��������ظ�ע��, ÿ�βɼ�Ҫ����ָ�뿪

00 01 02 03 04 05 06 07 08   09 10 11   12 13	// index
                             ȷ ��1��2
EF 01 FF FF FF FF 07 00 05 | 00 00 00 | 00 0C //ָ��Ϸ��Լ��  	00 �Ϸ�
EF 01 FF FF FF FF 07 00 05 | 00 01 01 | 00 0E //��1�β�ͼ��� 	  00 �ɹ�
EF 01 FF FF FF FF 07 00 05 | 00 02 01 | 00 0F //��1�������������  00�ɹ�
EF 01 FF FF FF FF 07 00 05 | 00 03 01 | 00 10 //��1����ָ�뿪	00�ɹ�
EF 01 FF FF FF FF 07 00 05 | 00 01 02 | 00 0F //��2�β�ͼ���
EF 01 FF FF FF FF 07 00 05 | 00 02 02 | 00 10 //��2����ָ�뿪
EF 01 FF FF FF FF 07 00 05 | 00 03 02 | 00 11 //��2����ָ�뿪
EF 01 FF FF FF FF 07 00 05 | 00 01 03 | 00 10 //��3�β�ͼ���
EF 01 FF FF FF FF 07 00 05 | 00 02 03 | 00 11 //��3����ָ�뿪
EF 01 FF FF FF FF 07 00 05 | 00 04 F0 | 01 00 //�ϲ�ģ��					00 �ɹ�
EF 01 FF FF FF FF 07 00 05 | 00 05 F1 | 01 02 //�����ָ�Ƿ���ע��
EF 01 FF FF FF FF 07 00 05 | 00 06 F2 | 01 04 //ģ��洢�ɹ�
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
    // У��� = ��[7,cnt - 2]���ֽڵĺ� & 0xFFFF
    u16 sum = 0;
    u8 i;
    for (i = 6; i < cnt - 2; i++)
    {
        sum += buffer[i];
    }
    sum &= 0xFFFF;
    return sum;
}

// ���У����Ƿ���ȷ����ȷ����0�����󷵻�1
u8 check_sum(u8 *buffer, u8 cnt)
{
    u16 actual_check_sum = get_checksum(buffer, cnt);
    // ��ȡĩβ�����ֽڵ�ֵ
    u16 except_check_sum = buffer[cnt - 2] << 8 | buffer[cnt - 1];
    if (actual_check_sum != except_check_sum)
    {
        printf("У��ʹ���! actual 0x%04X != except 0x%04X\n", (int)actual_check_sum, (int)except_check_sum);
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
        printf("�Զ�ע�᷵�س��ȴ���! %d\n", (int)cnt);
        // ��ӡ����Ķ���������
        printf_buffer(buffer, cnt);
        return 1;
    }
    confirm = buffer[9];
    arg1 = buffer[10];
    arg2 = buffer[11];
    printf("�Զ�ע�᷵��: 0x%02X 0x%02X 0x%02X -> ", (int)confirm, (int)arg1, (int)arg2);

    if (arg1 == 0x00 && arg2 == 0x00)
    {   // ָ��Ϸ��Լ��
        printf(confirm == 0x00 ? "ָ��Ϸ��Լ��ɹ�!\n" : "ָ��Ϸ��Լ��ʧ��!\n");
    }
    else if (arg1 == 0x01)
    {
        printf((confirm == 0x00 ? "��%d�β�ͼ�ɹ�!\n" : "��%d�β�ͼʧ��!\n"), (int)arg2);
    }
    else if (arg1 == 0x02)
    {
        printf((confirm == 0x00 ? "��%d�����������ɹ�!\n" : "��%d����������ʧ��!\n"), (int)arg2);
    }
    else if (arg1 == 0x03)
    {
        printf((confirm == 0x00 ? "��%d����ָ�뿪!\n" : "��%d����ָδ�뿪!\n"), (int)arg2);
    }
    else if (arg1 == 0x04)
    {
        printf((confirm == 0x00 ? "ָ��ģ��ϲ��ɹ�!\n" : "ָ��ģ��ϲ�ʧ��!\n"));
    }
    else if (arg1 == 0x05) // ע��У��
    {
        if (confirm == 0x27)
        {
            printf(">> ָ���Ѵ��� << ");
        }
        printf((confirm == 0x00 ? "ָ����ע��ɹ�!\n" : "ָ��ע��ʧ��!\n"));
    }
    else if (arg1 == 0x06 && arg2 == 0xF2)
    {
        printf((confirm == 0x00 ? "��ϲ��ָ��ģ��洢�ɹ�!!!\n" : "��Ǹ��ָ��ģ��洢ʧ��!\n"));
        return 2;
    }
    else
    {
        printf("δ֪����!\n");
    }
    if (confirm != 0x00)
    {
        switch (confirm)
        {
        case 0x0B:
            printf(">> ָ���洢ID��Ч << ");
            break;
        case 0x25:
            printf(">> ¼��������ô��� << ");
            break;
        case 0x1F:
            printf(">> ָ�ƿ����� << ");
            break;
        case 0x22:
            printf(">> ָ��ID���Ѵ���ָ�� << ");
            break;
        default:
            break;
        }
        printf("�Զ�ע��ʧ��! ����ȷ���룺0x%02X \n", (int)confirm);
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
        printf("�Զ�ʶ�𷵻س��ȴ���! %d\n", (int)cnt);
        return 1;
    }

    // ����У��������17���ֽڣ������λ��У���
    // У��� = ��[7,15]���ֽڵĺ� & 0xFFFF
    if (check_sum(buffer, cnt) != 0)
    {
        return 1;
    }
    confirm_code = buffer[9];

    switch (confirm_code)
    {
    case 0x00:
        // ȡ�� ��12,13ָ��ID��14,15�÷�
        id = buffer[11] << 8 | buffer[12];
        score = buffer[13] << 8 | buffer[14];
        printf("ָ����֤�ɹ� ָ��ID:%d �÷�:%d !", id, score);
        break;
    case 0x26:
        printf("ָ����֤ʧ��, ������ʱ! -> %d\n", (int)confirm_code);
        return 1;
    case 0x02:
        printf("ָ����֤ʧ��, ��������δ��⵽��ָ! -> %d\n", (int)confirm_code);
        return 1;
    case 0x01:
    case 0x07:
    case 0x09:
    default:
        if (confirm_code == 0x09)
        {
            printf(">> δ������ָ�� << ");
        }
        printf("ָ����֤ʧ��! -> %02X\n", (int)confirm_code);
        return 1;
    }


    return 0;
}

u8 handle_cancel_result(u8 *buffer, u8 cnt)
{
    u8 confirm_code;
    if (cnt != 12)
    {
        printf("ȡ��ָ��س��ȴ���! %d\n", (int)cnt);
        return 1;
    }

    confirm_code = buffer[9];
    if (confirm_code != 0x00)
    {
        printf("ȡ��ָ��ʧ��! -> %d\n", (int)confirm_code);
        return 0;
    }

    printf("ȡ��ָ��ɹ�!\n");
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
        printf("��ȡ��ע��ָ������ʧ��! -> %d\n", (int)confirm_code);
        return 1;
    }

    printf("��ע��ָ��-> ");
    // ������10��ʼ��ȡ����32bytes
    for (i = 0; i < 32; i++)
    {
        ids_mask = buffer[i + 10];
        // printf("%02X ", (int)ids_mask);
        if (ids_mask == 0x00)
        {
            continue;
        }
        // ��ӡһ���ֽ���ֵΪ1��λ������ (��32bytes * 8 = 256��λ)
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
        // ��¼ָ���룬�����յ�Ӧ����ж���ʲô����
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

    // �ж��Ƿ���ģ���Ӧ��
    if (buffer[0] != 0xEF || buffer[1] != 0x01)
    {
        return 1;
    }

    // if(instruction_code != INST_CODE_LED_CONTROL){ // ����LED����ָ���������ӡ
    //     // ��ӡ16����1���ֽڵ�ָ����instruction_code
    //     printf("��ǰָ����:0x%02X ", (int)instruction_code);
    // }

    switch (instruction_code)
    {
    case INST_CODE_AUTO_DIENTIFY: // ָ��У��
        // printf("�յ����Զ�ʶ��ָ�Ӧ��\n");
        instruction_code = 0x00;
        result = handle_auto_identify_result(buffer, cnt);
        if (result == 0)
        {   // success
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//�̵���
            open_door();
        }
        else
        {   // failure
            FPM383_send_instruction(PS_RedLEDBuffer, 16);//�����
        }
        break;
    case INST_CODE_AUTO_ENROLL: // ָ��ע��
        // printf("�յ����Զ�ע��ָ�Ӧ��\n");
        result = handle_auto_enroll_result(buffer, cnt);
        if (result == 2)
        {   // success
            instruction_code = 0x00;
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//�̵���
            OLED_Clear();
            OLED_Display_GB2312_string(20, 2, "ע��ɹ�!!");           
            delay_s(3);
            OLED_Clear();
        }
        else if (result == 1)
        {   // failure
            instruction_code = 0x00;
            FPM383_send_instruction(PS_RedLEDBuffer, 16);//�����
        }
        break;
    case INST_CODE_CANCEL: // ȡ��ָ��ע��
        // printf("�յ���ȡ��ָ�Ӧ��\n");
        result = handle_cancel_result(buffer, cnt);
        if (result == 0)
        {
            // success
            instruction_code = 0x00;
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//�̵���

        }
        break;
    case INST_CODE_GET_ENROLLED: // ��ȡ��¼��ָ������
        // printf("�յ�����ȡ��ע��ָ�ơ�ָ��Ӧ��\n");
        result = handle_get_enrolled_result(buffer, cnt);
        if (result == 0)
        {
            // success
            instruction_code = 0x00;
            FPM383_send_instruction(PS_GreenLEDBuffer, 16);//�̵���
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
            printf("ָ��ָ��ɾ���ɹ�!\n");
        }
        break;
    case INST_CODE_EMPTY:
        instruction_code = 0x00;
        if (buffer[9] == 0x00) {
            printf("����ָ����ճɹ�!\n");
        }
        break;
    case INST_CODE_LED_CONTROL: // LED����
        // printf("�յ���LED����ָ�Ӧ��\n");
        instruction_code = 0x00;
        break;
    default:
        // printf("�յ�δָ֪��Ӧ��δָ֪�����£�\n");
        // printf_buffer(buffer, cnt);
        instruction_code = 0x00;
        break;
    }
    return 0;
}

// -----------------------------------------------------------------������ģ��ָ�����������

void FPM383_enroll(u16 id)
{
    u16 wait_time = 1000;
    u16 checksum;
    u8 idata PS_AutoEnroll_Copy[17];
    printf("����ָ��ע�ᣬ�밴��̧��5����ָ��\n");
    // ���Ƴ���
    FPM383_send_instruction(PS_BlueLEDBuffer, 16);

    WAIT_FOR_RESPONSE(wait_time);

    // ��PS_AutoEnroll����һ��
    memcpy(PS_AutoEnroll_Copy, PS_AutoEnroll, 17);

    // �޸�[10, 11]ΪҪ¼��λ�õ�ID��
    PS_AutoEnroll_Copy[10] = (u8)(id >> 8);
    PS_AutoEnroll_Copy[11] = (u8)(id & 0xFF);

    // �޸�ĩβ��λУ���
    checksum = get_checksum(PS_AutoEnroll_Copy, 17);
    PS_AutoEnroll_Copy[15] = (u8)(checksum >> 8);
    PS_AutoEnroll_Copy[16] = (u8)(checksum & 0xFF);

    // printf_buffer(PS_AutoEnroll_Copy, 17);

    // �Զ�ע��ָ��PS_AutoEnroll
    FPM383_send_instruction(PS_AutoEnroll_Copy, 17);
}

void FPM383_identify()
{
    u16 wait_time = 1000;
    printf("����ָ��ʶ��\n");
    // ���Ƴ���
    FPM383_send_instruction(PS_BlueLEDBuffer, 16);

    WAIT_FOR_RESPONSE(wait_time);

    // �Զ���ָ֤��PS_AutoIdentifyBuffer
    FPM383_send_instruction(PS_AutoIdentifyBuffer, 17);
}

u8 FPM383_cancel(void)
{
    if (instruction_code == INST_CODE_AUTO_ENROLL)  //�Զ�ע��ָ����
    {
        printf("ȡ��ָ��ע�����̣�\n");
        FPM383_send_instruction(PS_Cancel, 12);// S_Cancel[12]:ȡ���Զ���֤��ע������
        return 1;
    }
    else if (instruction_code == INST_CODE_AUTO_DIENTIFY)// �Զ�У��ָ����
    {
        printf("ȡ��ָ��ʶ�����̣�\n");
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
    // ��PS_ReadIndexTable������PS_ReadIndexTable_Copy
    memcpy(PS_ReadIndexTable_Copy, PS_ReadIndexTable, 13);
    // ��pageIdд��PS_ReadIndexTable_Copy��10λ
    PS_ReadIndexTable_Copy[10] = pageId;
    // ��У���д�뵽[11, 12]λ
    checksum = get_checksum(PS_ReadIndexTable_Copy, 13);
    PS_ReadIndexTable_Copy[11] = checksum >> 8;
    PS_ReadIndexTable_Copy[12] = checksum & 0xFF;
    // ����ָ��
    FPM383_send_instruction(PS_ReadIndexTable_Copy, 13);//�����
}
void FPM383_delete(u16 id) {
    u16 checksum;
    // ����PS_DeleteChar
    u8 idata PS_DeleteChar_Copy[16];
    memcpy(PS_DeleteChar_Copy, PS_DeleteChar, 16);
    // �޸�[10, 11]ΪҪɾ��λ�õ�ID��
    PS_DeleteChar_Copy[10] = (u8)(id >> 8);
    PS_DeleteChar_Copy[11] = (u8)(id & 0xFF);
    // ����ɾ������0x0001
    PS_DeleteChar_Copy[12] = 0x00;
    PS_DeleteChar_Copy[13] = 0x01;
    // ����У���
    checksum = get_checksum(PS_DeleteChar_Copy, 16);
    PS_DeleteChar_Copy[14] = checksum >> 8;
    PS_DeleteChar_Copy[15] = checksum & 0xFF;

    // ����ָ��
    FPM383_send_instruction(PS_DeleteChar_Copy, 16);//ɾ��ָ��ָ��

}
void FPM383_empty(void) {   // ���ָ�ƿ�
    FPM383_send_instruction(PS_Empty, 12);
}
