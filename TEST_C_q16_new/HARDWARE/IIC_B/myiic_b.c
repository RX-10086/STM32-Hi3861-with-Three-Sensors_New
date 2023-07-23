#include "myiic_b.h"
#include "delay.h"

//��ʼ��IIC_B
void IIC_B_Init(void)
{			
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ��GPIOBʱ��

  //GPIOB8,B9��ʼ������
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��
	IIC_B_SCL=1;
	IIC_B_SDA=1;
}
//����IIC��ʼ�ź�
void IIC_B_Start(void)
{
	SDA_B_OUT();     //sda�����
	IIC_B_SDA=1;	  	  
	IIC_B_SCL=1;
	delay_us(4);
 	IIC_B_SDA=0;//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	IIC_B_SCL=0;//ǯסI2C���ߣ�׼�����ͻ�������� 
}	  
//����IICֹͣ�ź�
void IIC_B_Stop(void)
{
	SDA_B_OUT();//sda�����
	IIC_B_SCL=0;
	IIC_B_SDA=0;//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_B_SCL=1; 
	IIC_B_SDA=1;//����I2C���߽����ź�
	delay_us(4);							   	
}
//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
u8 IIC_B_Wait_Ack(void)
{
	u8 ucErrTime=0;
	SDA_B_IN();      //SDA����Ϊ����  
	IIC_B_SDA=1;delay_us(1);	   
	IIC_B_SCL=1;delay_us(1);	 
	while(READ_B_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_B_Stop();
			return 1;
		}
	}
	IIC_B_SCL=0;//ʱ�����0 	   
	return 0;  
} 
//����ACKӦ��
void IIC_B_Ack(void)
{
	IIC_B_SCL=0;
	SDA_B_OUT();
	IIC_B_SDA=0;
	delay_us(2);
	IIC_B_SCL=1;
	delay_us(2);
	IIC_B_SCL=0;
}
//������ACKӦ��		    
void IIC_B_NAck(void)
{
	IIC_B_SCL=0;
	SDA_B_OUT();
	IIC_B_SDA=1;
	delay_us(2);
	IIC_B_SCL=1;
	delay_us(2);
	IIC_B_SCL=0;
}					 				     
//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��			  
void IIC_B_Send_Byte(u8 txd)
{                        
    u8 t;   
	  SDA_B_OUT(); 	    
    IIC_B_SCL=0;//����ʱ�ӿ�ʼ���ݴ���
    for(t=0;t<8;t++)
    {              
        IIC_B_SDA=(txd&0x80)>>7;
        txd<<=1; 	  
		delay_us(2);   //��TEA5767��������ʱ���Ǳ����
		IIC_B_SCL=1;
		delay_us(2); 
		IIC_B_SCL=0;	
		delay_us(2);
    }	 
} 	    
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
u8 IIC_B_Read_Byte(unsigned char ack)
{
	 unsigned char i,receive=0;
	 SDA_B_IN();//SDA����Ϊ����
    for(i=0;i<8;i++ )
	  {
        IIC_B_SCL=0; 
        delay_us(2);
		    IIC_B_SCL=1;
        receive<<=1;
        if(READ_B_SDA)receive++;   
		delay_us(1); 
    }					 
    if (!ack)
        IIC_B_NAck();//����nACK
    else
        IIC_B_Ack(); //����ACK   
    return receive;
}



























