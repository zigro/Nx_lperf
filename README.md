
---
ページタイトル Readme
言語: ja
---
::: {.row}
::: {.col-sm-12 .col-lg-8}.

## <b>Nx_Iperf アプリケーションの説明</b>。

このアプリケーションは Azure RTOS NetXDuo スタックの使用例を提供します。
さまざまなモードを使用した場合のパフォーマンスを示します： TCP_server、UDP_server、TCP_client、UDP_client。
メインエントリ関数 tx_application_define() がカーネル起動中に ThreadX によって呼び出され、この段階ですべての NetXDuo リソースが作成されます。

 + NX_PACKET_POOL **NxAppPool** が割り当てられます。
 + そのプールを使用する NX_IP インスタンス **NetXDuoEthIpInstance** が初期化されます。
 + NX_PACKET_POOL **WebServerPool** が割り当てられます。
 + NX_IP インスタンスの ARP、ICMP、およびプロトコル（TCP および UDP）が有効化されます。
 + DHCP クライアントが作成されます。

アプリケーションが 1 つのスレッドを作成します：

 + NxAppThread** (priority 4, PreemtionThreashold 4) : <i>TX_AUTO_START</i> フラグで作成され、自動的に起動します。

NxAppThread**が起動し、以下の動作を実行します：

  + DHCP クライアントを起動します。
  + IP アドレスの解決を待機する。
  + nx_iperf_entry* を再開します。

nx_iperf_entry**が開始されると、以下のアクションを実行します：

  + NetXDuo Iperf デモ Web ページを作成します。

次に、アプリケーションは同じ優先順位で 4 つのスレッドを作成します：

   + thread_tcp_rx_iperf** (priority 1, PreemtionThreashold 1) : <i>TX_AUTO_START</i> フラグで作成され、自動的に開始する。
   + **thread_tcp_tx_iperf** (priority 1, PreemtionThreashold 1) : <i>TX_AUTO_START</i> フラグで作成され、自動的に開始する。
   + **thread_udp_rx_iperf** (priority 1, PreemtionThreashold 1) : <i>TX_AUTO_START</i> フラグで作成され、自動的に起動する。
   + **thread_udp_tx_iperf** (priority 1, PreemtionThreashold 1) : <i>TX_AUTO_START</i> フラグで作成され、自動的に開始する。

#### <b>期待される成功の動作</b> <b>期待される成功の動作</b> <b>期待される成功の動作</b

 + ボードのIPアドレスがハイパーターミナルに表示される。
 + ウェブHTTPサーバーが正常に開始されると、ユーザーはhttp://@IPというURLを入力した後、ウェブブラウザ上で
 　パフォーマンスをテストすることができます。
 + 各 Iperf テストを実行するには、以下のステップを実行し、このリンク 
 	https://docs.microsoft.com/en-us/azure/rtos/netx-duo/netx-duo-iperf/chapter3 に期待される結果を表示する必要があります。

#### <b>エラーの動作</b>

+ 赤色 LED が点滅し、エラーが発生したことを示す。

#### <b>もしあれば仮定</b>

