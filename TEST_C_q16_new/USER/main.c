#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "rtc.h"
#include "lcd.h"
#include "lcd_init.h"
#include "dht11.h"
#include "myiic.h"
#include "bh1750.h"
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
#include "spi.h"
#include "w25qxx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"

#define TRUE 1
#define FALSE 0

char TIME_ST[] = "00:00:00";   //显示RTC时间
u8 temp, humi; //温湿度
long int result_lux = 0;   //光照强度
float pitch, roll, yaw; 		//欧拉角 
int p_int, r_int, y_int;    //欧拉角整数部分 
int p_flo, r_flo, y_flo;    //欧拉角小数部分

u8 key_ = -1;  //记录按键状态
int USART_FLAG = TRUE;   //串口开关
int ESP_STABLE = FALSE;  //ESP/Hi状态
u16 chr;  

char TEXT_Buffer[] = "rec000:00C00%00000lux p:000r:000y:000-00:00:00";   //发送的数据（字符串）格式如下
#define SIZE sizeof(TEXT_Buffer)	 //字符串长度
u8 datatemp[SIZE];                 //存储读取FLASH中数据用的区域
u8 databuff[100][SIZE];
u32 FLASH_SIZE = 1024*1024;     //外置FLASH大小，这里测试时用的是W25Q16所以大小只有1M

int FULL_FLAG = FALSE;    //是否存满100条数据
int TEXT_NUM = 0;         //数据编号
#define TEXT_NUM_MAX  100   //最大数据数

int LOOK_UP = FALSE, CHECK_ALL = FALSE;    //发送指令标志位，确定时哪个指令  
int ESP = FALSE; //ESP32/Hi3861是否连接Client

RTC_TimeTypeDef RTC_TimeStruct;
RTC_TimeTypeDef RTC_TimeStruct_2;

#define START_TASK_PRIO		1         //任务优先级
#define START_STK_SIZE 		256       //任务堆栈大小	
TaskHandle_t StartTask_Handler;     //任务句柄
void start_task(void *pvParameters);     //任务函数声明

#define BH1750_TASK_PRIO		2	
#define BH1750_STK_SIZE 		128
TaskHandle_t BH1750Task_Handler;
void bh1750_task(void *pvParameters);

#define MPU6050_TASK_PRIO    5
#define MPU6050_STK_SIZE    1024
TaskHandle_t MPU6050Task_Handler;
void mpu6050_task(void *pvParameters);

#define USART_TRANS_TASK_PRIO   3
#define USART_TRANS_STK_SIZE    768
TaskHandle_t USART_TRANS_Task_Handler;
void usart_trans_task(void *pvParameters);

#define LCD_TASK_PRIO		4	
#define LCD_STK_SIZE 		256
TaskHandle_t LCDTask_Handler;
void lcd_task(void *pvParameters);

#define FLASH_TASK_PRIO		6	        //这里FLASH要写入1s一帧的数据，相对其他任务对实时性要求较高
#define FLASH_STK_SIZE 		512       //所以优先级最高
TaskHandle_t FLASHTask_Handler;
void flash_task(void *pvParameters);

const char *JD1 = "LOOKUP!";       //查询实时状态命令
const char *JD2 = "CHKALL!";       //查询所有历史记录命令
const char *J1 = "ESP_OK!";        //ESP/Hi是否连接Client信号，由ESP/Hi发出

void first_mode(void);
void second_mode(void);
void second_fresh_display(void);
int SECOND_MODE = FALSE;

int p1, y1;
int ra, rb;
int rc = 0, rd = 0;

int COMMAND_JUDGE(char *message, const char *Judge)   //识别指令
{
	int chr_d = 0;
	for(chr_d = 0; chr_d < 6; chr_d++)
	{
		if(*(Judge + chr_d) != *(message + chr_d))
			return FALSE;
	}
	return TRUE;
}

