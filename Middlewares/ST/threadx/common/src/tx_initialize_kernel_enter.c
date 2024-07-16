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
/** ThreadX Component                                                     */
/**                                                                       */
/**   Initialize                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define TX_SOURCE_CODE


/* Include necessary system files.  */

#include "tx_api.h"
#include "tx_initialize.h"
#include "tx_thread.h"
#include "tx_timer.h"

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
extern VOID _tx_execution_initialize(VOID);
#endif

/* Define any port-specific scheduling data structures.  */

TX_PORT_SPECIFIC_DATA


#ifdef TX_SAFETY_CRITICAL
TX_SAFETY_CRITICAL_EXCEPTION_HANDLER
#endif


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_initialize_kernel_enter                         PORTABLE C      */
/*                                                           6.1.11       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    この関数は、初期化中に呼び出される最初の ThreadX 関数です。         */
/*    アプリケーションの "main() "関数から呼び出される。                  */
/*    このルーチンは決して戻らないことに注意することが重要です。          */
/*    この関数の処理は比較的単純で、いくつかの ThreadX 初期化関数を       */
/*    呼び出し（必要な場合）、 アプリケーション定義関数を呼び出し、       */
/*    そしてスケジューラを呼び出します。                                  */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _tx_initialize_low_level          Low-level initialization          */
/*    _tx_initialize_high_level         High-level initialization         */
/*    tx_application_define             Application define function       */
/*    _tx_thread_scheduler              ThreadX scheduling loop           */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    main                              Application main program          */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020      William E. Lamie        Initial Version 6.0           */
/*  09-30-2020      Yuxin Zhou              Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  04-25-2022      Scott Larson            Modified comment(s),          */
/*                                            added EPK initialization,   */
/*                                            resulting in version 6.1.11 */
/*                                                                        */
/**************************************************************************/
VOID  _tx_initialize_kernel_enter(VOID)
{

    /* コンパイラが ThreadX を事前に初期化しているかどうかを判断する。 */
    if (_tx_thread_system_state != TX_INITIALIZE_ALMOST_DONE)
    {
        /* No、初期化が必要 */

        /* 初期化が進行中であることを示すシステム状態変数を設定する
         * この変数は、後で割り込みのネストを表すために使用される  */
        _tx_thread_system_state =  TX_INITIALIZE_IN_PROGRESS;

        /* ポート固有の前処理を呼び出す  */
        TX_PORT_SPECIFIC_PRE_INITIALIZATION

        /* 低レベル初期化関数を呼び出して、プロセッサ固有の初期化をすべて処理する  */
        _tx_initialize_low_level();

        /* 高レベルの初期化関数を呼び出して、すべての ThreadX コンポーネントと
         * アプリケーションの初期化関数を実行する  */
        _tx_initialize_high_level();

        /* ポート固有の後処理を呼び出す  */
        TX_PORT_SPECIFIC_POST_INITIALIZATION
    }

    /* Optional processing extension.  */
    TX_INITIALIZE_KERNEL_ENTER_EXTENSION

    /* 初期化が進行中であることを示すシステム状態変数を設定する
     * この変数は、後で割り込みのネストを表すために使用される  */
    _tx_thread_system_state =  TX_INITIALIZE_IN_PROGRESS;

    /* アプリケーション初期化関数を呼び出し、利用可能な最初のメモリアドレスを渡す  */
    tx_application_define(_tx_initialize_unused_memory);

    /* Set the system state in preparation for entering the thread
       scheduler.  */
    _tx_thread_system_state =  TX_INITIALIZE_IS_FINISHED;

    /* Call any port specific pre-scheduler processing.  */
    TX_PORT_SPECIFIC_PRE_SCHEDULER_INITIALIZATION

#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    /* Initialize Execution Profile Kit.  */
    _tx_execution_initialize();
#endif

    /* スレッドの実行を開始するスケジューリング・ループに入る！ */
    _tx_thread_schedule();

#ifdef TX_SAFETY_CRITICAL

    /* If we ever get here, raise safety critical exception.  */
    TX_SAFETY_CRITICAL_EXCEPTION(__FILE__, __LINE__, 0);
#endif
}

