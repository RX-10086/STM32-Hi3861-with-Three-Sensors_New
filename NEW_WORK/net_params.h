/* Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED */

#ifndef NET_PARAMS_H
#define NET_PARAMS_H

#ifndef PARAM_HOTSPOT_SSID
#define PARAM_HOTSPOT_SSID "Wintoki" // your AP SSID
#endif

#ifndef PARAM_HOTSPOT_PSK
#define PARAM_HOTSPOT_PSK "1234567890" // your AP PSK
#endif

#ifndef PARAM_HOTSPOT_TYPE
#define PARAM_HOTSPOT_TYPE WIFI_SEC_TYPE_PSK // defined in wifi_device_config.h
#endif

#ifndef PARAM_SERVER_ADDR
#define PARAM_SERVER_ADDR "192.168.127.1" // your PC IP address
#endif

#ifndef PARAM_SERVER_PORT
#define PARAM_SERVER_PORT 8080
#endif

#endif // NET_PARAMS_H