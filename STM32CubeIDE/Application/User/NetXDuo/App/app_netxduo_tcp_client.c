/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

TX_THREAD AppTCPThread;
TX_THREAD AppLinkThread;

ULONG IpAddress;
ULONG NetMask;

NX_TCP_SOCKET TCPSocket;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_app_thread_entry (ULONG thread_input);
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr);
/* USER CODE BEGIN PFP */
static VOID App_TCP_Thread_Entry(ULONG thread_input);
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
  CHAR *pointer;

  /* USER CODE BEGIN MX_NetXDuo_MEM_POOL */

  /* USER CODE END MX_NetXDuo_MEM_POOL */

  /* USER CODE BEGIN 0 */
  printf("Nx_TCP_Echo_Client application started..\n");
  /* USER CODE END 0 */

  /* Initialize the NetXDuo system. */
  nx_system_initialize();

    /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the Packet pool to be used for packet allocation,
   * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
   */
  ret = nx_packet_pool_create(&NxAppPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_POOL_ERROR;
  }

    /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

   /* Create the main NX_IP instance */
  ret = nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK, &NxAppPool, nx_stm32_eth_driver,
                     pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

    /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */

  /* USER CODE BEGIN ARP_Protocol_Initialization */

  /* USER CODE END ARP_Protocol_Initialization */

  ret = nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the ICMP */

  /* USER CODE BEGIN ICMP_Protocol_Initialization */

  /* USER CODE END ICMP_Protocol_Initialization */

  ret = nx_icmp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable TCP Protocol */

  /* USER CODE BEGIN TCP_Protocol_Initialization */
  /* Allocate the memory for TCP server thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,2 *  DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* create the TCP server thread */
  ret = tx_thread_create(&AppTCPThread, "App TCP Thread", App_TCP_Thread_Entry, 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);

  if (ret != TX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }
  /* USER CODE END TCP_Protocol_Initialization */

  ret = nx_tcp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the UDP protocol required for  DHCP communication */

  /* USER CODE BEGIN UDP_Protocol_Initialization */

  /* USER CODE END UDP_Protocol_Initialization */

  ret = nx_udp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

   /* Allocate the memory for main thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the main thread */
  ret = tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_app_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* Create the DHCP client */

  /* USER CODE BEGIN DHCP_Protocol_Initialization */

  /* USER CODE END DHCP_Protocol_Initialization */

  ret = nx_dhcp_create(&DHCPClient, &NetXDuoEthIpInstance, "DHCP Client");

  if (ret != NX_SUCCESS)
  {
    return NX_DHCP_ERROR;
  }

  /* set DHCP notification callback  */
  tx_semaphore_create(&DHCPSemaphore, "DHCP Semaphore", 0);

  /* USER CODE BEGIN MX_NetXDuo_Init */
  /* Allocate the memory for Link thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* create the Link thread */
  ret = tx_thread_create(&AppLinkThread, "App Link Thread", App_Link_Thread_Entry, 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         LINK_PRIORITY, LINK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }

  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

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
    Error_Handler();
    /* USER CODE END IP address change callback error */
  }

  /* start the DHCP client */
  ret = nx_dhcp_start(&DHCPClient);
  if (ret != NX_SUCCESS)
  {
    /* USER CODE BEGIN DHCP client start error */
    Error_Handler();
    /* USER CODE END DHCP client start error */
  }

  /* wait until an IP address is ready */
  if(tx_semaphore_get(&DHCPSemaphore, NX_APP_DEFAULT_TIMEOUT) != TX_SUCCESS)
  {
    /* USER CODE BEGIN DHCPSemaphore get error */
    Error_Handler();
    /* USER CODE END DHCPSemaphore get error */
  }

  /* USER CODE BEGIN Nx_App_Thread_Entry 2 */
  /* get IP address */
  ret = nx_ip_address_get(&NetXDuoEthIpInstance, &IpAddress, &NetMask);

  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }

  PRINT_IP_ADDRESS(IpAddress);

  /* the network is correctly initialized, start the TCP server thread */
  tx_thread_resume(&AppTCPThread);

  /* if this thread is not needed any more, we relinquish it */
  tx_thread_relinquish();

  return;
  /* USER CODE END Nx_App_Thread_Entry 2 */

}
/* USER CODE BEGIN 1 */
/**
* @brief  TCP thread entry.
* @param thread_input: thread user data
* @retval none
*/

