#ifndef __MYIIC_B_H
#define __MYIIC_B_H
#include "sys.h" 
   	   		   
//IO方向设置
#define SDA_B_IN()  {GPIOB->MODER&=~(3<<(11*2));GPIOB->MODER|=0<<11*2;}	//PB11输入模式
#define SDA_B_OUT() {GPIOB->MODER&=~(3<<(11*2));GPIOB->MODER|=1<<11*2;} //PB11输出模式
//IO操作函数	 
#define IIC_B_SCL    PBout(10) //SCL
#define IIC_B_SDA    PBout(11) //SDA	 
#define READ_B_SDA   PBin(11)  //输入SDA 

//IIC所有操作函数
void IIC_B_Init(void);                //初始化IIC的IO口				 
void IIC_B_Start(void);				//发送IIC开始信号
void IIC_B_Stop(void);	  			//发送IIC停止信号
void IIC_B_Send_Byte(u8 txd);			//IIC发送一个字节
u8 IIC_B_Read_Byte(unsigned char ack);//IIC读取一个字节
u8 IIC_B_Wait_Ack(void); 				//IIC等待ACK信号
void IIC_B_Ack(void);					//IIC发送ACK信号
void IIC_B_NAck(void);				//IIC不发送ACK信号

void IIC_B_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_B_Read_One_Byte(u8 daddr,u8 addr);	  
#endif
















