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


// 判断是否按下或抬起
#define IS_KEY_UP(row, col)     ((states & (1 << (row * ROW_NUM + col))) > 0)
#define IS_KEY_DOWN(row, col)   ((states & (1 << (row * ROW_NUM + col))) == 0)

// 设置按下或抬起
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

// 根据条件，返回列的引脚宏定义名字
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
    GPIO_InitStructure.Pin  = GPIO_Pin_4 | GPIO_Pin_5;		//指定要初始化的IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//指定IO的输入或输出方式,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P3, &GPIO_InitStructure);//初始化

    GPIO_InitStructure.Pin  = GPIO_Pin_3 | GPIO_Pin_6 | GPIO_Pin_7;		//指定要初始化的IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//指定IO的输入或输出方式,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P0, &GPIO_InitStructure);//初始化

    GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;		//指定要初始化的IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//指定IO的输入或输出方式,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P4, &GPIO_InitStructure);//初始化

    GPIO_InitStructure.Pin  = GPIO_Pin_7;		//指定要初始化的IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//指定IO的输入或输出方式,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P1, &GPIO_InitStructure);//初始化
}

// 初始化
void MK_init() {
    GPIO_config();
}


// 扫描按键
void MK_scan() {
    u8 row, col;

    // 遍历行
    for (row = 0; row < ROW_NUM; row++) {
        // 根据行的索引，指定哪一行置0， 检查这一行的按键
        row_out(row);
        for (col = 0; col < COL_NUM; col++) {
            //(row * 行数) + col = 第几个按键
            if (col_state(col) == DOWN && IS_KEY_UP(row, col)) {  // 现在按下，刚才是抬起的
                // printf("第%d行第%d列按键按下\n", (int)(row+1), (int)(col+1));
                // 按下的回调函数
                MK_on_keydown(row, col);
                
                SET_KEY_DOWN(row, col);
            } else if (col_state(col) == UP && IS_KEY_DOWN(row, col)) {  // 现在抬起，刚才是按下的
                // printf("第%d行第%d列按键抬起\n", (int)(row+1), (int)(col+1));
                // 抬起的回调函数
                MK_on_keyup(row, col);
                
                
                SET_KEY_UP(row, col);
            }
        }
    }
    // *key_states =  states;
}

