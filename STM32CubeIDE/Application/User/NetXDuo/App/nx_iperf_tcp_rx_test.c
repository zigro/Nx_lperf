/*
 * nx_iperf_tcp_rx_test.c
 *
 *  Created on: Sep 12, 2024
 *      Author: a-yam
 */
/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** NetX Utility                                                          */
/**                                                                       */
/**   NetX Duo IPerf Test Program                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#include   "tx_api.h"
#include   "nx_api.h"
#include   "nx_iperf.h"
#ifndef NX_WEB_HTTP_NO_FILEX
#include   "fx_api.h"
#else
#include   "filex_stub.h"
#endif

#include   "nx_web_http_server.h"

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
#include   "tx_execution_profile.h"
#endif /* TX_EXECUTION_PROFILE_ENABLE */

extern	TX_THREAD   		thread_tcp_rx_iperf;
extern	NX_TCP_SOCKET      tcp_server_socket;
extern	NX_TCP_SOCKET      tcp_client_socket;
extern	ctrl_info          nx_iperf_ctrl_info;
extern	NX_IP             *nx_iperf_test_ip;
extern	ULONG       error_counter;

void        nx_iperf_thread_tcp_rx_entry(ULONG thread_input);

void nx_iperf_tcp_rx_test(UCHAR *stack_space, ULONG stack_size)
{

UINT status;

    status = tx_thread_create(&thread_tcp_rx_iperf, "thread tcp rx",
                              nx_iperf_thread_tcp_rx_entry,
                              (ULONG)&nx_iperf_ctrl_info,
                              stack_space, stack_size, NX_IPERF_THREAD_PRIORITY, NX_IPERF_THREAD_PRIORITY,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status){
        nx_iperf_ctrl_info.ErrorCode = 1;
    }
    return;
}

/*---------------------------------------------------------------------------------------------------------------------
 * 	TCP受信スレッド
 * 	TCPサーバーソケットを作成しリッスン要求を待機
 */
