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
#include "logmsg.h"

/* Private includes ----------------------------------------------------------*/
#include "nxd_dhcp_client.h"

/* Private typedef -----------------------------------------------------------*/
/* Define the ThreadX and NetX object control blocks...  */
TX_THREAD		AppLinkThread;
TX_SEMAPHORE	Semaphore;
NX_PACKET_POOL	WebServerPool;
NX_PACKET_POOL	IPPacketPool;

UINT	DHCP_Enabled			=	0;
ULONG	DefaultIpAddress		=	IP_ADDRESS(172,  16,  51,  80);
ULONG	DefaultSubNetMask		=	IP_ADDRESS(255, 255,   0,   0);
#define	DEFAULT_IP_ADDRESS			IP_ADDRESS(192, 168,   0, 100)
#define	DEFAULT_NET_MASK			IP_ADDRESS(255, 255,   0,   0)

ULONG	IpAddress;
ULONG	NetMask;


#define HTTP_ENABLED	0

#if HTTP_ENABLED
UCHAR	*http_stack;
static uint8_t nx_server_pool[SERVER_POOL_SIZE];
#endif

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
TX_BYTE_POOL 	*TxBytePool;
TX_THREAD		NxAppThread;
NX_PACKET_POOL	NxPacketPool;
NX_IP			NetXDuoEthIpInstance;
TX_SEMAPHORE	DHCPSemaphore;
NX_DHCP			DHCPClient;

TX_THREAD		AppTCPThread;

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
	TxBytePool = (TX_BYTE_POOL*)memory_ptr;

	printf("NetXDuo application started..\n");
	// Initialize the NetXDuo system.
	nx_system_initialize();

	// CREATE PACKET POOL
	// アプリ内の通信で使用する通信パケット用メモリを確保する
	{
		CHAR *pointer;
		// Allocate the memory for packet_pool.  
		if (TX_SUCCESS != ASSERT_ERROR("nx_packet_pool_allocate",
				tx_byte_allocate(TxBytePool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT))){
			return TX_POOL_ERROR;
		}
		// パケット割り当てに使用するパケットプールを作成します。
		// 余分な NX_PACKET を使用する場合は、**NX_APP_PACKET_POOL_SIZE** を大きくする必要があります。
		if (NX_SUCCESS != ASSERT_ERROR("nx_packet_pool_create",
				nx_packet_pool_create(&NxPacketPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE))){
			return NX_NOT_SUCCESSFUL;
		}
	}

	// CREATE IP INSTANCE 
	{
		CHAR *pointer;
		// Allocate the memory for Ip_Instance
		if (TX_SUCCESS != ASSERT_ERROR("nx_ip_allocate",
				tx_byte_allocate(TxBytePool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT))){
			return TX_POOL_ERROR;
		}
		// Create the main NX_IP instance
		if (NX_SUCCESS != ASSERT_ERROR("nx_ip_create",
				nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", DefaultIpAddress, DefaultSubNetMask,
						&NxPacketPool, nx_stm32_eth_driver, pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY))){
			return NX_NOT_SUCCESSFUL;
		}
	}

	// CREATE ARP
	{
		CHAR *pointer;
		// Allocate the memory for ARP
		if (TX_SUCCESS != ASSERT_ERROR("nx_arp_allocate",
				tx_byte_allocate(TxBytePool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT))){
			return TX_POOL_ERROR;
		}
		// Enable the ARP protocol and provide the ARP cache size for the IP instance
		if (NX_SUCCESS != ASSERT_ERROR("nx_arp_enable",
				nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE))){
			return NX_NOT_SUCCESSFUL;
		}
		// Enable the ICMP
		if (NX_SUCCESS != ASSERT_ERROR("nx_icmp_enable",
				nx_icmp_enable(&NetXDuoEthIpInstance))){
			return NX_NOT_SUCCESSFUL;
		}
	}

	// CREATE TCP THREAD
	// 
	{
		CHAR *pointer;
		// Allocate the memory for TCP server thread
		if (TX_SUCCESS != ASSERT_ERROR("nx_tcp_allocate",
				tx_byte_allocate(TxBytePool, (VOID**) &pointer, 2 * DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT))){
			return TX_POOL_ERROR;
		}
		// Create the TCP server thread 
		if (TX_SUCCESS != ASSERT_ERROR("nx_tcp_thread",
				tx_thread_create(&AppTCPThread, "App TCP Thread", App_TCP_Thread_Entry, 0, pointer, NX_APP_THREAD_STACK_SIZE,
				NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START))){
			return NX_NOT_SUCCESSFUL;
		}
		// Enable TCP Protocol
		if (NX_SUCCESS != ASSERT_ERROR("nx_tcp_enable",
				nx_tcp_enable(&NetXDuoEthIpInstance))){
			return NX_NOT_SUCCESSFUL;
		}
	}

	// Enable the UDP protocol required for  DHCP communication
	if (NX_SUCCESS != ASSERT_ERROR("nx_udp_enable",
			nx_udp_enable(&NetXDuoEthIpInstance))){
		return NX_NOT_SUCCESSFUL;
	}

	// CREATE MAIN THREAD
	{
		CHAR *stack_space;
		// Allocate the memory for main thread
		if (TX_SUCCESS != ASSERT_ERROR("tx_thread_allocate",
				tx_byte_allocate(TxBytePool, (VOID **) &stack_space, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT))){
			return TX_POOL_ERROR;
		}
		// Allocate the memory for HTTP Server stack.
		// Create the main thread
		if (TX_SUCCESS != ASSERT_ERROR("tx_thread_create",
				tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_app_thread_entry , 0, stack_space, NX_APP_THREAD_STACK_SIZE,
						NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START))){
			return TX_THREAD_ERROR;
		}
	}

