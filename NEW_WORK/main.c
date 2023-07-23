#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "iot_gpio_ex.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_uart.h"
#include "hi_uart.h"
#include "iot_watchdog.h"
#include "iot_errno.h"
#include "lwip/sockets.h"
#include "net_demo.h"
#include "net_params.h"
#include "wifi_connecter.h"

#define UART_BUFF_SIZE 100
#define U_SLEEP_TIME 50000
const char exp[] = "rec000:00C00%00000lux p:000r:000y:000-00:00:00\r\n";
unsigned char uartReadBuff[49];
unsigned char Buff[8] = {0};
char tcpSendBuff[49] = {0};

void Uart1GpioInit(void) //
{
    IoTGpioInit(IOT_IO_NAME_GPIO_0); // 设置GPIO0的管脚复用关系为UART1_TX
    IoSetFunc(IOT_IO_NAME_GPIO_0, IOT_IO_FUNC_GPIO_0_UART1_TXD);
    IoTGpioInit(IOT_IO_NAME_GPIO_1); // 设置GPIO1的管脚复用关系为UART1_RX
    IoSetFunc(IOT_IO_NAME_GPIO_1, IOT_IO_FUNC_GPIO_1_UART1_RXD);
}
void Uart1Config(void)
{
    uint32_t ret;
    /* 初始化UART配置，波特率 115200，数据bit为8,停止位1，奇偶校验为NONE */
    IotUartAttribute uart_attr = {
        .baudRate = 115200,
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };
    ret = IoTUartInit(HI_UART_IDX_1, &uart_attr);
    if (ret != IOT_SUCCESS)
    {
        printf("Init Uart1 Falied Error No : %d\n", ret);
        return;
    }
}
static void WORKTask(void) // 从STM32串口读取数据并发送至PC
{
    uint32_t len = 0;
    int rel_len = strlen(exp);
    Uart1GpioInit();
    Uart1Config();
    while (1)
    {
        len = IoTUartRead(HI_UART_IDX_1, uartReadBuff, rel_len);
        strcpy(tcpSendBuff, uartReadBuff);
        if (len > 0) // 如果有数据的话
            tcp_server_send(tcpSendBuff, rel_len);
        usleep(U_SLEEP_TIME); // 线程暂停50000us，防止缓冲区出现问题
        // 具体我也没搞明白...
    }
}
static void TCPTask(void) // 从PC读取指令并发送到STM32
{
    int netId = ConnectToHotspot();
    NetDemoTest(PARAM_SERVER_PORT, PARAM_SERVER_ADDR);
    int len_2 = 0;
    while (1)
    {
        tcp_server_receive(Buff, 8);
        IoTUartWrite(HI_UART_IDX_1, (unsigned char *)Buff, strlen(Buff));
    }
}

void WORKEntry(void)
{
    osThreadAttr_t attr1;
    IoTWatchDogDisable();

    attr1.name = "WORKTask";
    attr1.attr_bits = 0U;
    attr1.cb_mem = NULL;
    attr1.cb_size = 0U;
    attr1.stack_mem = NULL;
    attr1.stack_size = 20 * 1024; // 任务栈大小20*1024
    attr1.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)WORKTask, NULL, &attr1) == NULL)
    {
        printf("[WORKTask] Failed to create UartTask!\n");
    }
}

SYS_RUN(WORKEntry);

void TCPEntry(void)
{
    osThreadAttr_t attr2;

    attr2.name = "TCPTask";
    attr2.attr_bits = 0U;
    attr2.cb_mem = NULL;
    attr2.cb_size = 0U;
    attr2.stack_mem = NULL;
    attr2.stack_size = 10 * 1024; // 任务栈大小20*1024
    attr2.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)TCPTask, NULL, &attr2) == NULL)
    {
        printf("[WORKTask] Failed to create UartTask!\n");
    }
}

SYS_RUN(TCPEntry);
