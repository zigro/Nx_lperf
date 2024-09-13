/*
 * nx_iperf_thread_tcp_tx.c
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

extern	ctrl_info		nx_iperf_ctrl_info;
extern	NX_IP			*nx_iperf_test_ip;
extern NX_PACKET_POOL	*nx_iperf_test_pool;
extern	ULONG			error_counter;

//static ULONG			thread_tcp_tx_counter;
static TX_THREAD   		thread_tcp_tx_iperf;
static NX_TCP_SOCKET   tcp_client_socket;

void nx_iperf_tcp_tx_cleanup(void)
{
    nx_tcp_socket_disconnect(&tcp_client_socket, NX_NO_WAIT);
    nx_tcp_client_socket_unbind(&tcp_client_socket);
    nx_tcp_socket_delete(&tcp_client_socket);

    tx_thread_terminate(&thread_tcp_tx_iperf);
    tx_thread_delete(&thread_tcp_tx_iperf);
}

void nx_iperf_thread_tcp_tx_entry(ULONG thread_input);

void nx_iperf_tcp_tx_test(UCHAR *stack_space, ULONG stack_size)
{
UINT status;

    status = tx_thread_create(&thread_tcp_tx_iperf, "thread tcp tx",
                              nx_iperf_thread_tcp_tx_entry,
                              (ULONG)&nx_iperf_ctrl_info,
                              stack_space, stack_size, NX_WEB_HTTP_SERVER_PRIORITY + 1, NX_WEB_HTTP_SERVER_PRIORITY + 1,
                              TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status)
    {
        nx_iperf_ctrl_info.ErrorCode = 1;
    }
    return;
}

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

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    thread_time = 0;
    isr_time = 0;
    idle_time = 0;
#endif

    ctrlInfo_ptr -> PacketsTxed = 0;
    ctrlInfo_ptr -> BytesTxed = 0;
    ctrlInfo_ptr -> ThroughPut = 0;
    ctrlInfo_ptr -> StartTime = 0;
    ctrlInfo_ptr -> RunTime = 0;
    ctrlInfo_ptr -> ErrorCode = 0;

    server_ip.nxd_ip_version = ctrlInfo_ptr -> version;

#ifdef FEATURE_NX_IPV6
    if (ctrlInfo_ptr -> version == NX_IP_VERSION_V6)
    {
        server_ip.nxd_ip_address.v6[0] = ctrlInfo_ptr -> ipv6[0];
        server_ip.nxd_ip_address.v6[1] = ctrlInfo_ptr -> ipv6[1];
        server_ip.nxd_ip_address.v6[2] = ctrlInfo_ptr -> ipv6[2];
        server_ip.nxd_ip_address.v6[3] = ctrlInfo_ptr -> ipv6[3];
    }
    else if (ctrlInfo_ptr -> version == NX_IP_VERSION_V4)
#endif
    {
#ifndef NX_DISABLE_IPV4
        server_ip.nxd_ip_address.v4 = ctrlInfo_ptr -> ip;
#endif
    }

    /* TCP Transmit Test Starts in 2 seconds.  */
    tx_thread_sleep(200);

    /* Create the socket.  */
    status =  nx_tcp_socket_create(nx_iperf_test_ip, &tcp_client_socket, "TCP Client Socket",
                                   NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 32 * 1024,
                                   NX_NULL, NX_NULL);
    if (status){
        error_counter++;
        return;
    }

    /* Bind the socket.  */
    status =  nx_tcp_client_socket_bind(&tcp_client_socket, NX_ANY_PORT, NX_WAIT_FOREVER);
    if (status){
        nx_tcp_socket_delete(&tcp_client_socket);
        error_counter++;
        return;
    }

    /* Attempt to connect the socket.  */
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

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    _tx_execution_thread_total_time_reset();
    _tx_execution_isr_time_reset();
    _tx_execution_idle_time_reset();
#endif

    /* Set the packet size.  */
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
        if (my_packet -> nx_packet_prepend_ptr + packet_size <= my_packet -> nx_packet_data_end)
        {
            my_packet -> nx_packet_append_ptr =  my_packet -> nx_packet_prepend_ptr + packet_size;
#ifndef NX_DISABLE_PACKET_CHAIN
            remaining_size = 0;
#endif /* NX_DISABLE_PACKET_CHAIN */
        }
        else{
#ifdef NX_DISABLE_PACKET_CHAIN
            packet_size = (ULONG)(my_packet -> nx_packet_data_end - my_packet -> nx_packet_prepend_ptr);
            my_packet -> nx_packet_append_ptr =  my_packet -> nx_packet_prepend_ptr + packet_size;
#else
            my_packet -> nx_packet_append_ptr = my_packet -> nx_packet_data_end;
            remaining_size = packet_size - (ULONG)(my_packet -> nx_packet_append_ptr - my_packet -> nx_packet_prepend_ptr);
            last_packet = my_packet;
#endif /* NX_DISABLE_PACKET_CHAIN */
        }
        my_packet -> nx_packet_length =  packet_size;

#ifndef NX_DISABLE_PACKET_CHAIN
        while (remaining_size){
            /* Allocate a packet.  */
            status =  nx_packet_allocate(nx_iperf_test_pool, &packet_ptr, NX_TCP_PACKET, NX_WAIT_FOREVER);
            if (status != NX_SUCCESS){
                break;
            }

            last_packet -> nx_packet_next = packet_ptr;
            last_packet = packet_ptr;
            if (remaining_size < (ULONG)(packet_ptr -> nx_packet_data_end - packet_ptr -> nx_packet_prepend_ptr))
            {
                packet_ptr -> nx_packet_append_ptr =  packet_ptr -> nx_packet_prepend_ptr + remaining_size;
            }
            else
            {
                packet_ptr -> nx_packet_append_ptr =  packet_ptr -> nx_packet_data_end;
            }
            remaining_size = remaining_size - (ULONG)(packet_ptr -> nx_packet_append_ptr - packet_ptr -> nx_packet_prepend_ptr);
        }
#endif /* NX_DISABLE_PACKET_CHAIN */

        if (is_first){
            memset(my_packet -> nx_packet_prepend_ptr, 0, (UINT)(my_packet -> nx_packet_data_end - my_packet -> nx_packet_prepend_ptr));
            is_first = NX_FALSE;
        }

        /* Send the packet out!  */
        status =  nx_tcp_socket_send(&tcp_client_socket, my_packet, NX_WAIT_FOREVER);
        /* ステータスが有効かどうかを確認します。  */
        if (status){
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
    ctrlInfo_ptr -> ThroughPut = ctrlInfo_ptr -> BytesTxed / ctrlInfo_ptr -> RunTime * NX_IP_PERIODIC_RATE / 125000;

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