#if HTTP_ENABLED
	// CREATE HTTP SERVER
	{
		CHAR *pointer;
		// Allocate the memory for HTTP server
		ASSERT_FATALERROR("HTTP Server packet pool memory allocation",
				tx_byte_allocate(TxBytePool, (VOID **) &pointer, SERVER_POOL_SIZE, TX_NO_WAIT));
		// Create the HTTP server packet pool.
		ASSERT_FATALERROR("HTTP Server packet pool creation",
				nx_packet_pool_create(&WebServerPool, "HTTP Server Packet Pool", SERVER_PACKET_SIZE, nx_server_pool, SERVER_POOL_SIZE));
		// Allocate the memory for HTTP Server stack.
		ASSERT_FATALERROR("Server stack memory allocation",
				tx_byte_allocate(TxBytePool, (VOID **) &pointer, HTTP_STACK_SIZE, TX_NO_WAIT));
		http_stack = (UCHAR *)pointer;
	}
#endif
//	{
//		CHAR *pointer;
//		// Allocate the memory for the Application stack.
//		ASSERT_FATALERROR("Application stack memory allocation",
//				tx_byte_allocate(TxBytePool, (VOID **) &pointer, APPLY_STACK_SIZE, TX_NO_WAIT));
//		Apply_stack = (UCHAR *)pointer;
//	}

	// CREATE LINK THREAD
	{
		CHAR *stack_space;
		// Allocate the memory for Link thread
		if (TX_SUCCESS != ASSERT_ERROR("Allocate the memory for App Link thread",
				tx_byte_allocate(TxBytePool, (VOID **) &stack_space,2 *  DEFAULT_MEMORY_SIZE, TX_NO_WAIT))){
			return TX_POOL_ERROR;
		}
		// create the Link thread
		if (TX_SUCCESS != ASSERT_ERROR("Create the App Link thread",
				tx_thread_create(&AppLinkThread, "App Link Thread", App_Link_Thread_Entry, 0, stack_space, 2 * DEFAULT_MEMORY_SIZE,
						LINK_PRIORITY, LINK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START))){
			return NX_NOT_ENABLED;
		}
	}

	// IPAddressを表示
	ASSERT_FATALERROR("nx_ip_address_get",
			nx_ip_address_get(&NetXDuoEthIpInstance, &IpAddress, &NetMask));
	PRINT_IP_ADDRESS(IpAddress);

	if (IpAddress != 0){
		tx_semaphore_put(&DHCPSemaphore);
	}

	return NX_SUCCESS;
}




/**------------------------------------------------------------------------------------------------
 * @brief  ip address change callback.
 * @param ip_instance: NX_IP instance
 * @param ptr: user data
 * @retval none
 */
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr)
{
	/* release the semaphore as soon as an IP address is available */
	tx_semaphore_put(&DHCPSemaphore);
}

/** ------------------------------------------------------------------------------------------------
 * @brief Main thread entry.
 * @param thread_input: ULONG user argument used by the thread entry
 * @retval none
 */
