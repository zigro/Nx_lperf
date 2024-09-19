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

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
VOID TcpThread_Resume(TCP_TASK *this){
	tx_thread_resume(&this->Thread);
}
/** -----------------------------------------------------------------------------------------------
* @brief  TCP socket recieve
* @param  TCP_TASK *this
* @retval none
*/
static UINT TcpSocket_Recieve(TCP_TASK *this){
	UINT nx_status;
	NX_PACKET *rx_packet;

	nx_status = nx_tcp_socket_receive(&this->Socket, &rx_packet, RECIEVE_TIMEOUT);
	if (nx_status == NX_SUCCESS){
		ULONG data_length;
		UCHAR data_buffer[this->WindowSize];
		if (nx_packet_length_get(rx_packet, &data_length) != NX_SUCCESS)
			printf("Invalid packet pointer\r\n");
		else if (data_length > sizeof(data_buffer))
			printf("Invalid packet length %lu\r\n", data_length);
		else {
			TX_MEMSET(data_buffer, '\0', sizeof(data_buffer));
			nx_packet_data_retrieve(rx_packet, data_buffer, &data_length);
		//	nxd_udp_source_extract(rx_packet, &this->RemoteIP, &this->RemotePort);
		//	PRINT_IP_ADDRESS_PORT(source_ip_address, source_port);
			if (this->RecieveCallback != NULL)
				this->RecieveCallback(this, data_buffer, (UINT)data_length);
		}
		nx_packet_release(rx_packet);
	}
	else if (nx_status == NX_NO_PACKET)
		nx_status = NX_SUCCESS;
	return nx_status;
}

/**	-------------------------------------------------------------------------------------------------------------------
 *  _TcpSocket_Send_Check
 *  送信可能かどうかをチェックする
*/
static UINT _TcpSocket_Send_Check(TCP_TASK *this){
	UINT nx_status;
	ULONG socket_state;

    // スレッドの生存確認
    if (this->Active == NX_FALSE) return NX_NOT_CONNECTED;
	// ソケットが接続されているかどうかをチェック
	nx_status = nx_tcp_socket_info_get(&this->Socket, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &socket_state, NULL, NULL, NULL);
	if (nx_status != NX_SUCCESS) return nx_status;
	// if the connections is not established
	if (socket_state != NX_TCP_ESTABLISHED) return NX_NOT_CONNECTED;
    // 送信スレッド内で再度呼び出された
    if (this->SendThread == &this->Thread) return NX_CALLER_ERROR;
    return NX_SUCCESS;
}
/**	-------------------------------------------------------------------------------------------------------------------
 *  _TcpSocket_Send
 *  排他制御 ソケット送信処理
*/
static UINT _TcpSocket_Send(TCP_TASK *this, NX_PACKET *tx_packet){
	UINT nx_status = tx_semaphore_get(&this->TxSemaphore, SEND_TIMEOUT);
	if (nx_status == TX_SUCCESS){
		/* send the packet over the TCP socket */
		nx_status = nx_tcp_socket_send(&this->Socket, tx_packet, this->SendWaitTicks);
		tx_semaphore_put(&this->TxSemaphore);
	}
	return nx_status;
}

/**	-------------------------------------------------------------------------------------------------------------------
 *  TcpSocket_Send
 *  ソケット送信処理
*/
static UINT TcpSocket_Send(TCP_TASK *this, const UCHAR *data_buffer, UINT data_length){
	UINT nx_status;
    NX_PACKET *tx_packet = NX_NULL;

	nx_status = _TcpSocket_Send_Check(this);
	if (nx_status != NX_SUCCESS)
		return nx_status;
    this->SendThread = &this->Thread;
	do {
		nx_status = nx_packet_allocate(&NxPacketPool, &tx_packet, NX_TCP_PACKET, TX_WAIT_FOREVER);
		if (nx_status != NX_SUCCESS)
			break;
		/* append the message to send into the packet */
		nx_status = nx_packet_data_append(tx_packet, (VOID*)data_buffer, data_length, &NxPacketPool, TX_WAIT_FOREVER);
		if (nx_status != NX_SUCCESS)
			break;
		nx_status = _TcpSocket_Send(this, tx_packet);
	}while(0);
	if (tx_packet != NX_NULL)
		nx_packet_release(tx_packet);
	this->SendThread = NX_NULL;
	return nx_status;
}

