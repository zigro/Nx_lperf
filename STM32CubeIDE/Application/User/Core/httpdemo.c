/* This is a small demo of the NetX Duo HTTP Client Server API running on a
   high-performance NetX Duo TCP/IP stack. This demo is applicable for
   either IPv4 or IPv6 enabled applications. */

#include   "tx_api.h"
#include   "fx_api.h"
#include   "nx_api.h"
#include   "nx_web_http_client.h"
#include   "nx_web_http_server.h"

#define     DEMO_STACK_SIZE         2048

/* Set up FileX and file memory resources. */
CHAR            *ram_disk_memory;
FX_MEDIA        ram_disk;
unsigned char   media_memory[512];

/* Define device drivers. */
extern void _fx_ram_driver(FX_MEDIA *media_ptr);
VOID        _nx_ram_network_driver(NX_IP_DRIVER *driver_req_ptr);

#define USE_DUO		/* Use the duo service (not legacy netx) */
#define IPTYPE   4       /* Send packets over IPv6 */

/* Set up the HTTP client. */
TX_THREAD       client_thread;
NX_PACKET_POOL  client_pool;
NX_WEB_HTTP_CLIENT  my_client;
NX_IP           client_ip;
#define         CLIENT_PACKET_SIZE  (NX_HTTP_SERVER_MIN_PACKET_SIZE * 2)
void            thread_client_entry(ULONG thread_input);

#define HTTP_SERVER_ADDRESS  IP_ADDRESS(172,16,51,80)
#define HTTP_CLIENT_ADDRESS  IP_ADDRESS(255,255,0,0)

/* Set up the HTTP server */

NX_WEB_HTTP_SERVER  my_server;
NX_PACKET_POOL  server_pool;
TX_THREAD       server_thread;
NX_IP           server_ip;
#define         SERVER_PACKET_SIZE  (NX_WEB_HTTP_SERVER_MIN_PACKET_SIZE * 2)

void            thread_server_entry(ULONG thread_input);
#ifdef FEATURE_NX_IPV6
NXD_ADDRESS     server_ip_address;
#endif


/* Define the application's authentication check. This is called by
   the HTTP server whenever a new request is received. */
UINT  authentication_check(NX_WEB_HTTP_SERVER *server_ptr, UINT request_type,
            CHAR *resource, CHAR **name, CHAR **password, CHAR **realm)
{

    /* Just use a simple name, password, and realm for all
       requests and resources. */
    *name =     "name";
    *password = "password";
    *realm =    "NetX Duo HTTP demo";

    /* Request basic authentication. */
    return(NX_WEB_HTTP_BASIC_AUTHENTICATE);
}

/* Define main entry point. */

//int main()
//{
//
//    /* Enter the ThreadX kernel. */
//    tx_kernel_enter();
//}