int main(void)
{ 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4
	delay_init(168);		//初始化延时函数
	uart_init(115200);		 //初始化串口波特率为115200
	
	LED_Init();		        //初始化LED端口
	KEY_Init();           //初始化按键
	My_RTC_Init();         //RTC初始化
	LCD_Init();            //LCD初始化
	LCD_Fill(0, 0, LCD_W, LCD_H, DARKBLUE);          //背景色
	IIC_Init();            //IIC初始化（BH1750）
	IIC_Send_Byte(0x01);
	IIC_B_Init();          //IIC初始化（MPU6050）
	mpu_dmp_init();	       //DMP初始化
	W25QXX_Init();         //W25Q16初始化
	while(DHT11_Init())	   //DHT11初始化	
	{
		LCD_ShowString(0,0,"DHT11 ERROR",RED,WHITE,12,0);
		delay_ms(400);                              
	}
	while(W25QXX_ReadID()!=W25Q16)								//检测不到W25Q16
	{
		LCD_ShowString(0,0,"FLASH ERROR",RED,WHITE,12,0);
		delay_ms(500);
		LCD_ShowString(0,0,"PLEASE CHECK!",RED,WHITE,12,0);
		delay_ms(500);
	}
	LCD_DrawLine(0,17,160,17,WHITE);
	LCD_DrawLine(0,34,160,34,WHITE);
	LCD_DrawLine(0,51,160,51,WHITE);
	LCD_DrawLine(0,68,160,68,WHITE);
	LCD_DrawLine(51,0,51,128,WHITE);
	LCD_DrawLine(104,0,104,68,WHITE);
	LCD_DrawLine(0,85,51,85,WHITE);
	LCD_ShowString(0, 0, "TEMP(C)", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(53, 0, "HUMI(%)", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(106, 0, "ILLUM(lx)", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(0, 35, "PITCH", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(53, 35, "ROLL", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(106, 35, "YAW", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(0, 69, "USART", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(0, 86, "ON!", GREEN, DARKBLUE,16,0);
	LCD_ShowChar(31, 52, '.', WHITE, DARKBLUE, 12, 0);
	LCD_ShowChar(84, 52, '.', WHITE, DARKBLUE, 12, 0);
	LCD_ShowChar(139, 52, '.', WHITE, DARKBLUE, 12, 0);
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}
 
//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
    //创建BH1750与DHT11任务
    xTaskCreate((TaskFunction_t )bh1750_task,     	
                (const char*    )"bh1750_task",   	
                (uint16_t       )BH1750_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )BH1750_TASK_PRIO,	
                (TaskHandle_t*  )&BH1750Task_Handler);  
     //创建MPU6050任务
    xTaskCreate((TaskFunction_t )mpu6050_task,     	
                (const char*    )"mpu6050_task",   	
                (uint16_t       )MPU6050_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )MPU6050_TASK_PRIO,	
                (TaskHandle_t*  )&MPU6050Task_Handler);
		//创建串口任务
		xTaskCreate((TaskFunction_t )usart_trans_task,     	
                (const char*    )"usart_trans_task",   	
                (uint16_t       )USART_TRANS_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )USART_TRANS_TASK_PRIO,	
                (TaskHandle_t*  )&USART_TRANS_Task_Handler);					
    //创建LCD任务
    xTaskCreate((TaskFunction_t )lcd_task,     
                (const char*    )"lcd_task",   
                (uint16_t       )LCD_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )LCD_TASK_PRIO,
                (TaskHandle_t*  )&LCDTask_Handler); 
		//创建FLASH写入任务
    xTaskCreate((TaskFunction_t )flash_task,     
                (const char*    )"flash_task",   
                (uint16_t       )FLASH_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )FLASH_TASK_PRIO,
                (TaskHandle_t*  )&FLASHTask_Handler);
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}

//BH1750任务函数 
void bh1750_task(void *pvParameters)
{
    while(TRUE)
    {
			  DHT11_Read_Data(&temp, &humi);		//读取温湿度值
        result_lux = TaskBH1750();   //读取光照强度
        vTaskDelay(250);
    }
}   
//MPU6050任务函数
void mpu6050_task(void *pvParameters)
{
	while(TRUE)
    {
			  mpu_dmp_get_data(&pitch, &roll, &yaw);      //从DMP读取欧拉角
        vTaskDelay(250);
    }
}
//串口任务函数
void usart_trans_task(void *pvParameters)
{
	int NUM = 0;
	u16 t = 0;
	while(TRUE)
    {
			key_ = KEY_Scan(0);            //K0控制串口功能开关
			if(key_ == KEY1_PRES && SECOND_MODE == FALSE)
			{
				SECOND_MODE = TRUE;
				second_mode();
				continue;
			}
			if(key_ == KEY1_PRES && SECOND_MODE == TRUE)
			{
				SECOND_MODE = FALSE;
				first_mode();
				continue;
			}
			if(key_ == KEY0_PRES && USART_FLAG == FALSE)
			{
	       LCD_ShowString(0, 86, "ON!", GREEN, DARKBLUE,16,0);
				 USART_FLAG = TRUE;
				 continue;
			}
			if(key_ == KEY0_PRES && USART_FLAG == TRUE)
			{
				 LCD_ShowString(0, 86, "OFF", RED, DARKBLUE, 16, 0);
				 USART_FLAG = FALSE;
				 continue;
			}
			if(USART_FLAG == TRUE)
			{
			  if(USART_RX_STA&0x8000)
		    {
				  if(ESP_STABLE == FALSE && COMMAND_JUDGE(USART_RX_BUF, J1) == TRUE) 
						//ESP_STABLE == TRUE时就不会再执行逻辑式后面的语句
			    {
				    LCD_ShowString(53, 112, "ESP/Hi OK", YELLOW, DARKBLUE,12,0);
						//显示“ESP/Hi OK”字样，表示ESP/Hi连接PC成功
						ESP_STABLE = TRUE;
			    }
				  if(COMMAND_JUDGE(USART_RX_BUF, JD1) == TRUE)
				  {
						 LCD_ShowString(0, 114, "LOADING", YELLOW, DARKBLUE,12,0);
						//屏幕左下角输出LOADING字样，以告知正在输出数据，下同
						 printf("%s\r\n", TEXT_Buffer);    //STM32自带的简化串口输出函数，下同
						 LCD_ShowString(0, 114, "       ", YELLOW, DARKBLUE,12,0);
			    }
				  if(COMMAND_JUDGE(USART_RX_BUF, JD2) == TRUE)
		      {
						LCD_ShowString(0, 114, "LOADING", YELLOW, DARKBLUE,12,0);
						vTaskSuspend(FLASHTask_Handler);     //暂停FLASH写入
					  if(FULL_FLAG == FALSE)
					  {
					    for(NUM = 1; NUM <= TEXT_NUM; NUM++)
					    {
					       W25QXX_Read(datatemp, FLASH_SIZE - 1000000 + (NUM - 1) * SIZE, SIZE);
								 printf("%s\r\n", datatemp);   
								 vTaskDelay(100);    //延时100ms，下同
					    }
				    }
					  else
					  {
						  for(NUM = 1; NUM <= 100; NUM++)
					    {
					       W25QXX_Read(datatemp, FLASH_SIZE - 1000000 + (NUM - 1) * SIZE, SIZE);
								 printf("%s\r\n", datatemp);
								 vTaskDelay(100);
					    }
					  }
			      vTaskResume(FLASHTask_Handler);      //恢复FLASH写入
						LCD_ShowString(0, 114, "       ", YELLOW, DARKBLUE,12,0);
		      }
			   USART_RX_STA = 0;
		  }
			vTaskDelay(50); //开启串口时的延时
			//出于FreeRTOS的特性，这个延时一定要有
			continue;
		}
		if(SECOND_MODE == TRUE)
		{
			second_fresh_display();
			vTaskDelay(500);
			continue;
		}
		vTaskDelay(250);  //不开启串口时的延时
	}
}
//LCD任务函数
void lcd_task(void *pvParameters)
{
    while(TRUE)
    {
			  RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
			  sprintf((char*)TIME_ST,"%02d:%02d:%02d",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds); 
			  LCD_ShowIntNum(1, 18, temp, 2, WHITE, DARKBLUE, 12);		//显示温度	   		   
			  LCD_ShowIntNum(57, 18, humi, 2, WHITE, DARKBLUE, 12);			//显示湿度
			  LCD_ShowIntNum(110, 18, result_lux, 5, WHITE, DARKBLUE, 12);   //显示光照强度
			  p_int = (int)(pitch * 10) / 10;  
			  r_int = (int)(roll * 10) / 10;
			  y_int = (int)(yaw * 10) / 10;
			  if(p_int < 0) {p_int = - p_int; LCD_ShowChar(0, 52, '-', WHITE, DARKBLUE, 12, 0);} else{LCD_ShowChar(0, 52, '+', WHITE, DARKBLUE, 12, 0);}
			  if(r_int < 0) {r_int = - r_int; LCD_ShowChar(53, 52, '-', WHITE, DARKBLUE, 12, 0);} else{LCD_ShowChar(53, 52, '+', WHITE, DARKBLUE, 12, 0);}
			  if(y_int < 0) {y_int = - y_int; LCD_ShowChar(106, 52, '-', WHITE, DARKBLUE, 12, 0);} else{LCD_ShowChar(106, 52, '+', WHITE, DARKBLUE, 12, 0);}
				p_flo = (int)(pitch * 10) % 10;
				r_flo = (int)(roll * 10) % 10;
				y_flo = (int)(yaw * 10) % 10;
			  LCD_ShowIntNum(12, 52, p_int, 3, WHITE, DARKBLUE, 12);
				LCD_ShowIntNum(38, 52, p_flo, 1, WHITE, DARKBLUE, 12);
			  LCD_ShowIntNum(65, 52, r_int, 3, WHITE, DARKBLUE, 12);
				LCD_ShowIntNum(91, 52, r_flo, 1, WHITE, DARKBLUE, 12);
			  LCD_ShowIntNum(120, 52, y_int, 3, WHITE, DARKBLUE, 12);
				LCD_ShowIntNum(146, 52, y_flo, 1, WHITE, DARKBLUE, 12);
				LCD_ShowString(53, 72, TIME_ST, LIGHTGREEN, DARKBLUE, 24, 0);
				vTaskDelay(500);
    }
}
//FLASH写入任务函数
void flash_task(void *pvParameters)
{
	while(TRUE)
	{
		if(TEXT_NUM == TEXT_NUM_MAX) 
    {
			FULL_FLAG = TRUE; 
		  TEXT_NUM = 0;
		}
		TEXT_NUM++;
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct_2); //再次获取实际时间
		sprintf((char*)TEXT_Buffer, "rec%03d:%02dC%02d%%%05dlux p:%03dr:%03dy:%03d-%02d:%02d:%02d", TEXT_NUM, temp, humi, result_lux, 
			abs(p_int), abs(r_int), abs(y_int), RTC_TimeStruct_2.RTC_Hours, RTC_TimeStruct_2.RTC_Minutes, RTC_TimeStruct_2.RTC_Seconds);
		W25QXX_Write((u8*)TEXT_Buffer, FLASH_SIZE - 1000000 + (TEXT_NUM - 1) * SIZE, SIZE);
		vTaskDelay(1000);
	}
}
void first_mode()
{
	vTaskResume(FLASHTask_Handler);
	vTaskResume(LCDTask_Handler);
	vTaskResume(BH1750Task_Handler);
	LCD_Init();           
	LCD_Fill(0, 0, LCD_W, LCD_H, DARKBLUE);
	LCD_DrawLine(0,17,160,17,WHITE);
	LCD_DrawLine(0,34,160,34,WHITE);
	LCD_DrawLine(0,51,160,51,WHITE);
	LCD_DrawLine(0,68,160,68,WHITE);
	LCD_DrawLine(51,0,51,128,WHITE);
	LCD_DrawLine(104,0,104,68,WHITE);
	LCD_DrawLine(0,85,51,85,WHITE);
	LCD_ShowString(0, 0, "TEMP(C)", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(53, 0, "HUMI(%)", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(106, 0, "ILLUM(lx)", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(0, 35, "PITCH", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(53, 35, "ROLL", WHITE, DARKBLUE, 12, 0);
  LCD_ShowString(106, 35, "YAW", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(0, 69, "USART", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(0, 86, "OFF!", RED, DARKBLUE,16,0);
	LCD_ShowChar(31, 52, '.', WHITE, DARKBLUE, 12, 0);
	LCD_ShowChar(84, 52, '.', WHITE, DARKBLUE, 12, 0);
	LCD_ShowChar(139, 52, '.', WHITE, DARKBLUE, 12, 0);
}
void second_mode()
{
	vTaskSuspend(FLASHTask_Handler);
	vTaskSuspend(LCDTask_Handler);
	vTaskSuspend(BH1750Task_Handler);
	USART_FLAG = FALSE;
	LCD_Init();         
	LCD_Fill(0, 0, LCD_W, LCD_H, DARKBLUE); 
	Draw_Circle(90, 52, 45, WHITE);
	LCD_DrawLine(90, 6, 90, 11, WHITE);
	LCD_DrawLine(44, 55, 49, 52, WHITE);
	LCD_DrawLine(90, 93, 90, 98, WHITE);
	LCD_DrawLine(131, 52, 136, 52, WHITE);
	LCD_DrawRectangle(1, 1, 10, 101, RED);
	LCD_Fill(1, 1, 10, 101, RED);
	LCD_Fill(1, 1, 10, 51, YELLOW);
  LCD_DrawRectangle(1, 110, 141, 120, RED);
	LCD_Fill(1, 110, 141, 120, RED);
	LCD_Fill(1, 110, 71, 120, YELLOW);
	LCD_ShowString(12, 0, "PITCH", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(120, 0, "ROLL", WHITE, DARKBLUE, 12, 0);
	LCD_ShowString(19, 97, "YAW", WHITE, DARKBLUE, 12, 0);
	LCD_DrawLine(10, 51, 20, 51, WHITE);
	LCD_DrawLine(10, 59, 17, 59, WHITE);
	LCD_DrawLine(10, 68, 17, 68, WHITE);
	LCD_DrawLine(10, 76, 20, 76, WHITE);
	LCD_DrawLine(10, 26, 20, 26, WHITE);
	LCD_DrawLine(10, 34, 17, 34, WHITE);
	LCD_DrawLine(10, 43, 17, 43, WHITE);
	LCD_DrawLine(71, 100, 71, 110, WHITE);
	LCD_DrawLine(36, 120, 36, 128, WHITE);
	LCD_DrawLine(48, 120, 48, 128, WHITE);
	LCD_DrawLine(59, 120, 59, 128, WHITE);
	LCD_DrawLine(106, 120, 106, 128, WHITE);
	LCD_DrawLine(94, 120, 94, 128, WHITE);
	LCD_DrawLine(83, 120, 83, 128, WHITE);
}
void second_fresh_display()
{
		p1 = (int)(51 - 50 * (pitch / 90.0));
	  LCD_Fill(1, 1, 10, 101, RED);
	  LCD_Fill(1, 1, 10, p1, YELLOW);
		y1 = (int)(71 - 70 * (yaw / 180.0));
	  LCD_Fill(1, 110, 141, 120, RED);
	  LCD_Fill(1, 110, y1, 120, YELLOW);
	  if(rc != 0 && rd != 0)
		{
			LCD_DrawLine(90, 52, 90 + rd, 52 + rc, DARKBLUE);
	    LCD_DrawLine(90, 52, 90 - rd, 52 - rc, DARKBLUE);
		}
	  ra = (int)(40 * sin(roll / 57.3));
	  rb = (int)(40 * cos(roll / 57.3)); 
	  LCD_DrawLine(90, 52, 90 + rb, 52 + ra, LIGHTGREEN);
	  LCD_DrawLine(90, 52, 90 - rb, 52 - ra, LIGHTGREEN);
	  rc = ra, rd = rb;
}