/** -------------------------------------------------------------------------------------------------------------------
 *	_TcpSocket_BuildAsiMessagePacket
 *	ASIメッセージプロトコル用パケットの作成
*/
static UINT _TcpSocket_BuildAsiMessagePacket(NX_PACKET *tx_packet, const UCHAR *data_buffer, UINT data_length, NX_PACKET_POOL* pool, ULONG wait){
	#define AsiCommandMessage	 0x20
	UCHAR AsiPacketHeadder[] = { 0x02, 0x00, AsiCommandMessage, data_length & 0xff };
	UCHAR AsiPacketFooter[] = { 0x03, 0x00, 0x0d };
	UINT nx_status;
	UCHAR check_sum = 0x00;

	/* append the message to send into the packet */
	nx_status = nx_packet_data_append(tx_packet, (VOID*)AsiPacketHeadder, sizeof(AsiPacketHeadder), pool, wait);
	if (nx_status != NX_SUCCESS) return nx_status;

	for (int i = 0; i < sizeof(AsiPacketHeadder); ++i)
		check_sum += AsiPacketHeadder[i];

	/* append the message to send into the packet */
	nx_status = nx_packet_data_append(tx_packet, (VOID*)data_buffer, data_length, pool, wait);
	if (nx_status != NX_SUCCESS) return nx_status;

	for (int i = 0; i < data_length; ++i)
		check_sum += data_buffer[i];
	check_sum += AsiPacketFooter[0];
	AsiPacketFooter[1] = check_sum;

	/* append the message to send into the packet */
	nx_status = nx_packet_data_append(tx_packet, (VOID*)AsiPacketFooter, sizeof(AsiPacketFooter), pool, wait);
	return nx_status;
}

/** -------------------------------------------------------------------------------------------------------------------
 *	TcpSocket_SendMessage
 *	ASIメッセージプロトコルを送信する
*/
static UINT TcpSocket_SendMessage(TCP_TASK *this, const UCHAR *message, UINT message_length){
	UINT nx_status;
    NX_PACKET *tx_packet = NX_NULL;

	do {
		nx_status = _TcpSocket_Send_Check(this);
		if (nx_status != NX_SUCCESS)
			return nx_status;
	    this->SendThread = &this->Thread;

		nx_status = nx_packet_allocate(&NxPacketPool, &tx_packet, NX_TCP_PACKET, TX_WAIT_FOREVER);
		if (nx_status != NX_SUCCESS)
			break;

		/* append the message to send into the packet */
		nx_status = _TcpSocket_BuildAsiMessagePacket(tx_packet, message, message_length, &NxPacketPool, TX_WAIT_FOREVER);
		if (nx_status != NX_SUCCESS)
			break;
		nx_status = _TcpSocket_Send(this, tx_packet);
	}while(0);

	if (tx_packet != NX_NULL)
		nx_packet_release(tx_packet);
	this->SendThread = NX_NULL;
	return nx_status;
}


/* KeepAlive
 * NetXDuoのキープアライブ設定は動的には変更できない
 * 設定を変更するためには、NetXDuoライブラリの再コンパイルが必要
 *
 * デフォルトではキープアライブ機能は無効になっている
 * 有効にするにはnx_user.hの修正と再コンパイルが必要になる
 * 		NX_ENABLE_TCP_KEEPALIVE 		コメントを外すと有効になる
 * パラメータ
 *		NX_TCP_KEEPALIVE_INITIAL 	デフォルト7200(2h)->120(2m)
 *		NX_TCP_KEEPALIVE_RETRY 		デフォルト75(sec)->20(sec)
 *		NX_TCP_KEEPALIVE_RETRIES		デフォルト10->3
 */

/** -----------------------------------------------------------------------------------------------
* @brief  TCP thread entry.
* @param thread_input: thread user data
* @retval none
*/
static inline UINT TcpSocket_Create(TCP_TASK *this, CHAR *name){
	return nx_tcp_socket_create(
			&NetXDuoEthIpInstance,	// Pointer to previously created IP instance.
			&this->Socket,
			name,
			NX_IP_NORMAL,
			NX_FRAGMENT_OKAY,
			NX_IP_TIME_TO_LIVE,
			this->WindowSize, // window_size : 受信キューの最大バイト数
			NX_NULL,		// urgent_data_callback
			NX_NULL			// disconnect_callback
		);
}

