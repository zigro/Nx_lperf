/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    app_netxduo.c
 * @author  MCD Application Team
 * @brief   NetXDuo applicative file
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

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
#include "nxd_dhcp_client.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* Define the ThreadX and NetX object control blocks...  */
TX_THREAD AppLinkThread;
TX_SEMAPHORE Semaphore;

NX_PACKET_POOL WebServerPool;

ULONG IpAddress;
ULONG NetMask;

UCHAR *http_stack;
UCHAR *iperf_stack;


static uint8_t nx_server_pool[SERVER_POOL_SIZE];

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD      NxAppThread;
NX_PACKET_POOL NxAppPool;
NX_IP          NetXDuoEthIpInstance;
TX_SEMAPHORE   DHCPSemaphore;
NX_DHCP        DHCPClient;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_app_thread_entry (ULONG thread_input);
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr);
/* USER CODE BEGIN PFP */
/* Define thread prototypes.  */
extern VOID nx_iperf_entry(NX_PACKET_POOL *pool_ptr, NX_IP *ip_ptr, UCHAR* http_stack, ULONG http_stack_size, UCHAR *iperf_stack, ULONG iperf_stack_size);
static VOID App_Link_Thread_Entry(ULONG thread_input);
/* USER CODE END PFP */


/**
 * @brief  Application NetXDuo Initialization.
 * @param memory_ptr: memory pointer
 * @retval int
 */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
	UINT ret = NX_SUCCESS;
	TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

	/* USER CODE BEGIN App_NetXDuo_MEM_POOL */
	printf("Nx_Iperf application started..\n");

	/* USER CODE END App_NetXDuo_MEM_POOL */
	/* USER CODE BEGIN 0 */

	/* USER CODE END 0 */

	/* Initialize the NetXDuo system. */
	CHAR *pointer;
	nx_system_initialize();

	/* packet_pool用メモリ確保 */
	if (TX_SUCCESS != tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT)){
		return TX_POOL_ERROR;
	}
	/* パケット割り当てに使用するパケットプールを作成します。
	 * 余分な NX_PACKET を使用する場合は、NX_APP_PACKET_POOL_SIZE を大きくする必要があります。
	 */
	if (NX_SUCCESS != nx_packet_pool_create(&NxAppPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE)){
		return NX_POOL_ERROR;
	}

	/* Ip_Instance用メモリ確保 */
	if (TX_SUCCESS != tx_byte_allocate(byte_pool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT)){
		return TX_POOL_ERROR;
	}
	/* Create the main NX_IP instance */
	if (NX_SUCCESS != nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK,
			&NxAppPool, nx_stm32_eth_driver,	pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY)){
		return NX_NOT_SUCCESSFUL;
	}

	/* Allocate the memory for ARP */
	if (TX_SUCCESS != tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT)){
		return TX_POOL_ERROR;
	}
	/* Enable the ARP protocol and provide the ARP cache size for the IP instance */
	if (NX_SUCCESS != nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE)){
		return NX_NOT_SUCCESSFUL;
	}

	/* Enable the ICMP */
	if (NX_SUCCESS != nx_icmp_enable(&NetXDuoEthIpInstance)){
		return NX_NOT_SUCCESSFUL;
	}

	/* Enable TCP Protocol */
	if (NX_SUCCESS != nx_tcp_enable(&NetXDuoEthIpInstance)){
		return NX_NOT_SUCCESSFUL;
	}

	/* Enable the UDP protocol required for  DHCP communication */
	if (NX_SUCCESS != nx_udp_enable(&NetXDuoEthIpInstance)){
		return NX_NOT_SUCCESSFUL;
	}

	/* Allocate the memory for main thread   */
	if (TX_SUCCESS != tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT)){
		return TX_POOL_ERROR;
	}
	/* Create the main thread */
	if (TX_SUCCESS != tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_app_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
			NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START)){
		return TX_THREAD_ERROR;
	}

