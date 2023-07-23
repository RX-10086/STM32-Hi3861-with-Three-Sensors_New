#include "bh1750.h"
#include "myiic.h"
#include "delay.h"
#include "lcd.h"

int result_lx = 0;
u8 BUF[2]={0};
u16 result=0;

void Single_Write_BH1750(unsigned char REG_Address)
{
   IIC_Start();                //��ʼ�ź�
   IIC_Send_Byte(0x46);        //�����豸��ַ+д�ź�
   IIC_Send_Byte(REG_Address);    //�ڲ��Ĵ�����ַ
   IIC_Stop();                 //����ֹͣ�ź�
}

void Cmd_Write_BH1750(char cmd)
{
    IIC_Start();                  
    IIC_Send_Byte(BH1750_Addr+0);   
		while(IIC_Wait_Ack()){};
    IIC_Send_Byte(cmd);    
		while(IIC_Wait_Ack()){};
    IIC_Stop();                   
		delay_ms(5);
}
void Start_BH1750(void)
{
	Cmd_Write_BH1750(BH1750_ON);	  //�ϵ�ָ��
	Cmd_Write_BH1750(BH1750_RSET);	//clear
	Cmd_Write_BH1750(BH1750_ONE);  
}
void Read_BH1750(void)
{   	
	IIC_Start();                          
	IIC_Send_Byte(BH1750_Addr+1);        
	while(IIC_Wait_Ack());
	BUF[0]=IIC_Read_Byte(1);  
	BUF[1]=IIC_Read_Byte(0); 
	IIC_Stop();                       
	delay_ms(5);
}
void Convert_BH1750(void)
{
	result=BUF[0];
	result=(result<<8)+BUF[1];  
	result_lx=result/1.2;
}
int TaskBH1750(void)
{
	 Start_BH1750();     //�ϵ�
   delay_ms(180);      //��ʱ 180ms
   Read_BH1750();      //��ȡ���������� 
   Convert_BH1750();   //����ת����ת��
	 return result_lx;
}	