static VOID nx_app_thread_entry (ULONG thread_input)
{
	// CREATE DHCP CLIENT
	// DHCP_Enabled ならばDHCPによるアドレス解決を行う
	// TODO 起動時のみのDHCPであり、DHCP経由でのアドレス変更は未対応
	if ( DHCP_Enabled != 0 ){
		// Create the DHCP client
		if (NX_SUCCESS != ASSERT_ERROR("nx_dhcp_create",
				nx_dhcp_create(&DHCPClient, &NetXDuoEthIpInstance, "DHCP Client"))){
			return NX_DHCP_ERROR;
		}
		// set DHCP notification callback
		tx_semaphore_create(&DHCPSemaphore, "DHCP Semaphore", 0);

		// IP address change callbackを設定する
		ASSERT_FATALERROR("nx_ip_address_change_notify",
			nx_ip_address_change_notify(&NetXDuoEthIpInstance, ip_address_change_notify_callback, NULL));

		// DHCPサーバーからのIP ADDRESS取得を開始
		ASSERT_FATALERROR("nx_dhcp_start",
			nx_dhcp_start(&DHCPClient));

		// DHCPサーバーからのIP ADDRESS取得を待機
		if (TX_SUCCESS != ASSERT_ERROR("tx_semaphore_get DHCP connection",
			tx_semaphore_get(&DHCPSemaphore, NX_APP_DEFAULT_TIMEOUT))){
			// DHCPサーバーからのIP ADDRESS取得に失敗した
			if (NX_APP_DEFAULT_IP_ADDRESS == 0 || NX_APP_DEFAULT_NET_MASK == 0)
				ERROR_HANDLER();
			// NX_APP_DEFAULT_IP_ADDRESS が定義されていれば固定IPAddressとして設定
			ASSERT_FATALERROR("nx_ip_address_set",
				nx_ip_address_set(&NetXDuoEthIpInstance, NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK));
		}
	}

	/* USER CODE BEGIN Nx_App_Thread_Entry 2 */
	ASSERT_FATALERROR("nx_ip_address_get",
		nx_ip_address_get(&NetXDuoEthIpInstance, &IpAddress, &NetMask));
	PRINT_IP_ADDRESS(IpAddress);

#if HTTP_ENABLED
	/* Call entry function to start iperf test.  */
	nx_iperf_entry(&WebServerPool, &NetXDuoEthIpInstance, http_stack, HTTP_STACK_SIZE, iperf_stack, IPERF_STACK_SIZE);
#endif
	/* the network is correctly initialized, start the TCP server thread */
	tx_thread_resume(&AppTCPThread);

	/* if this thread is not needed any more, we relinquish it */
	tx_thread_relinquish();
}

/** ------------------------------------------------------------------------------------------------
 * 
 */
typedef struct TCPTask_Struct {
	TX_THREAD		Thread;
	CHAR*			Name;
	UCHAR 			*StackSpace;
	ULONG 			StackSize;	// = NX_APP_THREAD_STACK_SIZE;
	UINT			Priority;

	NX_TCP_SOCKET	Socket;	// TcpSocket
	UINT 			Port;				//
	ULONG 			WindowSize;
	NXD_ADDRESS		RemoteIP;
	UINT			RemotePort;
	ULONG			QueueMax;			// QUEUE_MAX_SIZE
	NX_PACKET 		*rx_packet;
	void (*RecieveCallback)(UCHAR* buffer, UINT buffer_length);
	UINT volatile	Active;
} TCPTask;

typedef struct UDPTask_Struct {
	TX_THREAD		Thread;
	CHAR*			Name;
	UCHAR 			*StackSpace;
	ULONG 			StackSize;	// = NX_APP_THREAD_STACK_SIZE;
	UINT			Priority;

	NX_UDP_SOCKET 	Socket;		//
	UINT			Port;		// DEFAULT_PORT
	ULONG 			WindowSize;
	NXD_ADDRESS		RemoteIP;
	UINT			RemotePort;
	ULONG			QueueMax;			// QUEUE_MAX_SIZE
	NX_PACKET 		*rx_packet;
	void (*RecieveCallback)(UCHAR* buffer, UINT buffer_length);
	UINT volatile	Active;
} UDPTask;

TCPTask TcpControlTask = {
	//Thread;
	.Name = "TCP Control",
	.StackSpace 		= NX_NULL,
	.StackSize 			= NX_APP_THREAD_STACK_SIZE,
	.Priority 			= 10,

	//NX_TCP_SOCKET	Socket;	// TcpSocket
	.Port 				= 8001,				//
	.WindowSize			= ASI_PACKET_SIZE,
	//RemoteIP;
	//RemotePort;
	.QueueMax 			= 1,
	.rx_packet 			= NX_NULL,
	.RecieveCallback 	= NULL,
	.Active 			= 0
};




