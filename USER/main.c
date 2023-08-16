#include "led.h"
#include "key.h"
#include "beep.h"
#include "delay.h"
#include "sys.h"
#include "timer.h"
#include "usart.h"
#include "dht11.h"
#include "bh1750.h"
#include "oled.h"
#include "exti.h"
#include "stdio.h"
#include "esp8266.h"
#include "onenet.h"
#include "AD.h"

u8 handMode_flag = 0;

u8 alarmFlag = 0;//�Ƿ񱨾��ı�־
u8 alarm_is_free = 10;//�������Ƿ��ֶ�������������ֶ�����������Ϊ0


u8 humidityH;	  //ʪ����������
u8 humidityL;	  //ʪ��С������
u8 temperatureH;   //�¶���������
u8 temperatureL;   //�¶�С������

u8 warn_temp = 32;

u8 BeepIO_State  = 1;	//������״̬
u8 LED0_IO_State = 1;			//��

u8 is_open_beep_form_pj = 0;
u8 is_close_beep_form_pj = 0;
u8 is_open_led_form_pj = 0;
u8 is_close_led_form_pj = 0;

extern char oledBuf[20];
float Light = 0; //���ն�
u8 Led_Status = 0;

char PUB_BUF[256];//�ϴ����ݵ�buf
const char *devSubTopic[] = {"/mysmarthome/sub"};
const char devPubTopic[] = "/mysmarthome/pub";
u8 ESP8266_INIT_OK = 0;//esp8266��ʼ����ɱ�־



 int main(void)
 {	
	unsigned short timeCount = 0;	//���ͼ������
	unsigned char *dataPtr = NULL;
	static u8 lineNow;
	Usart1_Init(115200);//debug����
		DEBUG_LOG("\r\n");
		DEBUG_LOG("UART1��ʼ��			[OK]");
	delay_init();	    	 //��ʱ������ʼ��	
		DEBUG_LOG("��ʱ������ʼ��			[OK]");
	 
	 delay_ms(500);// �ӳ�һ�µȴ�oledʶ��
	OLED_Init();
	OLED_ColorTurn(0);//0������ʾ��1 ��ɫ��ʾ
  OLED_DisplayTurn(0);//0������ʾ 1 ��Ļ��ת��ʾ
	OLED_Clear();
	 	DEBUG_LOG("OLED1��ʼ��			[OK]");
		OLED_Refresh_Line("OLED");

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// �����ж����ȼ�����2
		DEBUG_LOG("�ж����ȳ�ʼ��			[OK]");
		OLED_Refresh_Line("NVIC");
	LED_Init();		  	//��ʼ����LED���ӵ�Ӳ���ӿ�
		DEBUG_LOG("LED��ʼ��			[OK]");
		OLED_Refresh_Line("Led");
	KEY_Init();          	//��ʼ���밴�����ӵ�Ӳ���ӿ�
		DEBUG_LOG("������ʼ��			[OK]");
		OLED_Refresh_Line("Key");
	EXTIX_Init();		//�ⲿ�жϳ�ʼ��
		DEBUG_LOG("�ⲿ�жϳ�ʼ��			[OK]");
		OLED_Refresh_Line("EXIT");
	BEEP_Init();
		DEBUG_LOG("��������ʼ��			[OK]");
		OLED_Refresh_Line("Beep");
	DHT11_Init();
		DEBUG_LOG("DHT11��ʼ��			[OK]");
		OLED_Refresh_Line("DHT11");
	AD_Init();
		DEBUG_LOG("ADͨ����ʼ��			[OK]");
		OLED_Refresh_Line("AD Cannel");
	Usart2_Init(115200);//stm32-8266ͨѶ����
		DEBUG_LOG("UART2��ʼ��			[OK]");
		OLED_Refresh_Line("Uart2");
	
		DEBUG_LOG("Ӳ����ʼ��			[OK]");
		
	delay_ms(1000);

	TIM2_Int_Init(4999,7199);
	TIM3_Int_Init(2499,7199);

	DEBUG_LOG("��ʼ��ESP8266 WIFIģ��...");
	if(!ESP8266_INIT_OK){
		OLED_Clear();
		OLED_ShowString(0,0,(u8*)"WiFi",16,1);
		OLED_ShowChinese(32,0,8,16,1);//��
		OLED_ShowChinese(48,0,9,16,1);//��
		OLED_ShowChinese(64,0,10,16,1);//��
		OLED_ShowString(80,0,(u8*)"...",16,1);
		
		OLED_Refresh();
	}
	ESP8266_Init();					//��ʼ��ESP8266
	
	OLED_Clear();
		OLED_ShowChinese(0,0,4,16,1);//��
		OLED_ShowChinese(16,0,5,16,1);//��
		OLED_ShowChinese(32,0,6,16,1);//��
		OLED_ShowChinese(48,0,8,16,1);//��
		OLED_ShowChinese(64,0,9,16,1);//��
		OLED_ShowChinese(80,0,10,16,1);//��
		OLED_ShowString(96,0,(u8*)"...",16,1);
	OLED_Refresh();	
	
	while(OneNet_DevLink()){//����OneNET
		delay_ms(500);
	}		
	
	OLED_Clear();	
	

	
	BEEP = 0;//������ʾ����ɹ�
	delay_ms(250);
	BEEP = 1;
	
	OneNet_Subscribe(devSubTopic, 1);	//esp8266Ҫ���ĵ�����
	
	while(1)
	{
		if(timeCount % 40 == 0)//1000ms / 25 = 40 һ��ִ��һ��
		{
			/********** ��ʪ�ȴ�������ȡ����**************/
			DHT11_Read_Data(&humidityH,&humidityL,&temperatureH,&temperatureL);
			
			/********** ���նȴ�������ȡ����**************/

			Light = 100 - AD_GetValue(ADC_Channel_6)*100/4095;

			
			/********** ��ȡLED0��״̬ **************/
			Led_Status = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4);
			
			/********** �����߼� **************/

			if( !handMode_flag )	//�Զ�ģʽ(��λ������)
			{
				if( temperatureH > warn_temp )	//�¶ȴ���30���϶ȱ���
				{
					BeepIO_State = 0;
				}
				else
					BeepIO_State = 1;
			}
			else		//�ֶ�ģʽ,��λ���ı�handMode_BeepIO��ֵ
			{
				
			}	


//			DEBUG_LOG("alarm_is_free = %d", alarm_is_free);
//			DEBUG_LOG("alarmFlag = %d\r\n", alarmFlag);

			
			/********** ���������Ϣ **************/
			DEBUG_LOG(" | ʪ�ȣ�%d.%d C | �¶ȣ�%d.%d �� | ���նȣ�%.1f lx | ָʾ�ƣ�%s | ��������%s | ",humidityH,humidityL,temperatureH,temperatureL,Light,Led_Status?"�ر�":"��������",alarmFlag?"��������":"ֹͣ");
		}
		if(++timeCount >= 40)	// 5000ms / 25 = 200 ���ͼ��1000ms
		{
			Led_Status = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4);//��ȡLED0��״̬
			DEBUG_LOG("==================================================================================");

			DEBUG_LOG("�������� ----- OneNet_Publish");
			sprintf(PUB_BUF,"{\"Hum\":%d.%d,\"Hand\":%d,\"Temp\":%d.%d,\"Warn\":%d,\"Light\":%.1f,\"Led\":%d,\"Beep\":%d}",
				humidityH,humidityL,handMode_flag,temperatureH,temperatureL,warn_temp,Light,!LED0_IO_State,!BeepIO_State);
			OneNet_Publish(devPubTopic, PUB_BUF);	//esp8266��С���򷢲�������

			DEBUG_LOG("==================================================================================");
			timeCount = 0;
			ESP8266_Clear();
		}
		


		dataPtr = ESP8266_GetIPD(3);	//8266�յ�����Ϣ
		if(dataPtr != NULL)
			OneNet_RevPro(dataPtr);
		
		if(is_open_beep_form_pj)	//�յ���������ָ��
		{
			is_open_beep_form_pj = 0;
			BeepIO_State = 0;
		}
		if(is_close_beep_form_pj)	//�յ��ط�����ָ��
		{
			is_close_beep_form_pj = 0;
			BeepIO_State = 1;
		}

		if(is_open_led_form_pj)	//�յ���LEDָ��
		{
			is_open_led_form_pj = 0;
			LED0_IO_State = 0;
		}

		if(is_close_led_form_pj)	//�յ���LEDָ��
		{
			is_close_led_form_pj = 0;
			LED0_IO_State = 1;
		}

		BEEP = BeepIO_State;
		LED0 = LED0_IO_State;

		delay_ms(10);
	}
}