static VOID App_TCP_Thread_Entry(ULONG thread_input)
{
  UINT ret;
  UCHAR data_buffer[512];
  ULONG bytes_read;
  ULONG		remote_ip_address;
  UINT		remote_port;
  NX_PACKET	*tx_packet;
  NX_PACKET	*rx_packet;
  UINT count = 0;

  // create the TCP socket
  ret = nx_tcp_socket_create(&NetXDuoEthIpInstance, &TCPSocket, "TCP Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY,
                             NX_IP_TIME_TO_LIVE, WINDOW_SIZE, NX_NULL, NX_NULL);
  if (ret != NX_SUCCESS){
    Error_Handler();
  }

  // bind the client socket for the DEFAULT_PORT
  ret =  nx_tcp_client_socket_bind(&TCPSocket, DEFAULT_PORT, NX_WAIT_FOREVER);
  if (ret != NX_SUCCESS){
    Error_Handler();
  }

  /* connect to the remote server on the specified port */
  ret = nx_tcp_client_socket_connect(&TCPSocket, TCP_SERVER_ADDRESS, TCP_SERVER_PORT, NX_WAIT_FOREVER);
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }

  while(count++ < MAX_PACKET_COUNT)
  {
    TX_MEMSET(data_buffer, '\0', sizeof(data_buffer));

    /* allocate the packet to send over the TCP socket */
    ret = nx_packet_allocate(&NxAppPool, &tx_packet, NX_UDP_PACKET, TX_WAIT_FOREVER);
    if (ret != NX_SUCCESS)
    {
      break;
    }

    /* append the message to send into the packet */
    ret = nx_packet_data_append(tx_packet, (VOID *)DEFAULT_MESSAGE, sizeof(DEFAULT_MESSAGE), &NxAppPool, TX_WAIT_FOREVER);
    if (ret != NX_SUCCESS)
    {
      nx_packet_release(tx_packet);
      break;
    }

    /* send the packet over the TCP socket */
    ret = nx_tcp_socket_send(&TCPSocket, tx_packet, DEFAULT_TIMEOUT);
    if (ret != NX_SUCCESS)
    {
      break;
    }

    /* wait for the server response */
    ret = nx_tcp_socket_receive(&TCPSocket, &rx_packet, DEFAULT_TIMEOUT);
    if (ret == NX_SUCCESS)
    {
      /* get the server IP address and  port */
      nx_udp_source_extract(rx_packet, &remote_ip_address, &remote_port);

      /* retrieve the data sent by the server */
      nx_packet_data_retrieve(rx_packet, data_buffer, &bytes_read);

      /* print the received data */
      PRINT_DATA(remote_ip_address, remote_port, data_buffer);

      /* release the server packet */
      nx_packet_release(rx_packet);

      /* toggle the green led on success */
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
    else
    {
      /* no message received exit the loop */
      break;
    }
  }

  /* release the allocated packets */
  nx_packet_release(rx_packet);

  /* disconnect the socket */
  nx_tcp_socket_disconnect(&TCPSocket, DEFAULT_TIMEOUT);

  /* unbind the socket */
  nx_tcp_client_socket_unbind(&TCPSocket);

  /* delete the socket */
  nx_tcp_socket_delete(&TCPSocket);

  /* print test summary on the UART */
  if (count == MAX_PACKET_COUNT + 1)
  {
    printf("\n-------------------------------------\n\tSUCCESS : %u / %u packets sent\n-------------------------------------\n", count - 1, MAX_PACKET_COUNT);
    Success_Handler();
  }
  else
  {
    printf("\n-------------------------------------\n\tFAIL : %u / %u packets sent\n-------------------------------------\n", count - 1, MAX_PACKET_COUNT);
    Error_Handler();
  }
}