/** ------------------------------------------------------------------------------------------------
 * @brief 
 * @param 
 * @retval 
 */
void TcpServerThread_Cleanup(TCPTask* this)
{
    nx_tcp_socket_disconnect(&this->Socket, NX_NO_WAIT);
    nx_tcp_server_socket_unaccept(&this->Socket);
    nx_tcp_server_socket_unlisten(&NetXDuoEthIpInstance, this->Port);
    nx_tcp_socket_delete(&this->Socket);

    tx_thread_terminate(&this->Thread);
    tx_thread_delete(&this->Thread);

    tx_byte_release(this->StackSpace);
}

static void TcpServerThread_Entry(ULONG thread_input);

UINT TcpServerThread_Create(TCPTask* this, UINT tx_start)
{
	UINT status;
	// Allocate the memory for main thread
	if (this->StackSpace == NX_NULL){
		if (TX_SUCCESS != ASSERT_ERROR("TcpThread_Stack_allocate",
				status = tx_byte_allocate(TxBytePool, (VOID**)&this->StackSpace, this->StackSize, TX_NO_WAIT))){
			return status;
		}
	}
	if (TX_SUCCESS != ASSERT_ERROR("TcpThread_Create",
			status = tx_thread_create(
						&this->Thread,
						this->Name,
						TcpServerThread_Entry,
                        (ULONG)this,
						this->StackSpace, this->StackSize,
						this->Priority, // 優先度（0～(TX_MAX_PRIORITIES-1)）
						this->Priority, // 先取無効の最大優先度（0～(TX_MAX_PRIORITIES-1)）
						// この設定値より高い優先度のスレッドのみ先取が許可される。
                        TX_NO_TIME_SLICE, tx_start /*TX_AUTO_START*/))){
	}
    return status;
}

/** -----------------------------------------------------------------------------------------------
* @brief  TCP thread entry.
* @param thread_input: thread user data
* @retval none
*/
static void TcpServerThread_Entry(ULONG thread_input)
{
	TCPTask* 	this = (TCPTask*)thread_input;
	UINT        status;
	ULONG       actual_status;

    // IPインスタンスが初期化されていることを確認します。
    // この機能は、必要な条件を満たすスレッドスリープを使用して、指定されたインターフェースのリンク状態をポーリングします。
    // 要求されたステータスがIPインスタンスにのみ存在する場合、例えばNX_IP_INITIALIZE_DONEの場合、このサービスは、そのステータスに対するIP設定を提供します。
    status =  nx_ip_status_check(&NetXDuoEthIpInstance, NX_IP_INITIALIZE_DONE, &actual_status, NX_IP_PERIODIC_RATE);
    if (status != NX_SUCCESS){
        return;
    }

    // TCP Server Socket を作成する
    // この機能は、指定された IP インスタンス用の TCP ソケットを作成します。
    // このサービスにより、クライアントソケットとサーバソケットの両方が作成されます。
    status =  nx_tcp_socket_create(
    				&NetXDuoEthIpInstance,	// Pointer to previously created IP instance.
					&this->Socket,
					"TCP Server Socket",
					NX_IP_NORMAL,
					NX_FRAGMENT_OKAY,
					NX_IP_TIME_TO_LIVE,
					this->WindowSize, // window_size : 受信キューの最大バイト数
					NX_NULL,		// urgent_data_callback
					NX_NULL			// disconnect_callback
				   );
    if (status){
        return;
    }

    // Setup this thread to listen.
    // 指定した TCP ポートに対するリッスン要求とサーバソケットを登録します。
    status =  nx_tcp_server_socket_listen(&NetXDuoEthIpInstance, this->Port, &this->Socket, this->QueueMax, NX_NULL);
    if (status){
        nx_tcp_socket_delete(&this->Socket);
        return;
    }

    // この関数は、アクティブなClient接続リクエストを受信した後にサーバーソケットを設定します。
    status =  nx_tcp_server_socket_accept(&this->Socket, NX_WAIT_FOREVER);
    if (status){
        nx_tcp_server_socket_unlisten(&NetXDuoEthIpInstance, this->Port);
        nx_tcp_socket_delete(&this->Socket);
        return;
    }

    // この関数は、指定された TCP ソケットに接続されているピアの IP アドレスとポート番号を取得します。
    status = nxd_tcp_socket_peer_info_get(&this->Socket, &this->RemoteIP, &this->RemotePort);
    if (status){
        nx_tcp_server_socket_unaccept(&this->Socket);
        nx_tcp_server_socket_unlisten(&NetXDuoEthIpInstance, this->Port);
        nx_tcp_socket_delete(&this->Socket);
        return;
    }

    // TCP受信処理
    this->Active = 1;
    while (this->Active){
        /* Receive a TCP message from the socket.  */
        status =  nx_tcp_socket_receive(&this->Socket, &this->rx_packet, NX_WAIT_FOREVER);
        if (status){
            break;
        }

        /* Release the packet.  */
        nx_packet_release(this->rx_packet);
    }

    /* Disconnect the server socket.  */
    status =  nx_tcp_socket_disconnect(&this->Socket, 10);
    if (status){
        //error_counter++;
    }

    /* Unaccept the server socket.  */
    status =  nx_tcp_server_socket_unaccept(&this->Socket);
    status += nx_tcp_server_socket_unlisten(&NetXDuoEthIpInstance, this->Port);
    if (status){
        //error_counter++;
    }

    /* Delete the socket.  */
    nx_tcp_socket_delete(&this->Socket);
}

