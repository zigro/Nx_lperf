main.c main()
    app_thread.c: MX_ThreadX_Init();
          tx_kernel_enter(); ->
            tx_initialize_kernel_enter.c:_tx_initialize_kernel_enter()
                _tx_initialize_low_level();
                tx_initialize_high_level.c:_tx_initialize_high_level();
                    _tx_thread_initialize();
                    _tx_timer_initialize();
                    _tx_semaphore_initialize();
                    _tx_queue_initialize();
                    _tx_event_flags_initialize();
                    _tx_block_pool_initialize();
                    _tx_byte_pool_initialize();
                    _tx_mutex_initialize();
                app_azure_rtos.c:tx_application_define(_tx_initialize_unused_memory);
                    app_threadx.c:App_ThreadX_Init
                    app_filex.c:MX_FileX_Init
                        fx_system_initialize.c:fx_system_initialize
                    app_netxduo.c:MX_NetXDuo_Init
                        スレッド作成
                _tx_thread_schedule();
                
httpdemo.c:void tx_application_define(void *first_unused_memory)
httpdemo.c:VOID thread_client_entry(ULONG thread_input)
httpdemo.c:void thread_server_entry(ULONG thread_input)


nx_ipref.c:
    void        nx_iperf_tcp_rx_test(UCHAR *, ULONG);
    void        nx_iperf_tcp_tx_test(UCHAR *, ULONG);
    void        nx_iperf_udp_rx_test(UCHAR *, ULONG);
    void        nx_iperf_udp_tx_test(UCHAR *, ULONG);

    void        nx_iperf_tcp_rx_cleanup(void);
    void        nx_iperf_tcp_tx_cleanup(void);
    void        nx_iperf_udp_rx_cleanup(void);
    void        nx_iperf_udp_tx_cleanup(void);

    void        nx_iperf_test_info_parse(ctrl_info *iperf_ctrlInfo_ptr);
    void        nx_iperf_tcp_rx_connect_received(NX_TCP_SOCKET *socket_ptr, UINT port);
    void        nx_iperf_tcp_rx_disconnect_received(NX_TCP_SOCKET *socket_ptr);
    UINT        nx_iperf_get_notify(NX_WEB_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr);

    void        nx_iperf_entry(NX_PACKET_POOL *pool_ptr, NX_IP *ip_ptr, UCHAR *http_stack, ULONG http_stack_size, UCHAR *iperf_stack, ULONG iperf_stack_size);
        <   nx_app_thread_entry
            <   MX_NetXDuo_Init
                <   tx_application_define
                    <   _tx_initialize_kernel_enter
                        <   MX_ThreadX_Init
                            <   main()
        >   nx_web_http_server_create(&nx_iperf_web_server, "My HTTP Server", ip_ptr, NX_WEB_HTTP_SERVER_PORT, &nx_iperf_ram_disk, http_stack, http_stack_size, pool_ptr, nx_iperf_authentication_check, nx_iperf_get_notify);
        >   nx_web_http_server_start(&nx_iperf_web_server);

    UINT nx_iperf_authentication_check(struct NX_WEB_HTTP_SERVER_STRUCT *server_ptr, UINT request_type, CHAR *resource, CHAR **name, CHAR **password, CHAR **realm);
        <   nx_iperf_entry

    char* nx_iperf_get_ip_addr_string(NXD_ADDRESS *ip_address, UINT *string_length);

    //  この関数はトークン/値のペアを受け取り、その情報を ctrl_info_ptr に格納します。
    //  例えば、トークン/値のペアは「TestType」=「TC_Rx」となり、ctrl_info_ptrに情報が格納されます。
    static void nx_iperf_check_token_value(char *token, char *value_ptr, ctrl_info *ctrl_info_ptr)
        // この関数は、受信したHTTPコマンドラインを解析します。トークンと値のペアごとに、
        // nx_iperf_check_token_valueルーチンを呼び出して値を解析します。
        <   static void nx_iperf_parse_command(NX_PACKET *packet_ptr, ctrl_info *ctrl_info_ptr)
            <   nx_iperf_get_notify(NX_WEB_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr)
                <   nx_iperf_entry(NX_PACKET_POOL *pool_ptr, NX_IP *ip_ptr, UCHAR *http_stack, ULONG http_stack_size, UCHAR *iperf_stack, ULONG iperf_stack_size);
    
    void    nx_iperf_thread_tcp_rx_entry(ULONG thread_input);
        <   tx_thread_create(&thread_tcp_rx_iperf, "thread tcp rx", nx_iperf_thread_tcp_rx_entry, (ULONG)&nx_iperf_ctrl_info, stack_space, stack_size, NX_IPERF_THREAD_PRIORITY, NX_IPERF_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START)
                nx_iperf_tcp_rx_test(UCHAR *stack_space, ULONG stack_size)
            <   nx_iperf_tcp_rx_test(nx_iperf_stack_area, nx_iperf_stack_area_size);
                    nx_iperf_test_info_parse(ctrl_info *iperf_ctrlInfo_ptr)
                <   nx_iperf_test_info_parse(&nx_iperf_ctrl_info)
                        nx_iperf_get_notify(NX_WEB_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr)
                    <   nx_web_http_server_create(&nx_iperf_web_server, "My HTTP Server", ip_ptr, NX_WEB_HTTP_SERVER_PORT, &nx_iperf_ram_disk, http_stack, http_stack_size, pool_ptr, nx_iperf_authentication_check, nx_iperf_get_notify)
                        nx_iperf_entry(NX_PACKET_POOL *pool_ptr, NX_IP *ip_ptr, UCHAR *http_stack, ULONG http_stack_size, UCHAR *iperf_stack, ULONG iperf_stack_size)
                        

    void        nx_iperf_thread_tcp_tx_entry(ULONG thread_input);
    void        nx_iperf_thread_udp_tx_entry(ULONG thread_input);
    void        nx_iperf_thread_udp_rx_entry(ULONG thread_input);
