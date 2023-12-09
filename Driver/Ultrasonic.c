#include "Ultrasonic.h"


static void GPIO_config(void) {
    // trig ׼˫��
    GPIO_InitTypeDef	GPIO_InitStructure;		//�ṹ����
    GPIO_InitStructure.Pin  = GPIO_Pin_3;		//ָ��Ҫ��ʼ����IO,
    GPIO_InitStructure.Mode = GPIO_PullUp;	//ָ��IO������������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P1, &GPIO_InitStructure);//��ʼ��
    // echo ��������
    GPIO_InitStructure.Pin  = GPIO_Pin_3;		//ָ��Ҫ��ʼ����IO,
    GPIO_InitStructure.Mode = GPIO_HighZ;	//ָ��IO������������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
    GPIO_Inilize(GPIO_P3, &GPIO_InitStructure);//��ʼ��

}

void Ultrasonic_init() {
    GPIO_config();

    PIN_TRIG = 0; // �ȸ�һ���͵�ƽ
}


static void Delay10us(void)	//@24.000MHz
{
    unsigned char data i;

    i = 78;
    while (--i);
}


// ����ֵΪchar����Ϊ�и���������ͬ��״̬
char Ultrasonic_get_distance(float *distance) {
    float dis;
    u16 cnt = 0;
    // 1. ���ٸ�trig 10us�ߵ�ƽʱ�䣬������
    PIN_TRIG = 1;
    Delay10us();
    Delay10us();
    PIN_TRIG = 0;

    // 2. ����echo�͵�ƽ��ʱ�䣬���197us����
    // ��echo��ߵ�ƽʱ���˳�ѭ��
    // ���� ���� 30 * 10 = 300us��û�б�Ϊ�ߵ�ƽ��ҲҪ�˳�ѭ��
    do {
        cnt ++;
        Delay10us();
    } while(PIN_ECHO == 0 && cnt < 30);

    if (cnt >= 30) {
        printf("��ʱ��%d us\n", (int)cnt * 10);
        return -1;
    }
    // 3. ����echo�ߵ�ƽʱ��
    cnt = 0;
    do {
        cnt ++;
        Delay10us();
    } while(PIN_ECHO == 1);
    dis = 34 * cnt * 0.01  * 0.5;
    *distance = dis;
    return 0;
}

