void    nx_iperf_thread_tcp_rx_entry(ULONG thread_input)
{
UINT        status;
NX_PACKET  *packet_ptr;
ULONG       actual_status;
ULONG       expire_time;
ctrl_info  *ctrlInfo_ptr;
NXD_ADDRESS ip_address;
ULONG       port;

    /* Set the pointer.  */
    ctrlInfo_ptr = (ctrl_info *)thread_input;

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    /* Update the time.  */
    thread_time = 0;
    isr_time = 0;
    idle_time = 0;
#endif

    /* Update the test result.  */
    ctrlInfo_ptr -> PacketsRxed = 0;
    ctrlInfo_ptr -> BytesRxed = 0;
    ctrlInfo_ptr -> ThroughPut = 0;
    ctrlInfo_ptr -> StartTime = 0;
    ctrlInfo_ptr -> RunTime = 0;
    ctrlInfo_ptr -> ErrorCode = 0;

    // IPインスタンスが初期化されていることを確認します。
    // この機能は、必要な条件を満たすスレッドスリープを使用して、指定されたインターフェースのリンク状態をポーリングします。
    // 要求されたステータスがIPインスタンスにのみ存在する場合、例えばNX_IP_INITIALIZE_DONEの場合、このサービスは、そのステータスに対するIP設定を提供します。
    status =  nx_ip_status_check(nx_iperf_test_ip, NX_IP_INITIALIZE_DONE, &actual_status, NX_IP_PERIODIC_RATE);
    if (status != NX_SUCCESS){
        error_counter++;
        return;
    }

    // TCP Server Socket を作成する
    // この機能は、指定された IP インスタンス用の TCP ソケットを作成します。
    // このサービスにより、クライアントソケットとサーバソケットの両方が作成されます。
    status =  nx_tcp_socket_create(nx_iperf_test_ip, &tcp_server_socket, "TCP Server Socket",
                                   NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 32 * 1024,
                                   NX_NULL, nx_iperf_tcp_rx_disconnect_received);
    if (status){
        error_counter++;
        return;
    }

    // Setup this thread to listen.
    // 指定した TCP ポートに対するリッスン要求とサーバソケットを登録します。
    status =  nx_tcp_server_socket_listen(nx_iperf_test_ip, NX_IPERF_TCP_RX_PORT, &tcp_server_socket, 5, nx_iperf_tcp_rx_connect_received);
    if (status){
        nx_tcp_socket_delete(&tcp_server_socket);
        error_counter++;
        return;
    }

    /* Increment thread tcp rx's counter.  */
    thread_tcp_rx_counter++;

    // この関数は、アクティブなClient接続リクエストを受信した後にサーバーソケットを設定します。
    status =  nx_tcp_server_socket_accept(&tcp_server_socket, NX_WAIT_FOREVER);
    if (status){
        nx_tcp_server_socket_unlisten(nx_iperf_test_ip, NX_IPERF_TCP_RX_PORT);
        nx_tcp_socket_delete(&tcp_server_socket);
        error_counter++;
        return;
    }

    // この関数は、指定された TCP ソケットに接続されているピアの IP アドレスとポート番号を取得します。
    status = nxd_tcp_socket_peer_info_get(&tcp_server_socket, &ip_address, &port);
    if (status){
        nx_tcp_server_socket_unaccept(&tcp_server_socket);
        nx_tcp_server_socket_unlisten(nx_iperf_test_ip, NX_IPERF_TCP_RX_PORT);
        nx_tcp_socket_delete(&tcp_server_socket);
        error_counter++;
        return;
    }

    ctrlInfo_ptr -> version = ip_address.nxd_ip_version;
    if (ctrlInfo_ptr -> version == NX_IP_VERSION_V4)
    {
#ifndef NX_DISABLE_IPV4
        ctrlInfo_ptr -> ip = ip_address.nxd_ip_address.v4;
#endif
    }
#ifdef FEATURE_NX_IPV6
    else if (ctrlInfo_ptr -> version == NX_IP_VERSION_V6)
    {
        memcpy(ctrlInfo_ptr -> ipv6, ip_address.nxd_ip_address.v6, sizeof(ULONG) * 4); /* Use case of memcpy is verified. */
    }
#endif

    /* Set the test start time.  */
    ctrlInfo_ptr -> StartTime = tx_time_get();
    expire_time = (ULONG)(ctrlInfo_ptr -> StartTime + (ctrlInfo_ptr -> TestTime) + 20);

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    _tx_execution_thread_total_time_reset();
    _tx_execution_isr_time_reset();
    _tx_execution_idle_time_reset();
#endif

    while (tx_time_get() < expire_time)
    {
        /* Receive a TCP message from the socket.  */
        status =  nx_tcp_socket_receive(&tcp_server_socket, &packet_ptr, NX_WAIT_FOREVER);
        if (status){
            error_counter++;
            break;
        }

        /* Update the counter.  */
        ctrlInfo_ptr -> PacketsRxed++;
        ctrlInfo_ptr -> BytesRxed += packet_ptr -> nx_packet_length;

        /* Release the packet.  */
        nx_packet_release(packet_ptr);
    }

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    _tx_execution_thread_total_time_get(&thread_time);
    _tx_execution_isr_time_get(&isr_time);
    _tx_execution_idle_time_get(&idle_time);
#endif

    /* Calculate the test time and Throughput(Mbps).  */
    ctrlInfo_ptr -> RunTime = tx_time_get() - ctrlInfo_ptr -> StartTime;
    ctrlInfo_ptr -> ThroughPut = ctrlInfo_ptr -> BytesRxed / ctrlInfo_ptr -> RunTime * NX_IP_PERIODIC_RATE / 125000;

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    ctrlInfo_ptr -> idleTime = (ULONG)((unsigned long long)idle_time * 100 / ((unsigned long long)thread_time + (unsigned long long)isr_time + (unsigned long long)idle_time));
#endif

    /* Disconnect the server socket.  */
    status =  nx_tcp_socket_disconnect(&tcp_server_socket, 10);
    if (status){
        error_counter++;
    }

    /* Unaccept the server socket.  */
    status =  nx_tcp_server_socket_unaccept(&tcp_server_socket);
    status += nx_tcp_server_socket_unlisten(nx_iperf_test_ip, NX_IPERF_TCP_RX_PORT);
    if (status){
        error_counter++;
    }

    if (error_counter){
        ctrlInfo_ptr -> ErrorCode = error_counter;
    }

    /* Delete the socket.  */
    nx_tcp_socket_delete(&tcp_server_socket);
}
