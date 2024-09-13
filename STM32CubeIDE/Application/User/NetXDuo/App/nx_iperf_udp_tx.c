/*
 * nx_iperf_udp_tx.c
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

extern	ctrl_info       nx_iperf_ctrl_info;
extern	NX_IP          *nx_iperf_test_ip;
extern NX_PACKET_POOL	*nx_iperf_test_pool;
extern	ULONG       	error_counter;
extern ULONG			_tx_timer_system_clock;

//static ULONG			thread_udp_tx_counter;
static TX_THREAD   		thread_udp_tx_iperf;
static NX_UDP_SOCKET	udp_client_socket;

void nx_iperf_udp_tx_cleanup(void)
{
    nx_udp_socket_unbind(&udp_client_socket);
    nx_udp_socket_delete(&udp_client_socket);
    tx_thread_terminate(&thread_udp_tx_iperf);
    tx_thread_delete(&thread_udp_tx_iperf);
}

void nx_iperf_thread_udp_tx_entry(ULONG thread_input);

void nx_iperf_udp_tx_test(UCHAR *stack_space, ULONG stack_size)
{
UINT status;

    status = tx_thread_create(&thread_udp_tx_iperf, "thread udp tx",
                              nx_iperf_thread_udp_tx_entry,
                              (ULONG)&nx_iperf_ctrl_info,
                              stack_space, stack_size, NX_WEB_HTTP_SERVER_PRIORITY + 1, NX_WEB_HTTP_SERVER_PRIORITY + 1,
                              TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status)
    {
        nx_iperf_ctrl_info.ErrorCode = 1;
    }
    return;
}

static void nx_iperf_send_udp_packet(int udp_id, ctrl_info *ctrlInfo_ptr)
{

UINT         status;
NX_PACKET   *my_packet = NX_NULL;
udp_payload *payload_ptr;
ULONG        tmp;

NXD_ADDRESS  server_ip;

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

    /* Send the end of test indicator. */
    nx_packet_allocate(nx_iperf_test_pool, &my_packet, NX_UDP_PACKET, TX_WAIT_FOREVER);
    my_packet -> nx_packet_append_ptr =  my_packet -> nx_packet_prepend_ptr + ctrlInfo_ptr -> PacketSize;

    payload_ptr = (udp_payload *)my_packet -> nx_packet_prepend_ptr;
    payload_ptr -> udp_id = udp_id;
    payload_ptr -> tv_sec = _tx_timer_system_clock / NX_IP_PERIODIC_RATE;
    payload_ptr -> tv_usec = _tx_timer_system_clock / NX_IP_PERIODIC_RATE * 1000000;

    tmp = (ULONG)payload_ptr -> udp_id;
    NX_CHANGE_ULONG_ENDIAN(tmp);
    payload_ptr -> udp_id = (int)tmp;

    NX_CHANGE_ULONG_ENDIAN(payload_ptr -> tv_sec);
    NX_CHANGE_ULONG_ENDIAN(payload_ptr -> tv_usec);

    /* Adjust the write pointer.  */
    my_packet -> nx_packet_length = (UINT)(ctrlInfo_ptr -> PacketSize);

    /* Send the UDP packet.  */
    status = nxd_udp_socket_send(&udp_client_socket, my_packet, &server_ip, ctrlInfo_ptr -> port);

    /* Check the status.  */
    if (status)
    {
        /* Release the packet.  */
        nx_packet_release(my_packet);
        return;
    }
}

void nx_iperf_thread_udp_tx_entry(ULONG thread_input)
{
UINT       status;
ULONG      expire_time;
ctrl_info *ctrlInfo_ptr;
NX_PACKET *my_packet;
int        i;
long       udp_id;

    /* Initialize the value.  */
    udp_id = 0;
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

    /* UDP Transmit Test Starts in 2 seconds.  */
    tx_thread_sleep(200);

    /* Create a UDP socket.  */
    status = nx_udp_socket_create(nx_iperf_test_ip, &udp_client_socket, "UDP Client Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, 0x80, 5);

    /* Check status.  */
    if (status)
    {
        error_counter++;
        return;
    }

    /* Bind the UDP socket to the IP port.  */
    status =  nx_udp_socket_bind(&udp_client_socket, NX_ANY_PORT, TX_WAIT_FOREVER);

    /* Check status.  */
    if (status)
    {
        nx_udp_socket_delete(&udp_client_socket);
        error_counter++;
        return;
    }

    /* Do not calculate checksum for UDP. */
    nx_udp_socket_checksum_disable(&udp_client_socket);

    /* Set the test start time.  */
    ctrlInfo_ptr -> StartTime = tx_time_get();
    expire_time = (ULONG)(ctrlInfo_ptr -> StartTime + (ctrlInfo_ptr -> TestTime));

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    _tx_execution_thread_total_time_reset();
    _tx_execution_isr_time_reset();
    _tx_execution_idle_time_reset();
#endif

    while (tx_time_get() < expire_time)
    {

        /* Write ABCs into the packet payload!  */
        nx_iperf_send_udp_packet(udp_id, ctrlInfo_ptr);

        /* Update the ID.  */
        udp_id = (udp_id + 1) & 0x7FFFFFFF;

        /* Update the counter.  */
        ctrlInfo_ptr -> PacketsTxed++;
        ctrlInfo_ptr -> BytesTxed += ctrlInfo_ptr -> PacketSize;
    }

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    _tx_execution_thread_total_time_get(&thread_time);
    _tx_execution_isr_time_get(&isr_time);
    _tx_execution_idle_time_get(&idle_time);
#endif

    /* Calculate the test time and Throughput.  */
    ctrlInfo_ptr -> RunTime = tx_time_get() - ctrlInfo_ptr -> StartTime;
    ctrlInfo_ptr -> ThroughPut = ctrlInfo_ptr -> BytesTxed / ctrlInfo_ptr -> RunTime * NX_IP_PERIODIC_RATE / 125000;

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    ctrlInfo_ptr -> idleTime = (ULONG)((unsigned long long)idle_time * 100 / ((unsigned long long)thread_time + (unsigned long long)isr_time + (unsigned long long)idle_time));
#endif

    if (error_counter)
    {
        ctrlInfo_ptr -> ErrorCode = error_counter;
    }

    ctrlInfo_ptr -> PacketSize = 100;
    for (i = 0; i < 10; i++)
    {
        /* Send the end of the test signal*/
        nx_iperf_send_udp_packet((0 - udp_id), ctrlInfo_ptr);

        /* Receive the packet.  s*/
        if (nx_udp_socket_receive(&udp_client_socket, &my_packet, 10) == NX_SUCCESS)
        {
            nx_packet_release(my_packet);
            break;
        }
    }

    /* Unbind and Delete the socket.  */
    nx_udp_socket_unbind(&udp_client_socket);
    nx_udp_socket_delete(&udp_client_socket);
}