#ifdef DHCP_ENABLED
	/* Create the DHCP client */
	if (NX_SUCCESS != nx_dhcp_create(&DHCPClient, &NetXDuoEthIpInstance, "DHCP Client")){
		return NX_DHCP_ERROR;
	}
#endif

	/* set DHCP notification callback  */
	tx_semaphore_create(&DHCPSemaphore, "DHCP Semaphore", 0);

	/* USER CODE BEGIN MX_NetXDuo_Init */

	/* Allocate the server packet pool. */
	if (TX_SUCCESS != (ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, SERVER_POOL_SIZE, TX_NO_WAIT))){
		/* Check server packet pool memory allocation. */
		printf("Packed pool memory allocation failed : 0x%02x\n", ret);
		Error_Handler();
	}
	/* Create the server packet pool. */
	if (NX_SUCCESS != (ret = nx_packet_pool_create(&WebServerPool, "HTTP Server Packet Pool", SERVER_PACKET_SIZE, nx_server_pool, SERVER_POOL_SIZE))){
		/* Check for server pool creation status. */
		printf("Server pool creation failed : 0x%02x\n", ret);
		Error_Handler();
	}

	/* Set the server stack and IPerf stack.  */
	/* Allocate the server stack. */
	if (TX_SUCCESS != (ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, HTTP_STACK_SIZE, TX_NO_WAIT))){
		/* Check server stack memory allocation. */
		printf("Server stack memory allocation failed : 0x%02x\n", ret);
		Error_Handler();
	}
	http_stack = (UCHAR *)pointer;

	/* Allocate the IPERF stack. */
	if (TX_SUCCESS != (ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, IPERF_STACK_SIZE, TX_NO_WAIT))){
		/* Check IPERF stack memory allocation. */
		printf("IPERF stack memory allocation failed : 0x%02x\n", ret);
		Error_Handler();
	}
	iperf_stack = (UCHAR *)pointer;

	/* Allocate the memory for Link thread   */
	if (TX_SUCCESS != tx_byte_allocate(byte_pool, (VOID **) &pointer,2 *  DEFAULT_MEMORY_SIZE, TX_NO_WAIT)){
		return TX_POOL_ERROR;
	}

	/* create the Link thread */
	if (TX_SUCCESS != tx_thread_create(&AppLinkThread, "App Link Thread", App_Link_Thread_Entry, 0, pointer, 2 * DEFAULT_MEMORY_SIZE,
			LINK_PRIORITY, LINK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START)){
		return NX_NOT_ENABLED;
	}

	// IPAddressを表示
	if (NX_SUCCESS != nx_ip_address_get(&NetXDuoEthIpInstance, &IpAddress, &NetMask)){
		Error_Handler();
	}
	PRINT_IP_ADDRESS(IpAddress);

#if 0
	// 固定IPAddressを設定
	ret = nx_ip_address_set(&NetXDuoEthIpInstance, IP_ADDRESS(172, 16, 51, 81), IP_ADDRESS(255, 255, 0, 0));
	if (ret != NX_SUCCESS)
	{
		Error_Handler();
	}

	// IPAddressを表示
	ret = nx_ip_address_get(&NetXDuoEthIpInstance, &IpAddress, &NetMask);
	if (ret != TX_SUCCESS)
	{
		Error_Handler();
	}
	PRINT_IP_ADDRESS(IpAddress);
	/* USER CODE END MX_NetXDuo_Init */
#endif
#if 1 //#ifdef DHCP_ENABLED
	tx_semaphore_put(&DHCPSemaphore);
#endif
	return ret;
}

#if 1 //#ifdef DHCP_ENABLED
/**
 * @brief  ip address change callback.
 * @param ip_instance: NX_IP instance
 * @param ptr: user data
 * @retval none
 */
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr)
{
	/* USER CODE BEGIN ip_address_change_notify_callback */

	/* USER CODE END ip_address_change_notify_callback */

	/* release the semaphore as soon as an IP address is available */
	tx_semaphore_put(&DHCPSemaphore);
}
#endif