VOID App_TCP_socket_send(NX_TCP_SOCKET *tcpsocket, ULONG thread_input){
	ULONG socket_state;

	// ソケットが接続されているかどうかをチェック
	nx_tcp_socket_info_get(&tcpsocket, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &socket_state, NULL, NULL, NULL);
	// if the connections is not established then accept new ones, otherwise start receiving data
	if (socket_state != NX_TCP_ESTABLISHED) {
		ret = nx_tcp_server_socket_accept(&tcpsocket, NX_IP_PERIODIC_RATE);
	}

	ret = nx_packet_allocate(&NxAppPool, &tx_packet, NX_UDP_PACKET, TX_WAIT_FOREVER);
    if (ret != NX_SUCCESS)
    {
      break;
    }

    /* append the message to send into the packet */
    ret = nx_packet_data_append(tx_packet, (VOID *)DEFAULT_MESSAGE, sizeof(DEFAULT_MESSAGE), &NxAppPool, TX_WAIT_FOREVER);
    if (ret != NX_SUCCESS)
    {
      nx_packet_release(tx_packet);
      break;
    }

    /* send the packet over the TCP socket */
    ret = nx_tcp_socket_send(&TCPSocket, tx_packet, DEFAULT_TIMEOUT);
    if (ret != NX_SUCCESS)
    {
      break;
    }

}

/* TODO
 * **nx_tcp_keepalive_initial**
 * キープアライブタイマーが有効になるまでの非アクティブ秒数を指定する。
 * デフォルト値は 2 時間を表す 7200 で、nx_tcp.h で定義されています。
 *
 * **nx_tcp_keepalive_retries**
 * 接続が切断されたと判断されるまでのキープアライブ再試行回数を指定します。
 * デフォルト値は 10 で、nx_tcp.h で定義されています。
 *
 * **nx_tcp_keepalive_retry**
 * キープアライブタイマの再試行間隔を秒数で指定します。
 * デフォルト値は75秒で、nx_tcp.hで定義されています。
 *
 * アプリケーションは、nx_api.hをインクルードする前に値を定義することで、
 * デフォルト値をオーバーライドすることができます。
 */
