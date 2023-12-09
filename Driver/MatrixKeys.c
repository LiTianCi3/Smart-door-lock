#include "MatrixKeys.h"
#include "GPIO.h"

#define  COL1   P03
#define  COL2   P06
#define  COL3   P07
#define  COL4   P17

#define  ROW1   P34
#define  ROW2   P35
#define  ROW3   P40
#define  ROW4   P41

#define     DOWN        0
#define     UP          1
#define     ROW_NUM     4
#define     COL_NUM     4

u16 states = 0xffff;


// �ж��Ƿ��»�̧��
#define IS_KEY_UP(row, col)     ((states & (1 << (row * ROW_NUM + col))) > 0)
#define IS_KEY_DOWN(row, col)   ((states & (1 << (row * ROW_NUM + col))) == 0)

// ���ð��»�̧��
#define SET_KEY_UP(row, col)     (states |= (1 << (row * ROW_NUM + col)))
#define SET_KEY_DOWN(row, col)   (states &= ~(1 << (row * ROW_NUM + col)))



void row_out(u8 row) {
    COL1 = 1;
    COL2 = 1;
    COL3 = 1;
    COL4 = 1;

    ROW1 = (row == 0 ? 0 : 1);
    ROW2 = (row == 1 ? 0 : 1);
    ROW3 = (row == 2 ? 0 : 1);
    ROW4 = (row == 3 ? 0 : 1);
}

// ���������������е����ź궨������
u8 col_state(u8 col) {
    if (col == 0) return COL1;
    if (col == 1) return COL2;
    if (col == 2) return COL3;
    if (col == 3) return COL4;


    return 0;
}

static void GPIO_config() {
    GPIO_InitTypeDef	GPIO_InitStructure;	
    // P34 P35
    GPIO_InitStructure.Pin  = GPIO_Pin_4 | GPIO_Pin_5;		//ָ��Ҫ��ʼ����IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//ָ��IO������������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P3, &GPIO_InitStructure);//��ʼ��

    GPIO_InitStructure.Pin  = GPIO_Pin_3 | GPIO_Pin_6 | GPIO_Pin_7;		//ָ��Ҫ��ʼ����IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//ָ��IO������������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P0, &GPIO_InitStructure);//��ʼ��

    GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;		//ָ��Ҫ��ʼ����IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//ָ��IO������������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P4, &GPIO_InitStructure);//��ʼ��

    GPIO_InitStructure.Pin  = GPIO_Pin_7;		//ָ��Ҫ��ʼ����IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//ָ��IO������������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P1, &GPIO_InitStructure);//��ʼ��
}

// ��ʼ��
void MK_init() {
    GPIO_config();
}


// ɨ�谴��
void MK_scan() {
    u8 row, col;

    // ������
    for (row = 0; row < ROW_NUM; row++) {
        // �����е�������ָ����һ����0�� �����һ�еİ���
        row_out(row);
        for (col = 0; col < COL_NUM; col++) {
            //(row * ����) + col = �ڼ�������
            if (col_state(col) == DOWN && IS_KEY_UP(row, col)) {  // ���ڰ��£��ղ���̧���
                // printf("��%d�е�%d�а�������\n", (int)(row+1), (int)(col+1));
                // ���µĻص�����
                MK_on_keydown(row, col);
                
                SET_KEY_DOWN(row, col);
            } else if (col_state(col) == UP && IS_KEY_DOWN(row, col)) {  // ����̧�𣬸ղ��ǰ��µ�
                // printf("��%d�е�%d�а���̧��\n", (int)(row+1), (int)(col+1));
                // ̧��Ļص�����
                MK_on_keyup(row, col);
                
                
                SET_KEY_UP(row, col);
            }
        }
    }
    // *key_states =  states;
}