- アプリケーションはIPアドレスを取得するためにDHCPを使用しているため、アプリケーションのテストに使用する
　LAN内のボードからDHCPサーバーに到達可能でなければなりません。
- 複数のボードが同じLANに接続されている場合は、潜在的なネットワークトラフィックの問題を避けるために、必ず変更してください。
- MACアドレスはmain.c`で定義されています。

```
void MX_ETH_Init(void)
{

  USER CODE BEGIN ETH_Init 0 */ /* USER CODE END ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

  USER CODE BEGIN ETH_Init 1 */ /* USER CODE END ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH.MACA
  heth.Init.MACAddr[0] = 0x00；
  heth.Init.MACAddr[1] = 0x80；
  heth.Init.MACAddr[2] = 0xE1；
  heth.Init.MACAddr[3] = 0x00；
  heth.Init.MACAddr[4] = 0x00；
  heth.Init.MACAddr[5] = 0x00；
```
#### <b>既知の制限事項</b>

  - パケットプールは最適化されていません。ファイル "app_netxduo.h" の NX_PACKET_POOL_SIZE とファイル "app_azure_rtos_config.h" の NX_APP_MEM_POOL_SIZE を小さくすることで、それ以下にすることができます。この更新により、NetXDuo のパフォーマンスが低下する可能性があります。


#### <b>ThreadX使用のヒント</b

 - ThreadX は Systick をタイムベースとして使用するため、HAL が TIM IP を介して別のタイムベースを使用することが必須です。
 - ThreadXはデフォルトで100 ticks/secで設定されており、アプリケーションで遅延やタイムアウトを使用する場合はこれを考慮する必要があります。tx_user.h "の "TX_TIMER_TICKS_PER_SECOND "定義でいつでも再設定できますが、これは "tx_initialize_low_level.S "ファイルにも反映されるべきです。
 - ThreadX は、予期せぬ動作を避けるために、カーネル起動中にすべての割り込みを無効にします。したがって、すべてのシステム関連コール (HAL、BSP) は、アプリケーションの最初か、スレッドエントリー関数の内部で行う必要があります。
 - ThreadX は、"tx_application_define()" 関数を提供しており、これは tx_kernel_enter() API によって自動的に呼び出されます。
   この関数を使用して、ThreadX 関連のリソース（スレッド、セマフォ、メモリプール...）を作成することを強く推奨しますが、どのような方法であれ、システム API 呼び出し（HAL または BSP）を含むべきではありません。
 - 動的メモリ割り当てを使用するには、リンカファイルにいくつかの変更を加える必要があります。
   ThreadX は、RAM 上の最初の空きメモリ位置へのポインタを tx_application_define() 関数に渡す必要があります、
   引数 "first_unused_memory "を使用する。
   このため、リンカ・ファイルを変更し、このメモリ位置を公開する必要があります。
    + EWARMの場合は、.icfファイルに以下のセクションを追加します：
     ```
	 RAM_region { last section FREE_MEM } に配置します；
	 ```
    + MDK-ARM の場合：
	```
    RW_IRAM1領域を".sct "ファイルに定義するか
    または、"tx_initialize_low_level.S "の以下の行を、使用するメモリー領域に合わせて修正する。
        LDR r1, =|Image$$RW_IRAM1$$ZI$$Limit|。
	```
    + STM32CubeIDE の場合は、.ld ファイルに以下のセクションを追加してください：
	```
    ._threadx_heap ：
      {
         .= ALIGN(8)；
         __RAM_segment_used_end__ = ；
         . = . + 64K;
         . = ALIGN(8)；
       } >RAM AT> RAM
	```

       ThreadX用のメモリを提供する最も簡単な方法は、上記の._threadx_heapを参照して、新しいセクションを定義することである。
       上記の例では、ThreadXのヒープ・サイズは64KBytesに設定されている。
       ._threadx_heap は、リンカ・スクリプトの .bss セクションと ._user_heap_stack セクションの間に配置する必要があります。
       注意： ThreadX が提供されたヒープ・メモリ（この例では 64KBytes）以上を必要としないことを確認してください。
       詳細は、『STM32CubeIDE User Guide』の章を参照してください： 「リンカ・スクリプト" を参照してください。

    + "USE_DYNAMIC_MEMORY_ALLOCATION "フラグを有効にするために、"tx_initialize_low_level.S "も修正する必要があります。

### キーワード

RTOS、ネットワーク、ThreadX、NetXDuo、Iperf、UART

### ハードウェアおよびソフトウェア環境

  - このアプリケーションはSTM32H563xxデバイス上で動作します。
  - このアプリケーションはSTMicroelectronics STM32H563G-DKボードでテストされています： MB1520-H563I-B02
    このアプリケーションは、STマイクロエレクトロニクスのSTM32H563G-DKボードでテストされています。

  - このアプリケーションはログを表示するためにUSART3を使用します：
      - ボーレート = 115200 ボー
      - ワード長 = 8ビット
      - ストップビット = 1
      - パリティ = なし
      - フロー制御 = なし

### <b>どのように使うか？

このプログラムを動作させるには、以下を実行する必要があります：

 - お好みのツールチェインを開く
 - すべてのファイルを再構築し、イメージをターゲット・メモリにロードする。
 - アプリケーションを実行する
 - このリンク https://docs.microsoft.com/en-us/azure/rtos/netx-duo/netx-duo-iperf/chapter3 に記載されている手順に従って、[iperf tool]を実行します。

:::
:::

---
pagetitle: Readme
lang: en
---
::: {.row}
::: {.col-sm-12 .col-lg-8}

##  <b>Nx_Iperf Application Description</b>

This application provides an example of Azure RTOS NetXDuo stack usage .
It shows performance when using different modes : TCP_server, UDP_server, TCP_client and UDP_client.
The main entry function tx_application_define() is then called by ThreadX during kernel start, at this stage, all NetXDuo resources are created.

 + A NX_PACKET_POOL **NxAppPool** is allocated
 + A NX_IP instance **NetXDuoEthIpInstance** using that pool is initialized
 + A NX_PACKET_POOL **WebServerPool** is allocated
 + The ARP, ICMP and protocols (TCP and UDP) are enabled for the NX_IP instance
 + A DHCP client is created.

The application creates 1 thread :

 + **NxAppThread** (priority 4, PreemtionThreashold 4) : created with the <i>TX_AUTO_START</i> flag to start automatically.

The **NxAppThread** starts and perform the following actions:

  + Starts the DHCP client
  + Waits for the IP address resolution
  + Resumes the *nx_iperf_entry*

The **nx_iperf_entry**, once started:

  + Creates a NetXDuo Iperf demo web page.

The application then creates 4 threads with the same priorities :

   + **thread_tcp_rx_iperf** (priority 1, PreemtionThreashold 1) : created with the <i>TX_AUTO_START</i> flag to start automatically.
   + **thread_tcp_tx_iperf** (priority 1, PreemtionThreashold 1) : created with the <i>TX_AUTO_START</i> flag to start automatically.
   + **thread_udp_rx_iperf** (priority 1, PreemtionThreashold 1) : created with the <i>TX_AUTO_START</i> flag to start automatically.
   + **thread_udp_tx_iperf** (priority 1, PreemtionThreashold 1) : created with the <i>TX_AUTO_START</i> flag to start automatically.

####  <b>Expected success behavior</b>

 + The board IP address is printed on the HyperTerminal
 + When the Web HTTP server is successfully started.Then the user can test the performance on the web browser after entring the url http://@IP.
 + To execute each Iperf test you must do the following steps and have the expected result in this link https://docs.microsoft.com/en-us/azure/rtos/netx-duo/netx-duo-iperf/chapter3 .

#### <b>Error behaviors</b>

+ The Red LED is toggling to indicate any error that have occurred.

#### <b>Assumptions if any</b>

- The Application is using the DHCP to acquire IP address, thus a DHCP server should be reachable by the board in the LAN used to test the application.
- The application is configuring the Ethernet IP with a static predefined <i>MAC Address</i>, make sure to change it in case multiple boards are connected on the same LAN to avoid any potential network traffic issues.
- The _MAC Address_ is defined in the main.c`

```
void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  heth.Init.MACAddr[0] =   0x00;
  heth.Init.MACAddr[1] =   0x80;
  heth.Init.MACAddr[2] =   0xE1;
  heth.Init.MACAddr[3] =   0x00;
  heth.Init.MACAddr[4] =   0x00;
  heth.Init.MACAddr[5] =   0x00;
```
#### <b>Known limitations</b>

  - The packet pool is not optimized. It can be less than that by reducing NX_PACKET_POOL_SIZE in file "app_netxduo.h" and NX_APP_MEM_POOL_SIZE in file "app_azure_rtos_config.h". This update can decrease NetXDuo performance.


#### <b>ThreadX usage hints</b>

 - ThreadX uses the Systick as time base, thus it is mandatory that the HAL uses a separate time base through the TIM IPs.
 - ThreadX is configured with 100 ticks/sec by default, this should be taken into account when using delays or timeouts at application. It is always possible to reconfigure it in the "tx_user.h", the "TX_TIMER_TICKS_PER_SECOND" define,but this should be reflected in "tx_initialize_low_level.S" file too.
 - ThreadX is disabling all interrupts during kernel start-up to avoid any unexpected behavior, therefore all system related calls (HAL, BSP) should be done either at the beginning of the application or inside the thread entry functions.
 - ThreadX offers the "tx_application_define()" function, that is automatically called by the tx_kernel_enter() API.
   It is highly recommended to use it to create all applications ThreadX related resources (threads, semaphores, memory pools...)  but it should not in any way contain a system API call (HAL or BSP).
 - Using dynamic memory allocation requires to apply some changes to the linker file.
   ThreadX needs to pass a pointer to the first free memory location in RAM to the tx_application_define() function,
   using the "first_unused_memory" argument.
   This require changes in the linker files to expose this memory location.
    + For EWARM add the following section into the .icf file:
     ```
	 place in RAM_region    { last section FREE_MEM };
	 ```
    + For MDK-ARM:
	```
    either define the RW_IRAM1 region in the ".sct" file
    or modify the line below in "tx_initialize_low_level.S to match the memory region being used
        LDR r1, =|Image$$RW_IRAM1$$ZI$$Limit|
	```
    + For STM32CubeIDE add the following section into the .ld file:
	```
    ._threadx_heap :
      {
         . = ALIGN(8);
         __RAM_segment_used_end__ = .;
         . = . + 64K;
         . = ALIGN(8);
       } >RAM AT> RAM
	```

       The simplest way to provide memory for ThreadX is to define a new section, see ._threadx_heap above.
       In the example above the ThreadX heap size is set to 64KBytes.
       The ._threadx_heap must be located between the .bss and the ._user_heap_stack sections in the linker script.
       Caution: Make sure that ThreadX does not need more than the provided heap memory (64KBytes in this example).
       Read more in STM32CubeIDE User Guide, chapter: "Linker script".

    + The "tx_initialize_low_level.S" should be also modified to enable the "USE_DYNAMIC_MEMORY_ALLOCATION" flag.

### <b>Keywords</b>

RTOS, Network, ThreadX, NetXDuo, Iperf, UART

### <b>Hardware and Software environment</b>

  - This application runs on STM32H563xx devices
  - This application has been tested with STMicroelectronics STM32H563G-DK boards Revision: MB1520-H563I-B02
    and can be easily tailored to any other supported device and development board.

  - This application uses USART3 to display logs, the hyperterminal configuration is as follows:
      - BaudRate = 115200 baud
      - Word Length = 8 Bits
      - Stop Bit = 1
      - Parity = None
      - Flow control = None

###  <b>How to use it ?</b>

In order to make the program work, you must do the following :

 - Open your preferred toolchain
 - Rebuild all files and load your image into target memory
 - Run the application
 - Run the [iperf tool] by following steps described in this link https://docs.microsoft.com/en-us/azure/rtos/netx-duo/netx-duo-iperf/chapter3 .

:::
:::