#define NX_DISABLE_PACKET_CHAIN
void nx_iperf_thread_tcp_tx_entry(ULONG thread_input)
{
UINT       status;
UINT       is_first = NX_TRUE;
NX_PACKET *my_packet = NX_NULL;
#ifndef NX_DISABLE_PACKET_CHAIN
NX_PACKET  *packet_ptr;
NX_PACKET  *last_packet;
ULONG       remaining_size;
#endif /* NX_DISABLE_PACKET_CHAIN */
ULONG       expire_time;
ctrl_info  *ctrlInfo_ptr;
ULONG       packet_size;
NXD_ADDRESS server_ip;

    /* Set the pointer.  */
    ctrlInfo_ptr = (ctrl_info *)thread_input;


    ctrlInfo_ptr -> PacketsTxed = 0;
    ctrlInfo_ptr -> BytesTxed = 0;
    ctrlInfo_ptr -> ThroughPut = 0;
    ctrlInfo_ptr -> StartTime = 0;
    ctrlInfo_ptr -> RunTime = 0;
    ctrlInfo_ptr -> ErrorCode = 0;

    server_ip.nxd_ip_version = ctrlInfo_ptr -> version;

        server_ip.nxd_ip_address.v4 = ctrlInfo_ptr -> ip;

    /* TCP Transmit Test Starts in 2 seconds.  */
    tx_thread_sleep(200);

    // Create the TCP Client socket.
    status =  nx_tcp_socket_create(nx_iperf_test_ip, &tcp_client_socket, "TCP Client Socket",
    			NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, WINDOW_SIZE, NX_NULL, NX_NULL);
    if (status){
        error_counter++;
        return;
    }

    // ソケットをポートにバインドする
    status =  nx_tcp_client_socket_bind(&tcp_client_socket, NX_ANY_PORT, NX_WAIT_FOREVER);
    if (status){
        nx_tcp_socket_delete(&tcp_client_socket);
        error_counter++;
        return;
    }

    // TCPサーバーに接続
    // TODO 接続できない場合はリトライする
    status =  nxd_tcp_client_socket_connect(&tcp_client_socket, &server_ip, ctrlInfo_ptr -> port, NX_WAIT_FOREVER);
    if (status){
        nx_tcp_client_socket_unbind(&tcp_client_socket);
        nx_tcp_socket_delete(&tcp_client_socket);
        error_counter++;
        return;
    }

    /* Set the test start time.  */
    ctrlInfo_ptr -> StartTime = tx_time_get();
    expire_time = (ULONG)(ctrlInfo_ptr -> StartTime + (ctrlInfo_ptr -> TestTime));

    // 指定したソケットのローカル最大セグメントサイズ(MSS)を取得する。
    status = nx_tcp_socket_mss_get(&tcp_client_socket, &packet_size);
    if (status){
        nx_tcp_socket_disconnect(&tcp_client_socket, NX_NO_WAIT);
        nx_tcp_client_socket_unbind(&tcp_client_socket);
        nx_tcp_socket_delete(&tcp_client_socket);
        error_counter++;
        return;
    }

    /* Loop to transmit the packet.  */
    while (tx_time_get() < expire_time)
    {
        /* Allocate a packet.  */
        status =  nx_packet_allocate(nx_iperf_test_pool, &my_packet, NX_TCP_PACKET, NX_WAIT_FOREVER);
        if (status != NX_SUCCESS){
            break;
        }

        /* Write ABCs into the packet payload!  */
        /* Adjust the write pointer.  */
        if (my_packet->nx_packet_prepend_ptr + packet_size <= my_packet->nx_packet_data_end){
            my_packet->nx_packet_append_ptr = my_packet->nx_packet_prepend_ptr + packet_size;
        }
        else{
            packet_size = (ULONG)(my_packet->nx_packet_data_end - my_packet->nx_packet_prepend_ptr);
            my_packet->nx_packet_append_ptr =  my_packet->nx_packet_prepend_ptr + packet_size;
        }
        my_packet->nx_packet_length = packet_size;

        if (is_first){
            memset(my_packet->nx_packet_prepend_ptr, 0, (UINT)(my_packet->nx_packet_data_end - my_packet->nx_packet_prepend_ptr));
            is_first = NX_FALSE;
        }

        /* Send the packet out!  */
        status =  nx_tcp_socket_send(&tcp_client_socket, my_packet, NX_WAIT_FOREVER);
        if (status){
            // TODO 送信リトライ
            error_counter++;
            nx_packet_release(my_packet);
            break;
        }
        else{
            /* Update the counter.  */
            ctrlInfo_ptr -> PacketsTxed++;
            ctrlInfo_ptr -> BytesTxed += packet_size;
        }
    }

    /* Calculate the test time and Throughput(Mbps).  */
    ctrlInfo_ptr -> RunTime = tx_time_get() - ctrlInfo_ptr -> StartTime;

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    _tx_execution_thread_total_time_get(&thread_time);
    _tx_execution_isr_time_get(&isr_time);
    _tx_execution_idle_time_get(&idle_time);
#endif

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    ctrlInfo_ptr -> idleTime = (ULONG)((unsigned long long)idle_time * 100 / ((unsigned long long)thread_time + (unsigned long long)isr_time + (unsigned long long)idle_time));
#endif

    /* Disconnect this socket.  */
    status =  nx_tcp_socket_disconnect(&tcp_client_socket, NX_NO_WAIT);
    /* ステータスが有効かどうかを確認します。 */
    if (status){
        error_counter++;
    }

    /* Unbind the socket.  */
    status =  nx_tcp_client_socket_unbind(&tcp_client_socket);
    if (status){
        error_counter++;
    }

    if (error_counter){
        ctrlInfo_ptr -> ErrorCode = error_counter;
    }

    /* Delete the socket.  */
    nx_tcp_socket_delete(&tcp_client_socket);
}




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
          /* Print TCP Echo Client is available again. */
          printf("TCP Echo Client is available again.\n");
        }
        else
        {
          /* The network cable is connected. */
          printf("The network cable is connected.\n");
          /* Send command to Enable Nx driver. */
          nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_ENABLE,
                                      &actual_status);
          /* Restart DHCP Client. */
          nx_dhcp_stop(&DHCPClient);
          nx_dhcp_start(&DHCPClient);
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