/* Define what the initial system looks like. */
void    tx_application_define(void *first_unused_memory)
{

CHAR    *pointer;
UINT    status;


    /* Setup the working pointer. */
    pointer =  (CHAR *) first_unused_memory;

    /* Create a helper thread for the server. */
    tx_thread_create(&server_thread, "HTTP Server thread", thread_server_entry, 0,
                     pointer, DEMO_STACK_SIZE,
                     1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;

    /* Initialize the NetX system. */
    nx_system_initialize();

    /* Create the server packet pool. */
    status =  nx_packet_pool_create(&server_pool, "HTTP Server Packet Pool",
		SERVER_PACKET_SIZE, pointer, SERVER_PACKET_SIZE*4);


    pointer = pointer + SERVER_PACKET_SIZE * 4;

    /* Check for pool creation error. */
    if (status)
    {

        return;
    }

    /* Create an IP instance. */
    status = nx_ip_create(&server_ip, "HTTP Server IP", HTTP_SERVER_ADDRESS,
                          0xFFFFFF00UL, &server_pool, _nx_ram_network_driver,
                          pointer, 4096, 1);

    pointer =  pointer + 4096;

    /* Check for IP create errors. */
    if (status)
    {
        printf("nx_ip_create failed. Status 0x%x\n", status);
        return;
    }

    /* Enable ARP and supply ARP cache memory for the server IP instance. */
    status = nx_arp_enable(&server_ip, (void *) pointer, 1024);

    /* Check for ARP enable errors. */
    if (status)
    {
        return;
    }

    pointer = pointer + 1024;

     /* Enable TCP traffic. */
    status = nx_tcp_enable(&server_ip);

    if (status)
    {
        return;
    }

#if (IP_TYPE==6)

    /* Set up HTTPv6 server, but we have to wait till its address has been
       validated before we can start the thread_server_entry thread. */

    /* Set up the server's IPv6 address here. */
    server_ip_address.nxd_ip_address.v6[3] = 0x105;
    server_ip_address.nxd_ip_address.v6[2] = 0x0;
    server_ip_address.nxd_ip_address.v6[1] = 0x0000f101;
    server_ip_address.nxd_ip_address.v6[0] = 0x20010db8;
    server_ip_address.nxd_ip_version = NX_IP_VERSION_V6;

#endif

    /* Create the NetX HTTP Server. */
    status = nx_web_http_server_create(&my_server, "My HTTP Server", &server_ip,80,
			&ram_disk, pointer, 2048, &server_pool, authentication_check,
			NX_NULL);

    if (status)
    {
        return;
    }

    pointer =  pointer + 2048;

    /* Save the memory pointer for the RAM disk. */
    ram_disk_memory =  pointer;

    /* Create the HTTP client thread. */
    status = tx_thread_create(&client_thread, "HTTP Client", thread_client_entry, 0,
                     pointer, DEMO_STACK_SIZE,
                     2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;

    /* Check for thread create error. */
    if (status)
    {

        return;
    }

    /* Create the Client packet pool. */
    status =  nx_packet_pool_create(&client_pool, "HTTP Client Packet Pool",SERVER_PACKET_SIZE, pointer, SERVER_PACKET_SIZE*4);



    pointer = pointer + SERVER_PACKET_SIZE * 4;

    /* Check for pool creation error. */
    if (status)
    {

        return;
    }


    /* Create an IP instance. */
    status = nx_ip_create(&client_ip, "HTTP Client IP", HTTP_CLIENT_ADDRESS,
                          0xFFFFFF00UL, &client_pool, _nx_ram_network_driver,
                          pointer, 2048, 1);

    pointer =  pointer + 2048;

    /* Check for IP create errors. */
    if (status)
    {
        return;
    }

    nx_arp_enable(&client_ip, (void *) pointer, 1024);

    pointer =  pointer + 2048;

     /* Enable TCP traffic. */
    nx_tcp_enable(&client_ip);

    return;
}


VOID thread_client_entry(ULONG thread_input)
{

UINT            status;
NX_PACKET       *my_packet;
#ifdef FEATURE_NX_IPV6
NXD_ADDRESS     client_ip_address;
UINT            address_index;
#endif


    /* Format the RAM disk - the memory for the RAM disk was setup in
      tx_application_define above. This must be set up before the client(s) start
      sending requests. */
    status = fx_media_format(&ram_disk,
                            _fx_ram_driver,         // Driver entry
                            ram_disk_memory,        // RAM disk memory pointer
                            media_memory,           // Media buffer pointer
                            sizeof(media_memory),   // Media buffer size
                            "MY_RAM_DISK",          // Volume Name
                            1,                      // Number of FATs
                            32,                     // Directory Entries
                            0,                      // Hidden sectors
                            256,                    // Total sectors
                            128,                    // Sector size
                            1,                      // Sectors per cluster
                            1,                      // Heads
                            1);                     // Sectors per track

    /* Check the media format status. */
    if (status != FX_SUCCESS)
    {

        /* Error, bail out. */
        return ;
    }

    /* Open the RAM disk. */
    status =  fx_media_open(&ram_disk, "RAM DISK", _fx_ram_driver, ram_disk_memory,
				media_memory, sizeof(media_memory));

    /* Check the media open status. */
    if (status != FX_SUCCESS)
    {

        /* Error, bail out. */
        return ;
    }

    /* Create an HTTP client instance. */
    status = nx_web_http_client_create(&my_client, "HTTP Client", &client_ip,
					&client_pool, 600);

    /* Check status. */
    if (status != NX_SUCCESS)
    {
        return;
    }

    /* Attempt to upload a file to the HTTP server. */


#if (IPTYPE== 6)

    /* Relinquish control so the HTTP server can get set up...*/
    tx_thread_relinquish();

    /* Set up the client's IPv6 address here. */
    client_ip_address.nxd_ip_address.v6[3] = 0x101;
    client_ip_address.nxd_ip_address.v6[2] = 0x0;
    client_ip_address.nxd_ip_address.v6[1] = 0x0000f101;
    client_ip_address.nxd_ip_address.v6[0] = 0x20010db1;
    client_ip_address.nxd_ip_version = NX_IP_VERSION_V6;

    /* Here's where we make the HTTP Client IPv6 enabled. */

    nxd_ipv6_enable(&client_ip);
    nxd_icmp_enable(&client_ip);

    /* Wait till the IP task thread has set the device MAC address. */
    tx_thread_sleep(100);

    /* Now update NetX Duo the Client's link local and global IPv6 address. */
    nxd_ipv6_address_set(&server_ip, 0, NX_NULL, 10, &address_index)
    nxd_ipv6_address_set(&server_ip, 0, &client_ip_address, 64, &address_index);

    /* Then make sure NetX Duo has had time to validate the addresses. */

    tx_thread_sleep(400);

    /* Now upload an HTML file to the HTTPv6 server. */
    status =  nx_web_http_client_put_start(&my_client, &server_ip_address,
			"/client_test.htm", "name", "password", 103, 500);


    /* Check status. */
    if (status != NX_SUCCESS)
    {

        return;
    }


#else

    /* Relinquish control so the HTTP server can get set up...*/
    tx_thread_relinquish();

    do
    {

        /* Attempt to upload to the HTTP IPv4 server. */
        status =  nx_web_http_client_put_start(&my_client, &HTTP_SERVER_ADDRESS,80,
			"/client_test.htm", "name", "password", 103, 500);


        /* Check status. */
        if (status != NX_SUCCESS)
        {
            tx_thread_sleep(100);
        }

    } while (status != NX_SUCCESS);


#endif  /* (IPTYPE== 6) */


    /* Allocate a packet. */
    status =  nx_packet_allocate(&client_pool, &my_packet, NX_TCP_PACKET,
						NX_WAIT_FOREVER);

    /* Check status. */
    if (status != NX_SUCCESS)
    {
        return;
    }

    /* Build a simple 103-byte HTML page. */
    nx_packet_data_append(my_packet, "<HTML>\r\n", 8,
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(my_packet,
                 "<HEAD><TITLE>NetX HTTP Test</TITLE></HEAD>\r\n", 44,
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(my_packet, "<BODY>\r\n", 8,
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(my_packet, "<H1>Another NetX Test Page!</H1>\r\n", 25,
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(my_packet, "</BODY>\r\n", 9,
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(my_packet, "</HTML>\r\n", 9,
                        &client_pool, NX_WAIT_FOREVER);

    /* Complete the PUT by writing the total length. */
    status =  nx_web_http_client_put_packet(&my_client, my_packet, 50);

    /* Check status. */
    if (status != NX_SUCCESS)
    {
        return;
    }

    /* Now GET the test file  */

#ifdef USE_DUO

    status =  nx_web_http_client_get_start(&my_client, &server_ip_address,
                   "/client_test.htm", NX_NULL, 0, "name", "password", 50);
#else

    status =  nx_http_client_get_start(&my_client, HTTP_SERVER_ADDRESS,
		         "/client_test.htm",  NX_NULL, 0, "name", "password", 50);

#endif

    /* Check status. */
    if (status != NX_SUCCESS)
    {
        return;
    }

    status = nx_web_http_client_delete(&my_client);

    return;
}

/* Define the helper HTTP server thread. */
void    thread_server_entry(ULONG thread_input)
{

UINT            status;
#if (IPTYPE == 6)
UINT            address_index
NXD_ADDRESS     ip_address

    /* Allow time for the IP task to initialize the driver. */
    tx_thread_sleep(100);

  ip_address.nxd_ip_version = NX_IP_VERSION_V6;
  ip_address.nxd_ip_address.v6[0] = 0x20010000;
  ip_address.nxd_ip_address.v6[1] = 0;
  ip_address.nxd_ip_address.v6[2] = 0;
  ip_address.nxd_ip_address.v6[3] = 4;

    /* Here's where we make the HTTP server IPv6 enabled. */
    nxd_ipv6_enable(&server_ip);
    nxd_icmp_enable(&server_ip);

    /* Wait till the IP task thread has set the device MAC address. */
    while (server_ip.nx_ip_arp_physical_address_msw == 0 ||
           server_ip.nx_ip_arp_physical_address_lsw == 0)
    {
        tx_thread_sleep(30);
    }

    nxd_ipv6_address_set(&server_ip, 0, NX_NULL, 10, &address_index)
    nxd_ipv6_ address_set(&server_ip, 0, &ip_address, 64, &address_index);

    /* Wait for NetX Duo to validate server address. */
    tx_thread_sleep(400);

#endif  /* (IPTYPE == 6) */

    /* OK to start the HTTPv6 Server. */
    status = nx_web_http_server_start(&my_server);

    if (status != NX_SUCCESS)
    {
        return;
    }

    /* HTTP server ready to take requests! */

    /* Let the IP threads execute. */
    tx_thread_relinquish();

    return;
}