/**
 * @brief  Main thread entry.
 * @param thread_input: ULONG user argument used by the thread entry
 * @retval none
 */
static VOID nx_app_thread_entry (ULONG thread_input)
{
	/* USER CODE BEGIN Nx_App_Thread_Entry 0 */

	/* USER CODE END Nx_App_Thread_Entry 0 */
	UINT ret = NX_SUCCESS;

	/* USER CODE BEGIN Nx_App_Thread_Entry 1 */

	/* USER CODE END Nx_App_Thread_Entry 1 */

	/* register the IP address change callback */
	ret = nx_ip_address_change_notify(&NetXDuoEthIpInstance, ip_address_change_notify_callback, NULL);
	if (ret != NX_SUCCESS)
	{
		/* USER CODE BEGIN IP address change callback error */

		/* Error, call error handler.*/
		Error_Handler();

		/* USER CODE END IP address change callback error */
	}

#ifdef DHCP_ENABLED
	/* start the DHCP client */
	ret = nx_dhcp_start(&DHCPClient);
	if (ret != NX_SUCCESS)
	{
		/* USER CODE BEGIN DHCP client start error */

		/* Error, call error handler.*/
		Error_Handler();

		/* USER CODE END DHCP client start error */
	}
#endif

	/* wait until an IP address is ready */
	if(tx_semaphore_get(&DHCPSemaphore, NX_APP_DEFAULT_TIMEOUT) != TX_SUCCESS)
	{
		/* USER CODE BEGIN DHCPSemaphore get error */

		/* Error, call error handler.*/
		Error_Handler();

		/* USER CODE END DHCPSemaphore get error */
	}

	/* USER CODE BEGIN Nx_App_Thread_Entry 2 */
	ret = nx_ip_address_get(&NetXDuoEthIpInstance, &IpAddress, &NetMask);

	if (ret != TX_SUCCESS)
	{
		Error_Handler();
	}

	PRINT_IP_ADDRESS(IpAddress);

	/* Call entry function to start iperf test.  */
	nx_iperf_entry(&WebServerPool, &NetXDuoEthIpInstance, http_stack, HTTP_STACK_SIZE, iperf_stack, IPERF_STACK_SIZE);
	/* USER CODE END Nx_App_Thread_Entry 2 */
}
/* USER CODE BEGIN 1 */
/**
 * @brief  Link thread entry
 * @param thread_input: ULONG thread parameter
 * @retval none
 */
static VOID App_Link_Thread_Entry(ULONG thread_input)
{
	ULONG actual_status;
	UINT linkdown = 0, status;

	while(1)
	{
		/* Get Physical Link status. */
		status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_LINK_ENABLED,
				&actual_status, 10);

		if(status == NX_SUCCESS)
		{
			if(linkdown == 1)
			{
				linkdown = 0;
				status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_ADDRESS_RESOLVED,
						&actual_status, 10);
				if(status == NX_SUCCESS)
				{
					/* The network cable is connected again. */
					printf("The network cable is connected again.\n");
					/* Print IPERF is available again. */
					printf("IPERF is available again.\n");
				}
				else
				{
					/* The network cable is connected. */
					printf("The network cable is connected.\n");
					/* Send command to Enable Nx driver. */
					nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_ENABLE,
							&actual_status);
#ifdef DHCP_ENABLED
					/* Restart DHCP Client. */
					nx_dhcp_stop(&DHCPClient);
					nx_dhcp_start(&DHCPClient);
#endif
				}
			}
		}
		else
		{
			if(0 == linkdown)
			{
				linkdown = 1;
				/* The network cable is not connected. */
				printf("The network cable is not connected.\n");
			}
		}

		tx_thread_sleep(NX_APP_CABLE_CONNECTION_CHECK_PERIOD);
	}
}

/* USER CODE END 1 */
