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

char TIME_ST[] = "00:00:00";   //��ʾRTCʱ��
u8 temp, humi; //��ʪ��
long int result_lux = 0;   //����ǿ��
float pitch, roll, yaw; 		//ŷ���� 
int p_int, r_int, y_int;    //ŷ������������ 
int p_flo, r_flo, y_flo;    //ŷ����С������

u8 key_ = -1;  //��¼����״̬
int USART_FLAG = TRUE;   //���ڿ���
int ESP_STABLE = FALSE;  //ESP/Hi״̬
u16 chr;  

char TEXT_Buffer[] = "rec000:00C00%00000lux p:000r:000y:000-00:00:00";   //���͵����ݣ��ַ�������ʽ����
#define SIZE sizeof(TEXT_Buffer)	 //�ַ�������
u8 datatemp[SIZE];                 //�洢��ȡFLASH�������õ�����
u8 databuff[100][SIZE];
u32 FLASH_SIZE = 1024*1024;     //����FLASH��С���������ʱ�õ���W25Q16���Դ�Сֻ��1M

int FULL_FLAG = FALSE;    //�Ƿ����100������
int TEXT_NUM = 0;         //���ݱ��
#define TEXT_NUM_MAX  100   //���������

int LOOK_UP = FALSE, CHECK_ALL = FALSE;    //����ָ���־λ��ȷ��ʱ�ĸ�ָ��  
int ESP = FALSE; //ESP32/Hi3861�Ƿ�����Client

RTC_TimeTypeDef RTC_TimeStruct;
RTC_TimeTypeDef RTC_TimeStruct_2;

#define START_TASK_PRIO		1         //�������ȼ�
#define START_STK_SIZE 		256       //�����ջ��С	
TaskHandle_t StartTask_Handler;     //������
void start_task(void *pvParameters);     //����������

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

#define FLASH_TASK_PRIO		6	        //����FLASHҪд��1sһ֡�����ݣ�������������ʵʱ��Ҫ��ϸ�
#define FLASH_STK_SIZE 		512       //�������ȼ����
TaskHandle_t FLASHTask_Handler;
void flash_task(void *pvParameters);

const char *JD1 = "LOOKUP!";       //��ѯʵʱ״̬����
const char *JD2 = "CHKALL!";       //��ѯ������ʷ��¼����
const char *J1 = "ESP_OK!";        //ESP/Hi�Ƿ�����Client�źţ���ESP/Hi����

void first_mode(void);
void second_mode(void);
void second_fresh_display(void);
int SECOND_MODE = FALSE;

int p1, y1;
int ra, rb;
int rc = 0, rd = 0;

int COMMAND_JUDGE(char *message, const char *Judge)   //ʶ��ָ��
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
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
	delay_init(168);		//��ʼ����ʱ����
	uart_init(115200);		 //��ʼ�����ڲ�����Ϊ115200
	
	LED_Init();		        //��ʼ��LED�˿�
	KEY_Init();           //��ʼ������
	My_RTC_Init();         //RTC��ʼ��
	LCD_Init();            //LCD��ʼ��
	LCD_Fill(0, 0, LCD_W, LCD_H, DARKBLUE);          //����ɫ
	IIC_Init();            //IIC��ʼ����BH1750��
	IIC_Send_Byte(0x01);
	IIC_B_Init();          //IIC��ʼ����MPU6050��
	mpu_dmp_init();	       //DMP��ʼ��
	W25QXX_Init();         //W25Q16��ʼ��
	while(DHT11_Init())	   //DHT11��ʼ��	
	{
		LCD_ShowString(0,0,"DHT11 ERROR",RED,WHITE,12,0);
		delay_ms(400);                              
	}
	while(W25QXX_ReadID()!=W25Q16)								//��ⲻ��W25Q16
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
	//������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&StartTask_Handler);   //������              
    vTaskStartScheduler();          //�����������
}
 
