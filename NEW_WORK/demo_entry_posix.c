/* Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED */

#include <stdio.h>
#include <stdlib.h>
#include "net_demo.h"
#include "net_params.h"

int main(int argc, char *argv[])
{
    printf("Usage: %s [port] [host]\n", argv[0]);

    short port = argc > 1 ? atoi(argv[1]) : PARAM_SERVER_PORT; /* 1 */
    char *host = argc > 2 ? argv[2] : PARAM_SERVER_ADDR;       /* 2,2 */

    NetDemoTest(port, host);
    return 0;
}