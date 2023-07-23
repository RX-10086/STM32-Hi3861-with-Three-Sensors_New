/* Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED */

#ifndef NET_DEMO_COMMON_H
#define NET_DEMO_COMMON_H

void NetDemoTest(unsigned short port, const char *host);

void tcp_server_receive(char *received, int size);
void tcp_server_send(char *sent, int size);

#endif // NET_DEMO_COMMON_H