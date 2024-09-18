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
 * サーバーポートでのリスニングの停止
 * アプリケーションが、nx_tcp_server_socket_listenサービスへのコールで以前に
 * 指定されたサーバーポートでクライアント接続リクエストをリスニングすることを
 * 望まなくなった場合、アプリケーションは単にnx_tcp_server_socket_unlisten
 * サービスをコールします。このサービスは、接続待ちのソケットをCLOSED状態に戻し、
 * キューに入れられたクライアント接続リクエストパケットを解放します。
 *
 * TCPウィンドウサイズ
 * 接続のセットアップフェーズおよびデータ転送フェーズの両方において、各ポートは
 * 処理可能なデータ量を報告します。この量をウィンドウサイズと呼びます。データが
 * 受信され処理されると、このウィンドウサイズは動的に調整されます。TCPでは、
 * 送信者は受信者のウィンドウに収まる量のデータのみを送信できます。
 * つまり、ウィンドウサイズは接続の各方向におけるデータ転送のフロー制御を提供します。
 *
 * TCP パケットの送信
 * TCP データの送信は、nx_tcp_socket_send 関数を呼び出すことで簡単に実行できます。
 * 送信されるデータのサイズがソケットの MSS 値または現在のピア受信ウィンドウサイズ
 * (いずれか小さい方)よりも大きい場合、TCP の内部ロジックにより、min (MSS, ピア受
 * 信ウィンドウ) に収まるサイズのデータが送信用に切り出されます。その後、このサー
 * ビスにより、パケットの前に TCP ヘッダーが作成されます(チェックサム計算を含む)。
 * 受信側のウィンドウサイズがゼロでない場合、呼び出し側は受信側のウィンドウサイズ
 * を埋められるだけ多くのデータを送信します。受信ウィンドウがゼロになった場合、
 * 呼び出し側は送信を中断し、このパケットが送信されるのに十分な受信側のウィンドウ
 * サイズの増加を待ちます。任意の時点で、同じソケットを通じてデータを送信しようと
 * している間に、複数のスレッドが中断される可能性があります。
 *
 * NX_PACKET構造体に格納されているTCPデータは、ロングワードの境界上に存在する必要
 * があります。さらに、TCP、IP、および物理メディアのヘッダーを配置するために、先頭
 * ポインタとデータ開始ポインタの間に十分なスペースが必要です。
 *
 * TCP パケットの再送信
 * 以前に送信された TCP パケットは、接続の相手側から ACK が返されるまで、実際には
 * 内部に保存されます。タイムアウト期間内に送信データが確認されなかった場合、保存
 * されたパケットが再送信され、次のタイムアウト期間が設定されます。
 * ACK を受信すると、内部送信キュー内の確認番号でカバーされたすべてのパケットが
 * 最終的に解放されます。
 * アプリケーションは、nx_tcp_socket_send()がNX_SUCCESSを返した後、そのパケットを
 * 再利用したり、パケットの内容を変更したりしてはなりません。送信されたパケットは、
 * 最終的に、相手側でデータが確認された後に、NetX Duoの内部処理によって解放されます。
 *
 * TCP Keepalive
 * TCP Keepalive機能は、ソケットが相手側が適切に終了することなく切断したかどうか
 * (例えば、相手側がクラッシュしたなど)を検出したり、特定のネットワーク監視機能が
 * 長期間アイドル状態のために接続を終了することを防止したりすることができます。
 * TCP Keepalive は、データを含まない TCP フレームを定期的に送信し、シーケンス番号
 * を現在のシーケンス番号より 1 つ少なく設定することで機能します。このような TCP
 * Keepalive フレームを受信すると、受信側がまだ生きている場合は、現在のシーケンス
 * 番号に対する ACK で応答します。これにより、キープアライブの処理が完了します。
 * デフォルトでは、キープアライブ機能は有効になっていません。この機能を使用するには、
 * NetX Duo ライブラリを NX_ENABLE_TCP_KEEPALIVE を定義してビルドする必要があります。
 * シンボル NX_TCP_KEEPALIVE_INITIAL は、キープアライブフレームが開始されるまでの
 * 非アクティブ状態の秒数を指定します。
 *
 * TCP パケットの受信
 * TCP 受信パケット処理(IP ヘルパースレッドから呼び出される)は、送信確認処理だけで
 * なく、さまざまな接続および切断処理を処理する役割を担っています。さらに、TCP受信
 * パケット処理は、受信データを持つパケットを適切な TCP ソケットの受信キューに配置
 * したり、パケットを待機中の最初のスレッドに配信したりする役割も担っています。
 *
 * TCP 受信通知
 * アプリケーションスレッドが複数のソケットからの受信データを処理する必要がある
 * 場合は、nx_tcp_socket_receive_notify 関数を使用します。この関数は、ソケットに
 * 対して受信パケットコールバック関数を登録します。ソケットにパケットが受信されると、
 * コールバック関数が実行されます。
 * コールバック関数の内容はアプリケーションに依存しますが、対応するソケットにパケ
 * ットが到着したことを処理スレッドに通知するロジックが含まれる可能性が高いでしょう。
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
NX_IP			NetXDuoEthIpInstance;
TX_BYTE_POOL 	*TxBytePool;
NX_PACKET_POOL	NxPacketPool;
TX_THREAD		NxAppThread;
TX_SEMAPHORE	DHCPSemaphore;
NX_DHCP			DHCPClient;

TX_THREAD		AppTCPThread;
TCP_TASK			TcpManagement;
#define			TcpManagementPort	8001
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_app_thread_entry (ULONG thread_input);
/* USER CODE BEGIN PFP */
/* Define thread prototypes.  */
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

//	// CREATE TCP THREAD
//	//
//	{
//		CHAR *pointer;
//		// Allocate the memory for TCP server thread
//		if (TX_SUCCESS != ASSERT_ERROR("nx_tcp_allocate",
//				tx_byte_allocate(TxBytePool, (VOID**) &pointer, 2 * DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT))){
//			return TX_POOL_ERROR;
//		}
//		// Create the TCP server thread
//		if (TX_SUCCESS != ASSERT_ERROR("nx_tcp_thread",
//				tx_thread_create(&AppTCPThread, "App TCP Thread", App_TCP_Thread_Entry, 0, pointer, NX_APP_THREAD_STACK_SIZE,
//				NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START))){
//			return NX_NOT_SUCCESSFUL;
//		}
//	}
	if (NX_SUCCESS != ASSERT_ERROR("TCP Manegement Interface Thread",
			TcpServerThread_Create(&TcpManagement, TcpManagementPort, "Management TCP Thread", TX_DONT_START, DEFAULT_PRIORITY))){
		return NX_NOT_SUCCESSFUL;
	}
	// Enable TCP Protocol
	if (NX_SUCCESS != ASSERT_ERROR("nx_tcp_enable",
			nx_tcp_enable(&NetXDuoEthIpInstance))){
		return NX_NOT_SUCCESSFUL;
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
			return; // NX_DHCP_ERROR;
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
	TcpThread_Resume(&TcpManagement);

	/* if this thread is not needed any more, we relinquish it */
	tx_thread_relinquish();
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
