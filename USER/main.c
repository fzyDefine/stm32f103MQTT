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

u8 alarmFlag = 0;//是否报警的标志
u8 alarm_is_free = 10;//报警器是否被手动操作，如果被手动操作即设置为0


u8 humidityH;	  //湿度整数部分
u8 humidityL;	  //湿度小数部分
u8 temperatureH;   //温度整数部分
u8 temperatureL;   //温度小数部分

u8 warn_temp = 32;

u8 BeepIO_State  = 1;	//蜂鸣器状态
u8 LED0_IO_State = 1;			//灯

u8 is_open_beep_form_pj = 0;
u8 is_close_beep_form_pj = 0;
u8 is_open_led_form_pj = 0;
u8 is_close_led_form_pj = 0;

extern char oledBuf[20];
float Light = 0; //光照度
u8 Led_Status = 0;

char PUB_BUF[256];//上传数据的buf
const char *devSubTopic[] = {"/mysmarthome/sub"};
const char devPubTopic[] = "/mysmarthome/pub";
u8 ESP8266_INIT_OK = 0;//esp8266初始化完成标志



 int main(void)
 {	
	unsigned short timeCount = 0;	//发送间隔变量
	unsigned char *dataPtr = NULL;
	static u8 lineNow;
	Usart1_Init(115200);//debug串口
		DEBUG_LOG("\r\n");
		DEBUG_LOG("UART1初始化			[OK]");
	delay_init();	    	 //延时函数初始化	
		DEBUG_LOG("延时函数初始化			[OK]");
	 
	 delay_ms(500);// 延迟一下等待oled识别
	OLED_Init();
	OLED_ColorTurn(0);//0正常显示，1 反色显示
  OLED_DisplayTurn(0);//0正常显示 1 屏幕翻转显示
	OLED_Clear();
	 	DEBUG_LOG("OLED1初始化			[OK]");
		OLED_Refresh_Line("OLED");

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// 设置中断优先级分组2
		DEBUG_LOG("中断优先初始化			[OK]");
		OLED_Refresh_Line("NVIC");
	LED_Init();		  	//初始化与LED连接的硬件接口
		DEBUG_LOG("LED初始化			[OK]");
		OLED_Refresh_Line("Led");
	KEY_Init();          	//初始化与按键连接的硬件接口
		DEBUG_LOG("按键初始化			[OK]");
		OLED_Refresh_Line("Key");
	EXTIX_Init();		//外部中断初始化
		DEBUG_LOG("外部中断初始化			[OK]");
		OLED_Refresh_Line("EXIT");
	BEEP_Init();
		DEBUG_LOG("蜂鸣器初始化			[OK]");
		OLED_Refresh_Line("Beep");
	DHT11_Init();
		DEBUG_LOG("DHT11初始化			[OK]");
		OLED_Refresh_Line("DHT11");
	AD_Init();
		DEBUG_LOG("AD通道初始化			[OK]");
		OLED_Refresh_Line("AD Cannel");
	Usart2_Init(115200);//stm32-8266通讯串口
		DEBUG_LOG("UART2初始化			[OK]");
		OLED_Refresh_Line("Uart2");
	
		DEBUG_LOG("硬件初始化			[OK]");
		
	delay_ms(1000);

	TIM2_Int_Init(4999,7199);
	TIM3_Int_Init(2499,7199);

	DEBUG_LOG("初始化ESP8266 WIFI模块...");
	if(!ESP8266_INIT_OK){
		OLED_Clear();
		OLED_ShowString(0,0,(u8*)"WiFi",16,1);
		OLED_ShowChinese(32,0,8,16,1);//连
		OLED_ShowChinese(48,0,9,16,1);//接
		OLED_ShowChinese(64,0,10,16,1);//中
		OLED_ShowString(80,0,(u8*)"...",16,1);
		
		OLED_Refresh();
	}
	ESP8266_Init();					//初始化ESP8266
	
	OLED_Clear();
		OLED_ShowChinese(0,0,4,16,1);//服
		OLED_ShowChinese(16,0,5,16,1);//务
		OLED_ShowChinese(32,0,6,16,1);//器
		OLED_ShowChinese(48,0,8,16,1);//连
		OLED_ShowChinese(64,0,9,16,1);//接
		OLED_ShowChinese(80,0,10,16,1);//中
		OLED_ShowString(96,0,(u8*)"...",16,1);
	OLED_Refresh();	
	
	while(OneNet_DevLink()){//接入OneNET
		delay_ms(500);
	}		
	
	OLED_Clear();	
	

	
	BEEP = 0;//鸣叫提示接入成功
	delay_ms(250);
	BEEP = 1;
	
	OneNet_Subscribe(devSubTopic, 1);	//esp8266要订阅的主题
	
	while(1)
	{
		if(timeCount % 40 == 0)//1000ms / 25 = 40 一秒执行一次
		{
			/********** 温湿度传感器获取数据**************/
			DHT11_Read_Data(&humidityH,&humidityL,&temperatureH,&temperatureL);
			
			/********** 光照度传感器获取数据**************/

			Light = 100 - AD_GetValue(ADC_Channel_6)*100/4095;

			
			/********** 读取LED0的状态 **************/
			Led_Status = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4);
			
			/********** 报警逻辑 **************/

			if( !handMode_flag )	//自动模式(上位机不打开)
			{
				if( temperatureH > warn_temp )	//温度大于30摄氏度报警
				{
					BeepIO_State = 0;
				}
				else
					BeepIO_State = 1;
			}
			else		//手动模式,上位机改变handMode_BeepIO的值
			{
				
			}	


//			DEBUG_LOG("alarm_is_free = %d", alarm_is_free);
//			DEBUG_LOG("alarmFlag = %d\r\n", alarmFlag);

			
			/********** 输出调试信息 **************/
			DEBUG_LOG(" | 湿度：%d.%d C | 温度：%d.%d ℃ | 光照度：%.1f lx | 指示灯：%s | 报警器：%s | ",humidityH,humidityL,temperatureH,temperatureL,Light,Led_Status?"关闭":"【启动】",alarmFlag?"【启动】":"停止");
		}
		if(++timeCount >= 40)	// 5000ms / 25 = 200 发送间隔1000ms
		{
			Led_Status = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4);//读取LED0的状态
			DEBUG_LOG("==================================================================================");

			DEBUG_LOG("发布数据 ----- OneNet_Publish");
			sprintf(PUB_BUF,"{\"Hum\":%d.%d,\"Hand\":%d,\"Temp\":%d.%d,\"Warn\":%d,\"Light\":%.1f,\"Led\":%d,\"Beep\":%d}",
				humidityH,humidityL,handMode_flag,temperatureH,temperatureL,warn_temp,Light,!LED0_IO_State,!BeepIO_State);
			OneNet_Publish(devPubTopic, PUB_BUF);	//esp8266向小程序发布的主题

			DEBUG_LOG("==================================================================================");
			timeCount = 0;
			ESP8266_Clear();
		}
		


		dataPtr = ESP8266_GetIPD(3);	//8266收到的信息
		if(dataPtr != NULL)
			OneNet_RevPro(dataPtr);
		
		if(is_open_beep_form_pj)	//收到开蜂鸣器指令
		{
			is_open_beep_form_pj = 0;
			BeepIO_State = 0;
		}
		if(is_close_beep_form_pj)	//收到关蜂鸣器指令
		{
			is_close_beep_form_pj = 0;
			BeepIO_State = 1;
		}

		if(is_open_led_form_pj)	//收到开LED指令
		{
			is_open_led_form_pj = 0;
			LED0_IO_State = 0;
		}

		if(is_close_led_form_pj)	//收到关LED指令
		{
			is_close_led_form_pj = 0;
			LED0_IO_State = 1;
		}

		BEEP = BeepIO_State;
		LED0 = LED0_IO_State;

		delay_ms(10);
	}
}

