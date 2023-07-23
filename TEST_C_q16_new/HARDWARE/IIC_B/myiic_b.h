#ifndef __MYIIC_B_H
#define __MYIIC_B_H
#include "sys.h" 
   	   		   
//IO��������
#define SDA_B_IN()  {GPIOB->MODER&=~(3<<(11*2));GPIOB->MODER|=0<<11*2;}	//PB11����ģʽ
#define SDA_B_OUT() {GPIOB->MODER&=~(3<<(11*2));GPIOB->MODER|=1<<11*2;} //PB11���ģʽ
//IO��������	 
#define IIC_B_SCL    PBout(10) //SCL
#define IIC_B_SDA    PBout(11) //SDA	 
#define READ_B_SDA   PBin(11)  //����SDA 

//IIC���в�������
void IIC_B_Init(void);                //��ʼ��IIC��IO��				 
void IIC_B_Start(void);				//����IIC��ʼ�ź�
void IIC_B_Stop(void);	  			//����IICֹͣ�ź�
void IIC_B_Send_Byte(u8 txd);			//IIC����һ���ֽ�
u8 IIC_B_Read_Byte(unsigned char ack);//IIC��ȡһ���ֽ�
u8 IIC_B_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
void IIC_B_Ack(void);					//IIC����ACK�ź�
void IIC_B_NAck(void);				//IIC������ACK�ź�

void IIC_B_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_B_Read_One_Byte(u8 daddr,u8 addr);	  
#endif
















