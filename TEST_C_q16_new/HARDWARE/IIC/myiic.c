#include "myiic.h"
#include "delay.h"

//初始化I2C
void IIC_Init(void)
{			
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);    //使能GPIOB时钟
//  																SCL					SDA
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      //普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       //上拉
  GPIO_Init(GPIOB, &GPIO_InitStructure);             //初始化
	IIC_SCL=1;
	IIC_SDA=1;
}

void IIC_Start(void)
{
	SDA_OUT();     //SDA模式
	IIC_SDA=1;	     
	IIC_SCL=1;
	delay_us(4);     //为了通信稳定，一般在跳变中途延时4ms
 	IIC_SDA=0;       //下降沿
	delay_us(4);
	IIC_SCL=0;       //钳住I2C总线，准备发送或接收数据 
}	  

void IIC_Stop(void)  
{
	SDA_OUT();     //SDA模式
	IIC_SCL=0;
	IIC_SDA=0;
 	delay_us(4);
	IIC_SCL=1; 
	IIC_SDA=1;     //发送I2C总线结束信号
	delay_us(4);							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	SDA_IN();       //SDA设置为输入  
	IIC_SDA=1;delay_us(1);	   
	IIC_SCL=1;delay_us(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL=0;	   //时钟输出0 	  
	return 0;  
} 
//产生ACK应答
void IIC_Ack(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=0;
	delay_us(2);
	IIC_SCL=1;
	delay_us(2);
	IIC_SCL=0;
}
//不产生ACK应答		     
void IIC_NAck(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=1;
	delay_us(2);
	IIC_SCL=1;
	delay_us(2);
	IIC_SCL=0;
}					 				     
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答				  
void IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	  SDA_OUT(); 	    
    IIC_SCL=0;           //拉低时钟开始数据传输
    for(t=0;t<8;t++)     //8位计数器
    {              
        IIC_SDA=(txd&0x80)>>7;
        txd<<=1; 	  
		    delay_us(2);   
		    IIC_SCL=1;     //拉高时钟线
		    delay_us(2); 
		    IIC_SCL=0;	   //拉低时钟线
		    delay_us(2);
    }	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();       //SDA设置为输入
  for(i=0;i<8;i++ )
	{
      IIC_SCL=0; 
      delay_us(2);
		  IIC_SCL=1;
      receive<<=1;
      if(READ_SDA) receive++;   
		  delay_us(1); 
  }					 
  if(!ack)
     IIC_NAck();  //发送nACK
  else
     IIC_Ack();   //发送ACK    
  return receive;
}



