/** -----------------------------------------------------------------------------------------------
* @brief  TCP thread entry.
* @param thread_input: thread user data
* @retval none
*/
static VOID TcpClientThread_Entry(ULONG thread_input){
	TCPTask* 	this = (TCPTask*)thread_input;
    UINT	status;
//    ULONG	remote_ip_address;
//    UINT	remote_port;
//    NX_PACKET *rx_packet;

    // TCP Server Socket を作成する
    // この機能は、指定された IP インスタンス用の TCP ソケットを作成します。
    // このサービスにより、クライアントソケットとサーバソケットの両方が作成されます。
    status =  nx_tcp_socket_create(
    				&NetXDuoEthIpInstance,	// Pointer to previously created IP instance.
					&this->Socket,
					"TCP Server Socket",
					NX_IP_NORMAL,
					NX_FRAGMENT_OKAY,
					NX_IP_TIME_TO_LIVE,
					this->WindowSize, // window_size : 受信キューの最大バイト数
					NX_NULL,		// urgent_data_callback
					NX_NULL			// disconnect_callback
				   );
    if (status != NX_SUCCESS)
        return;

    // bind the client socket for the DEFAULT_PORT
    status =  nx_tcp_client_socket_bind(&this->Socket, this->Port, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
      	return;

    /* connect to the remote server on the specified port */
    status = nxd_tcp_client_socket_connect(&this->Socket, &this->RemoteIP, this->RemotePort, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
      	return;

//    remote_ip_address = this->Socket.nx_tcp_socket_connect_ip.nxd_ip_address.v4;
//    remote_port       = this->Socket.nx_tcp_socket_connect_port;
//    PRINT_IP_ADDRESS_PORT(remote_ip_address, remote_port);

    this->Active = 1;
    while (this->Active){
        if ((status = nx_tcp_socket_receive(&this->Socket, &this->rx_packet, 100)) == NX_SUCCESS){
			ULONG data_length;
			UCHAR data_buffer[ASI_PACKET_SIZE];
			if (nx_packet_length_get(this->rx_packet, &data_length) != NX_SUCCESS)
				printf("Invalid packet pointer\r\n");
			else if (data_length > sizeof(data_buffer))
				printf("Invalid packet length %lu\r\n", data_length);
			else {
				TX_MEMSET(data_buffer, '\0', sizeof(data_buffer));
				nx_packet_data_retrieve(this->rx_packet, data_buffer, &data_length);
				nxd_udp_source_extract(this->rx_packet, &this->RemoteIP, &this->RemotePort);
			//	PRINT_IP_ADDRESS_PORT(source_ip_address, source_port);
				if (this->RecieveCallback != NULL)
					this->RecieveCallback(data_buffer, data_length);
			}
        }
        else if (status != NX_NO_PACKET)
          break;
    }
	nx_packet_release(this->rx_packet);
    nx_tcp_socket_disconnect(&this->Socket, 10);
    nx_tcp_client_socket_unbind(&this->Socket);
    nx_tcp_socket_delete(&this->Socket);
}

/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
void UdpServerThread_Cleanup(UDPTask* this)
{
    nx_udp_socket_unbind(&this->Socket);
    nx_udp_socket_delete(&this->Socket);
    tx_thread_terminate(&this->Thread);
    tx_thread_delete(&this->Thread);
}

static VOID UdpServerThread_Entry(ULONG thread_input);

/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
UINT UdpServerThread_Create(UDPTask* this, UINT tx_start)
{
    UINT status;
    status = tx_thread_create(
                &this->Thread,
                this->Name,
				UdpServerThread_Entry,
                (ULONG)this,
                this->StackSpace, this->StackSize,
                this->Priority,
                this->Priority,	// NX_WEB_HTTP_SERVER_PRIORITY + 1,
                TX_NO_TIME_SLICE,
                tx_start	// TX_AUTO_START
                );
    return status;
}

/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
static VOID UdpServerThread_Entry(ULONG thread_input){
    UDPTask *this = (UDPTask*)thread_input;

	// create the UDP socket
	if (nx_udp_socket_create(&NetXDuoEthIpInstance, &this->Socket, "UDP Server Socket", 
				NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, this->QueueMax) != NX_SUCCESS)
		return;
	// bind the socket indefinitely on the required port
	if (nx_udp_socket_bind(&this->Socket, this->Port, TX_WAIT_FOREVER) != NX_SUCCESS)
		return;

	printf("UDP Server listening on PORT %d.. \n", this->Port);
	while(1){
		if (nx_udp_socket_receive(&this->Socket, &this->rx_packet, 100) == NX_SUCCESS){
			ULONG data_length;
			UCHAR data_buffer[ASI_PACKET_SIZE];
			if (nx_packet_length_get(this->rx_packet, &data_length) != NX_SUCCESS)
				printf("Invalid packet pointer\r\n");
			else if (data_length > sizeof(data_buffer))
				printf("Invalid packet length %lu\r\n", data_length);
			else {
				TX_MEMSET(data_buffer, '\0', sizeof(data_buffer));
				nx_packet_data_retrieve(this->rx_packet, data_buffer, &data_length);
				nxd_udp_source_extract(this->rx_packet, &this->RemoteIP, &this->RemotePort);
			//	PRINT_IP_ADDRESS_PORT(source_ip_address, source_port);
				if (this->RecieveCallback != NULL)
					this->RecieveCallback(data_buffer, data_length);
				nx_packet_release(this->rx_packet);
			}
		}
	}
}


UINT UdpSend(UDPTask* this, VOID* data_buf, ULONG data_len){
	UINT         status;
	NX_PACKET   *packet = NX_NULL;

    /* Send the end of test indicator. */
    status = nx_packet_allocate(&NxPacketPool, &packet, NX_UDP_PACKET, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
      return status;

    status = nx_packet_data_append(packet, data_buf, data_len, &NxPacketPool, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
        return status;

    /* Send the UDP packet.  */
    status = nxd_udp_socket_send(&this->Socket, packet, &this->RemoteIP, this->RemotePort);
    if (status != NX_SUCCESS)
        nx_packet_release(packet);
    return status;
}



/** -----------------------------------------------------------------------------------------------
 * @brief  Link thread entry
 * @param thread_input: ULONG thread parameter
 * @retval none
 */
static VOID App_Link_Thread_Entry(ULONG thread_input){
    ULONG actual_status;
    UINT linkdown = 0, status;
    while(1){
        /* Get Physical Link status. */
        status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_LINK_ENABLED, &actual_status, 10);
        if( status == NX_SUCCESS ){	// NX_IP_LINK_ENABLED
            if( linkdown == 1 ){ // LINK DOWN -> UP
                linkdown = 0;
                status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_ADDRESS_RESOLVED, &actual_status, 10);
                if( status == NX_SUCCESS ){ // ADDRESS RESOLVED
                    printf("The network cable is connected again.\n");
                    printf("TCP Echo Client is available again.\n");
                }
                else{
                    printf("The network cable is connected.\n");
                    /* Send command to Enable Nx driver. */
                    nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_ENABLE, &actual_status);
                    /* Restart DHCP Client. */
					if (DHCP_Enabled != 0){
                	    nx_dhcp_stop(&DHCPClient);
                    	nx_dhcp_start(&DHCPClient);
					}
                }
            }
        }
        else{ // NX_IP_LINK_DISABLED
            if( linkdown == 0 ){ // LINK UP -> DOWN
                linkdown = 1;
                /* The network cable is not connected. */
                printf("The network cable is not connected.\n");
            }
        }
        tx_thread_sleep(NX_APP_CABLE_CONNECTION_CHECK_PERIOD);
    }
}
