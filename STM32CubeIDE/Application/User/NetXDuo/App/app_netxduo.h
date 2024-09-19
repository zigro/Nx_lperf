/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.h
  * @author  MCD Application Team
  * @brief   NetXDuo applicative header file
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_NETXDUO_H__
#define __APP_NETXDUO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "nx_api.h"

/* Private includes ----------------------------------------------------------*/
#include "nx_stm32_eth_driver.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "main.h"
#include "nx_web_http_server.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
typedef struct TCP_TASK_Struct {
	TX_THREAD		Thread;
//	CHAR*			Name;
	UCHAR 			*StackSpace;
	ULONG 			StackSize;	// = NX_APP_THREAD_STACK_SIZE;
//	UINT			Priority;

	UINT volatile	Active;
	UINT 			Port;				//
	ULONG 			WindowSize;
	NXD_ADDRESS		RemoteIP;
	UINT			RemotePort;
	ULONG			QueueMax;			// QUEUE_MAX_SIZE
	UINT 			(*RecieveCallback)(struct TCP_TASK_Struct* this, const UCHAR *buffer, UINT buffer_length);
	void 			(*CleanUp)(struct TCP_TASK_Struct* this);
	UINT 			(*SendPacket)(struct TCP_TASK_Struct* this, const UCHAR *buffer, UINT buffer_length);
	UINT 			(*SendMessage)(struct TCP_TASK_Struct* this, const UCHAR *buffer, UINT buffer_length);
	TX_THREAD		*SendThread;
	TX_SEMAPHORE	TxSemaphore;
	UINT			SendWaitTicks;	// 送信待機Ticks NX_NO_WAIT(0)を指定すると即時送信する
	NX_TCP_SOCKET	Socket;	// TcpSocket
} TCP_TASK;

/* External variables ---------------------------------------------------------*/
extern NX_IP					NetXDuoEthIpInstance;
extern TX_BYTE_POOL 			*TxBytePool;
extern NX_PACKET_POOL			NxPacketPool;

/* Exported constants --------------------------------------------------------*/
#define PERIODIC_MSEC(msec)		(ULONG)((msec)*NX_IP_PERIODIC_RATE/1000)
#define SEND_TIMEOUT			PERIODIC_MSEC(3000)
#define RECIEVE_TIMEOUT			PERIODIC_MSEC(1000)
#define DISCONNECT_WAIT			PERIODIC_MSEC(5000)

/* The DEFAULT_PAYLOAD_SIZE should match with RxBuffLen configured via MX_ETH_Init */
#ifndef DEFAULT_PAYLOAD_SIZE
#define DEFAULT_PAYLOAD_SIZE      1536
#endif

#ifndef DEFAULT_ARP_CACHE_SIZE
#define DEFAULT_ARP_CACHE_SIZE    1024
#endif

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define PRINT_IP_ADDRESS(addr)             do { \
                                                printf("STM32 %s: %lu.%lu.%lu.%lu \n", #addr, \
                                                (addr >> 24) & 0xff, \
                                                (addr >> 16) & 0xff, \
                                                (addr >> 8) & 0xff, \
                                                addr& 0xff);\
                                           }while(0)

#define PRINT_IP_ADDRESS_PORT(addr,port)  do { \
                                                printf("STM32 %s: %lu.%lu.%lu.%lu : %u\n", #addr, \
                                                (addr >> 24) & 0xff, \
                                                (addr >> 16) & 0xff, \
                                                (addr >> 8) & 0xff, \
                                                addr& 0xff, port);\
                                           }while(0)

#define PRINT_DATA(addr, port, data)       do { \
                                                printf("[%lu.%lu.%lu.%lu:%u] -> '%s' \n", \
                                                (addr >> 24) & 0xff, \
                                                (addr >> 16) & 0xff, \
                                                (addr >> 8) & 0xff,  \
                                                (addr & 0xff), port, data); \
                                           } while(0)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT MX_NetXDuo_Init(VOID *memory_ptr);

/* USER CODE BEGIN EFP */
UINT TcpServerThread_Create(TCP_TASK* this, UINT port, CHAR* name, UINT tx_start, UINT priority);
UINT TcpClientThread_Create(TCP_TASK* this, ULONG remote_ip_address, UINT remote_port,
		CHAR* name, UINT tx_start, UINT priority);
VOID TcpThread_Resume(TCP_TASK *this);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define 				ASI_PACKET_SIZE				(256 + 7)
#define 				ASI_PACKET_MSS				(((ASI_PACKET_SIZE + 1)>>1)<<1)
#define 				ASI_PACKET_PAYLOADSIZE		(ASI_PACKET_MSS + 80)




#define                 PACKET_PAYLOAD_SIZE        1536
#define                 NX_PACKET_POOL_SIZE        ((PACKET_PAYLOAD_SIZE + sizeof(NX_PACKET)) * 40)
#define                 DEMO_STACK_SIZE            2048
#define                 HTTP_STACK_SIZE            2048
#define                 APPLY_STACK_SIZE           4096


#define                 PAYLOAD_SIZE               1536 + sizeof(NX_PACKET)

#define                 DEFAULT_MEMORY_SIZE        1024
#define                 DEFAULT_PRIORITY           10
#define LINK_PRIORITY                              11

#define                 NULL_ADDRESS               0

#define SERVER_PACKET_SIZE                        (1024 * 2)
#define SERVER_POOL_SIZE                          (SERVER_PACKET_SIZE * 8)

/* Server pool size */

#define NX_APP_INSTANCE_PRIORITY                  1

#define NX_APP_CABLE_CONNECTION_CHECK_PERIOD      (6 * NX_IP_PERIODIC_RATE)
/* USER CODE END PD */

#define NX_APP_DEFAULT_TIMEOUT               (10 * NX_IP_PERIODIC_RATE)

#define NX_APP_PACKET_POOL_SIZE              ((DEFAULT_PAYLOAD_SIZE + sizeof(NX_PACKET)) * 40)

#define NX_APP_THREAD_STACK_SIZE             2 * 1024

#define Nx_IP_INSTANCE_THREAD_SIZE           2 * 1024

#define NX_APP_THREAD_PRIORITY               10

#ifndef NX_APP_INSTANCE_PRIORITY
#define NX_APP_INSTANCE_PRIORITY             NX_APP_THREAD_PRIORITY
#endif

#define NX_APP_DEFAULT_IP_ADDRESS                   IP_ADDRESS(172, 16, 51, 80)

#define NX_APP_DEFAULT_NET_MASK                     IP_ADDRESS(255, 255, 0, 0)

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_NETXDUO_H__ */