//��ʼ����������
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //�����ٽ���
    //����BH1750��DHT11����
    xTaskCreate((TaskFunction_t )bh1750_task,     	
                (const char*    )"bh1750_task",   	
                (uint16_t       )BH1750_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )BH1750_TASK_PRIO,	
                (TaskHandle_t*  )&BH1750Task_Handler);  
     //����MPU6050����
    xTaskCreate((TaskFunction_t )mpu6050_task,     	
                (const char*    )"mpu6050_task",   	
                (uint16_t       )MPU6050_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )MPU6050_TASK_PRIO,	
                (TaskHandle_t*  )&MPU6050Task_Handler);
		//������������
		xTaskCreate((TaskFunction_t )usart_trans_task,     	
                (const char*    )"usart_trans_task",   	
                (uint16_t       )USART_TRANS_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )USART_TRANS_TASK_PRIO,	
                (TaskHandle_t*  )&USART_TRANS_Task_Handler);					
    //����LCD����
    xTaskCreate((TaskFunction_t )lcd_task,     
                (const char*    )"lcd_task",   
                (uint16_t       )LCD_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )LCD_TASK_PRIO,
                (TaskHandle_t*  )&LCDTask_Handler); 
		//����FLASHд������
    xTaskCreate((TaskFunction_t )flash_task,     
                (const char*    )"flash_task",   
                (uint16_t       )FLASH_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )FLASH_TASK_PRIO,
                (TaskHandle_t*  )&FLASHTask_Handler);
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}

//BH1750������ 
void bh1750_task(void *pvParameters)
{
    while(TRUE)
    {
			  DHT11_Read_Data(&temp, &humi);		//��ȡ��ʪ��ֵ
        result_lux = TaskBH1750();   //��ȡ����ǿ��
        vTaskDelay(250);
    }
}   
//MPU6050������
void mpu6050_task(void *pvParameters)
{
	while(TRUE)
    {
			  mpu_dmp_get_data(&pitch, &roll, &yaw);      //��DMP��ȡŷ����
        vTaskDelay(250);
    }
}
//����������
void usart_trans_task(void *pvParameters)
{
	int NUM = 0;
	u16 t = 0;
	while(TRUE)
    {
			key_ = KEY_Scan(0);            //K0���ƴ��ڹ��ܿ���
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
						//ESP_STABLE == TRUEʱ�Ͳ�����ִ���߼�ʽ��������
			    {
				    LCD_ShowString(53, 112, "ESP/Hi OK", YELLOW, DARKBLUE,12,0);
						//��ʾ��ESP/Hi OK����������ʾESP/Hi����PC�ɹ�
						ESP_STABLE = TRUE;
			    }
				  if(COMMAND_JUDGE(USART_RX_BUF, JD1) == TRUE)
				  {
						 LCD_ShowString(0, 114, "LOADING", YELLOW, DARKBLUE,12,0);
						//��Ļ���½����LOADING�������Ը�֪����������ݣ���ͬ
						 printf("%s\r\n", TEXT_Buffer);    //STM32�Դ��ļ򻯴��������������ͬ
						 LCD_ShowString(0, 114, "       ", YELLOW, DARKBLUE,12,0);
			    }
				  if(COMMAND_JUDGE(USART_RX_BUF, JD2) == TRUE)
		      {
						LCD_ShowString(0, 114, "LOADING", YELLOW, DARKBLUE,12,0);
						vTaskSuspend(FLASHTask_Handler);     //��ͣFLASHд��
					  if(FULL_FLAG == FALSE)
					  {
					    for(NUM = 1; NUM <= TEXT_NUM; NUM++)
					    {
					       W25QXX_Read(datatemp, FLASH_SIZE - 1000000 + (NUM - 1) * SIZE, SIZE);
								 printf("%s\r\n", datatemp);   
								 vTaskDelay(100);    //��ʱ100ms����ͬ
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
			      vTaskResume(FLASHTask_Handler);      //�ָ�FLASHд��
						LCD_ShowString(0, 114, "       ", YELLOW, DARKBLUE,12,0);
		      }
			   USART_RX_STA = 0;
		  }
			vTaskDelay(50); //��������ʱ����ʱ
			//����FreeRTOS�����ԣ������ʱһ��Ҫ��
			continue;
		}
		if(SECOND_MODE == TRUE)
		{
			second_fresh_display();
			vTaskDelay(500);
			continue;
		}
		vTaskDelay(250);  //����������ʱ����ʱ
	}
}
//LCD������
void lcd_task(void *pvParameters)
{
    while(TRUE)
    {
			  RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
			  sprintf((char*)TIME_ST,"%02d:%02d:%02d",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds); 
			  LCD_ShowIntNum(1, 18, temp, 2, WHITE, DARKBLUE, 12);		//��ʾ�¶�	   		   
			  LCD_ShowIntNum(57, 18, humi, 2, WHITE, DARKBLUE, 12);			//��ʾʪ��
			  LCD_ShowIntNum(110, 18, result_lux, 5, WHITE, DARKBLUE, 12);   //��ʾ����ǿ��
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
//FLASHд��������
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
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct_2); //�ٴλ�ȡʵ��ʱ��
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