/** -----------------------------------------------------------------------------------------------
* @brief  TCP server thread entry.
* @param thread_input: thread user data
* @retval none
*/
static VOID TcpServerThread_Entry(ULONG thread_input)
{
	TCP_TASK	*this = (TCP_TASK*)thread_input;
    UINT	step = 0;
	do {
		this->Active = NX_TRUE;
		LOGMSG_DEBUG("TCP Server Thread [%s] start.", this->Thread.tx_thread_name);

		ULONG actual_status;
		// Check status of an IP instance
		if (NX_SUCCESS != nx_ip_status_check(&NetXDuoEthIpInstance, NX_IP_INITIALIZE_DONE, &actual_status, NX_IP_PERIODIC_RATE))
			break;
		// 送信排他用セマフォを作成
		if (NX_SUCCESS != tx_semaphore_create(&this->TxSemaphore, "TX Semaphore", 1))
			break;
		step	= 1;
		// TCP Server Socket を作成する
		if (NX_SUCCESS != TcpSocket_Create(this, "TCP Server Socket"))
			break;
		step	= 2;
		// 指定TCPポートに対するリッスン要求を登録
		if (NX_SUCCESS != nx_tcp_server_socket_listen(&NetXDuoEthIpInstance, this->Port, &this->Socket, this->QueueMax, NX_NULL))
			break;
		LOGMSG_DEBUG("TCP Server Thread [%s] start listen port:%u.", this->Thread.tx_thread_name, this->Port);
		while (this->Active == NX_TRUE){
			LOGMSG_DEBUG("TCP Server Thread [%s] wait accept port:%u.", this->Thread.tx_thread_name, this->Port);
			step	= 3;
			// Client接続リクエストを受信
			if (NX_SUCCESS == nx_tcp_server_socket_accept(&this->Socket, NX_WAIT_FOREVER)){
				// 接続したピアIPアドレスとポート番号を取得
				ULONG RemotePort;
				if (NX_SUCCESS != nxd_tcp_socket_peer_info_get(&this->Socket, &this->RemoteIP, &RemotePort))
					break;
				this->RemotePort = (UINT)RemotePort;
				LOGMSG_DEBUG("TCP Server Thread [%s] connected.", this->Thread.tx_thread_name);
				PRINT_IP_ADDRESS_PORT(this->RemoteIP.nxd_ip_address.v4, this->RemotePort);
				step	= 4;
				// TCP受信処理
				while (this->Active == NX_TRUE){
					if (NX_SUCCESS != TcpSocket_Recieve(this))
						break;
				}
				nx_tcp_socket_disconnect(&this->Socket, DISCONNECT_WAIT);
			}
			nx_tcp_server_socket_unaccept(&this->Socket);
			if (NX_SUCCESS != nx_tcp_server_socket_relisten(&NetXDuoEthIpInstance, this->Port, &this->Socket))
				break;
		}
	} while(0);
	LOGMSG_DEBUG("TCP Server Thread [%s] exit loop.", this->Thread.tx_thread_name);
    this->Active = NX_FALSE;
    switch (step){
    	case 4:
			nx_tcp_server_socket_unaccept(&this->Socket);
		case 3:
		    nx_tcp_server_socket_unlisten(&NetXDuoEthIpInstance, this->Port);
		case 2:
			nx_tcp_socket_delete(&this->Socket);
		case 1:
    		tx_semaphore_delete(&this->TxSemaphore);
		case 0:
			break;
    }
}

/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
static void TcpServerThread_Cleanup(TCP_TASK* this)
{
    nx_tcp_socket_disconnect(&this->Socket, NX_NO_WAIT);
    nx_tcp_server_socket_unaccept(&this->Socket);
    nx_tcp_server_socket_unlisten(&NetXDuoEthIpInstance, this->Port);
    nx_tcp_socket_delete(&this->Socket);

    tx_thread_terminate(&this->Thread);
    tx_thread_delete(&this->Thread);

    tx_byte_release(this->StackSpace);
    this->StackSpace = NX_NULL;
}

