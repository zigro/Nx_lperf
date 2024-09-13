/*
 * nx_iperf_udp_rx.c
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
extern	ULONG       	error_counter;

//static ULONG			thread_udp_rx_counter;
static TX_THREAD   		thread_udp_rx_iperf;
static NX_UDP_SOCKET   udp_server_socket;

void nx_iperf_udp_rx_cleanup(void)
{
    nx_udp_socket_unbind(&udp_server_socket);
    nx_udp_socket_delete(&udp_server_socket);

    tx_thread_terminate(&thread_udp_rx_iperf);
    tx_thread_delete(&thread_udp_rx_iperf);
}

void nx_iperf_thread_udp_rx_entry(ULONG thread_input);

void nx_iperf_udp_rx_test(UCHAR *stack_space, ULONG stack_size)
{
UINT status;

    status = tx_thread_create(&thread_udp_rx_iperf, "thread udp rx",
                              nx_iperf_thread_udp_rx_entry,
                              (ULONG)&nx_iperf_ctrl_info,
                              stack_space, stack_size, NX_WEB_HTTP_SERVER_PRIORITY + 1, NX_WEB_HTTP_SERVER_PRIORITY + 1,
                              TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status)
    {
        nx_iperf_ctrl_info.ErrorCode = 1;
    }
    return;
}

void nx_iperf_thread_udp_rx_entry(ULONG thread_input)
{
UINT        status;
ULONG       expire_time;
NX_PACKET  *my_packet;
ctrl_info  *ctrlInfo_ptr;
int         packetID = 0;
UINT        sender_port;
ULONG       tmp;
NXD_ADDRESS source_ip_address;

    /* Set the pointer.  */
    ctrlInfo_ptr = (ctrl_info *)thread_input;

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
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

    /* Create a UDP socket.  */
    status = nx_udp_socket_create(nx_iperf_test_ip, &udp_server_socket, "UDP Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, 0x80, 5);

    /* Check status.  */
    if (status)
    {
        error_counter++;
        return;
    }

    /* Bind the UDP socket to the IP port.  */
    status = nx_udp_socket_bind(&udp_server_socket, NX_IPERF_UDP_RX_PORT, TX_WAIT_FOREVER);

    /* Check status.  */
    if (status)
    {
        nx_udp_socket_delete(&udp_server_socket);
        error_counter++;
        return;
    }

    /* Disable checksum for UDP. */
    nx_udp_socket_checksum_disable(&udp_server_socket);

    /* Receive a UDP packet.  */
    status = nx_udp_socket_receive(&udp_server_socket, &my_packet, TX_WAIT_FOREVER);

    /* Check status.  */
    if (status)
    {
        nx_udp_socket_unbind(&udp_server_socket);
        nx_udp_socket_delete(&udp_server_socket);
        error_counter++;
        return;
    }

    /* Get source ip address*/
    nxd_udp_source_extract(my_packet, &source_ip_address, &sender_port);

    /* Set the IP address Version.  */
    ctrlInfo_ptr -> version = source_ip_address.nxd_ip_version;

#ifndef NX_DISABLE_IPV4
    if (ctrlInfo_ptr -> version == NX_IP_VERSION_V4)
    {
        ctrlInfo_ptr -> ip = source_ip_address.nxd_ip_address.v4;
    }
#endif
#ifdef FEATURE_NX_IPV6
    if (ctrlInfo_ptr -> version == NX_IP_VERSION_V6)
    {
        ctrlInfo_ptr -> ipv6[0] = source_ip_address.nxd_ip_address.v6[0];
        ctrlInfo_ptr -> ipv6[1] = source_ip_address.nxd_ip_address.v6[1];
        ctrlInfo_ptr -> ipv6[2] = source_ip_address.nxd_ip_address.v6[2];
        ctrlInfo_ptr -> ipv6[3] = source_ip_address.nxd_ip_address.v6[3];
    }
#endif

    /* Release the packet.  */
    nx_packet_release(my_packet);

    /* Set the test start time.  */
    ctrlInfo_ptr -> StartTime = tx_time_get();
    expire_time = (ULONG)(ctrlInfo_ptr -> StartTime + (ctrlInfo_ptr -> TestTime) + 5);   /* Wait 5 more ticks to synchronize. */

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    _tx_execution_thread_total_time_reset();
    _tx_execution_isr_time_reset();
    _tx_execution_idle_time_reset();
#endif

    while (tx_time_get() < expire_time)
    {

        /* Receive a UDP packet.  */
        status =  nx_udp_socket_receive(&udp_server_socket, &my_packet, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != NX_SUCCESS)
        {
            error_counter++;
            break;
        }

        /* Update the counter.  */
        ctrlInfo_ptr -> PacketsRxed++;
        ctrlInfo_ptr -> BytesRxed += my_packet -> nx_packet_length;

        /* Detect the end of the test signal. */
        packetID = *(int *)(my_packet -> nx_packet_prepend_ptr);

        tmp = (ULONG)packetID;
        NX_CHANGE_ULONG_ENDIAN(tmp);
        packetID = (int)tmp;


        /* Check the packet ID.  */
        if (packetID < 0)
        {

            /* Test has finished. */
#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
            _tx_execution_thread_total_time_get(&thread_time);
            _tx_execution_isr_time_get(&isr_time);
            _tx_execution_idle_time_get(&idle_time);
#endif

            /* Calculate the test time and Throughput.  */
            ctrlInfo_ptr -> RunTime = tx_time_get() - ctrlInfo_ptr -> StartTime;
            ctrlInfo_ptr -> ThroughPut = ctrlInfo_ptr -> BytesRxed / ctrlInfo_ptr -> RunTime * NX_IP_PERIODIC_RATE / 125000;

            /* received end of the test signal */

            /* Send the UDP packet.  */
            status = nxd_udp_socket_send(&udp_server_socket, my_packet, &source_ip_address, sender_port);

            /* Check the status.  */
            if (status)
            {

                /* Release the packet.  */
                nx_packet_release(my_packet);
            }
            else
            {

                /* Loop to receive the end of the test signal.  */
                while (1)
                {

                    /* Receive a UDP packet.  */
                    status =  nx_udp_socket_receive(&udp_server_socket, &my_packet, 20);

                    /* Check the status.  */
                    if (status)
                    {
                        break;
                    }

                    /* Send the UDP packet.  */
                    status = nxd_udp_socket_send(&udp_server_socket, my_packet, &source_ip_address, sender_port);

                    /* Check the status.  */
                    if (status)
                    {

                        /* Release the packet.  */
                        nx_packet_release(my_packet);
                    }
                }
            }
            break;
        }
        else
        {

            /* Release the packet.  */
            nx_packet_release(my_packet);
        }
    }

    if (packetID >= 0)
    {

        /* Test is not synchronized. */
#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
        _tx_execution_thread_total_time_get(&thread_time);
        _tx_execution_isr_time_get(&isr_time);
        _tx_execution_idle_time_get(&idle_time);
#endif

        /* Calculate the test time and Throughput.  */
        ctrlInfo_ptr -> RunTime = tx_time_get() - ctrlInfo_ptr -> StartTime;
        ctrlInfo_ptr -> ThroughPut = ctrlInfo_ptr -> BytesRxed / ctrlInfo_ptr -> RunTime * NX_IP_PERIODIC_RATE / 125000;
    }

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    ctrlInfo_ptr -> idleTime = (ULONG)((unsigned long long)idle_time * 100 / ((unsigned long long)thread_time + (unsigned long long)isr_time + (unsigned long long)idle_time));
#endif

    /* Unbind and Delete the socket.  */
    nx_udp_socket_unbind(&udp_server_socket);
    nx_udp_socket_delete(&udp_server_socket);

    /* Check error counter.  */
    if (error_counter)
    {
        ctrlInfo_ptr -> ErrorCode = error_counter;
    }
}