/** -----------------------------------------------------------------------------------------------
* @brief  TCP client thread entry.
* @param thread_input: thread user data
* @retval none
*/
static VOID TcpClientThread_Entry(ULONG thread_input){
	TCP_TASK	*this = (TCP_TASK*)thread_input;
    UINT	step;
    do {
    	step   = 0;	// TCP Socket Create
		if (NX_SUCCESS != TcpSocket_Create(this, "TCP Client Socket"))
			break;
		step   = 1;	// bind the client socket for the DEFAULT_PORT
		if (NX_SUCCESS != nx_tcp_client_socket_bind(&this->Socket, this->Port, NX_WAIT_FOREVER))
			break;
		step   = 2;	// connect to the remote server on the specified port
		if (NX_SUCCESS != nxd_tcp_client_socket_connect(&this->Socket, &this->RemoteIP, this->RemotePort, NX_WAIT_FOREVER))
			break;
		// 送信排他用セマフォを作成
		if (NX_SUCCESS != tx_semaphore_create(&this->TxSemaphore, "TX Semaphore", 1))
			break;
		step   = 3; // 受信処理
		for (this->Active = 1; this->Active == 1; ){
			if (NX_SUCCESS != TcpSocket_Recieve(this))
				break;
		}
		nx_tcp_socket_disconnect(&this->Socket, DISCONNECT_WAIT);
    } while(0);

    this->Active = 0;
    switch (step){
    	case 3:
    		tx_semaphore_delete(&this->TxSemaphore);
		case 2:
			nx_tcp_client_socket_unbind(&this->Socket);
		case 1:
			nx_tcp_socket_delete(&this->Socket);
		case 0:
			break;
    }
}

/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
static void TcpClientThread_Cleanup(TCP_TASK* this)
{
    nx_tcp_socket_disconnect(&this->Socket, NX_NO_WAIT);
	nx_tcp_client_socket_unbind(&this->Socket);
    nx_tcp_socket_delete(&this->Socket);

    tx_thread_terminate(&this->Thread);
    tx_thread_delete(&this->Thread);

    tx_byte_release(this->StackSpace);
    this->StackSpace = NX_NULL;
}

/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
static UINT TcpThread_Create(TCP_TASK* this, CHAR* name, UINT tx_start, UINT priority,
			VOID (*thread_entry)(ULONG))
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
						name, //this->Name,
						thread_entry,
                        (ULONG)this,
						this->StackSpace, this->StackSize,
						priority,	//this->Priority, // 優先度（0～(TX_MAX_PRIORITIES-1)）
						priority,	//this->Priority, // 先取無効の最大優先度（0～(TX_MAX_PRIORITIES-1)）
						// この設定値より高い優先度のスレッドのみ先取が許可される。
                        TX_NO_TIME_SLICE, tx_start /*TX_AUTO_START*/
					)))
	{
	}
    return status;
}

/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
UINT TcpServerThread_Create(TCP_TASK* this, UINT port, CHAR* name, UINT tx_start, UINT priority)
{
	if (this == NX_NULL)
		return 0x8000;
	TX_MEMSET(this, '\0', sizeof(TCP_TASK));
//	this->Name				= name;
	this->StackSize 		= NX_APP_THREAD_STACK_SIZE;
//	this->Priority 			= priority;
	this->Port 				= port;
	this->WindowSize		= ASI_PACKET_SIZE;
	//this.RemoteIP;
	//this.RemotePort;
	this->QueueMax 			= 1;
	this->RecieveCallback 	= TcpSocket_Send;
	this->CleanUp			= TcpServerThread_Cleanup;
	this->SendPacket			= TcpSocket_Send;
	this->SendMessage		= TcpSocket_SendMessage;
	this->SendWaitTicks		= PERIODIC_MSEC(1000);
	this->Active 			= NX_FALSE;

	return TcpThread_Create(this, name, tx_start, priority, TcpServerThread_Entry);
}
/** ------------------------------------------------------------------------------------------------
 * @brief
 * @param
 * @retval
 */
UINT TcpClientThread_Create(TCP_TASK* this, ULONG remote_ip_address, UINT remote_port,
		CHAR* name, UINT tx_start, UINT priority)
{
	if (this == NX_NULL)
		return 0x8000;
	TX_MEMSET(this, '\0', sizeof(TCP_TASK));
//	this->Name				= name;
	this->StackSize 		= NX_APP_THREAD_STACK_SIZE;
//	this->Priority 			= priority;
	this->Port 				= NX_ANY_PORT;
	this->WindowSize		= ASI_PACKET_SIZE;
	this->RemoteIP.nxd_ip_version
							= NX_IP_VERSION_V4;
	this->RemoteIP.nxd_ip_address.v4
							= remote_ip_address;
	this->RemotePort		= remote_port;
	this->QueueMax 			= 1;
	this->RecieveCallback 	= TcpSocket_Send;
	this->CleanUp			= TcpClientThread_Cleanup;
	this->SendPacket			= TcpSocket_Send;
	this->SendMessage		= TcpSocket_SendMessage;
	this->Active 			= NX_FALSE;

	return TcpThread_Create(this, name, tx_start, priority, TcpClientThread_Entry);
}

