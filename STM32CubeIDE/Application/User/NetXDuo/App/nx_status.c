/*
 * nx_status.c
 *
 *  Created on: Sep 20, 2024
 *      Author: a-yam
 */

#include "app_netxduo.h"
#include "logmsg.h"

typedef struct CODENAME_Struct {
	const UCHAR code;
	const CHAR* const name;
} CODENAME;

#define DefValue(code)		{ code, #code }

static const CHAR* const Get_CodeName(UINT code, const CODENAME table[], const UINT count){
	for (UINT i=0; i<count; ++i){
		if (table[i].code == code)
			return table[i].name;
	}
	return "UNDEFINED CODE";
}


const CODENAME NxStatus[] = {
		DefValue( NX_SUCCESS              ),
		DefValue( NX_NO_PACKET            ),
		DefValue( NX_UNDERFLOW            ),
		DefValue( NX_OVERFLOW             ),
		DefValue( NX_NO_MAPPING           ),
		DefValue( NX_DELETED              ),
		DefValue( NX_POOL_ERROR           ),
		DefValue( NX_PTR_ERROR            ),
		DefValue( NX_WAIT_ERROR           ),
		DefValue( NX_SIZE_ERROR           ),
		DefValue( NX_OPTION_ERROR         ),
		DefValue( NX_DELETE_ERROR         ),
		DefValue( NX_CALLER_ERROR         ),
		DefValue( NX_INVALID_PACKET       ),
		DefValue( NX_INVALID_SOCKET       ),
		DefValue( NX_NOT_ENABLED          ),
		DefValue( NX_ALREADY_ENABLED      ),
		DefValue( NX_ENTRY_NOT_FOUND      ),
		DefValue( NX_NO_MORE_ENTRIES      ),
		DefValue( NX_ARP_TIMER_ERROR      ),
		DefValue( NX_RESERVED_CODE0       ),
		DefValue( NX_WAIT_ABORTED         ),
		DefValue( NX_IP_INTERNAL_ERROR    ),
		DefValue( NX_IP_ADDRESS_ERROR     ),
		DefValue( NX_ALREADY_BOUND        ),
		DefValue( NX_PORT_UNAVAILABLE     ),
		DefValue( NX_NOT_BOUND            ),
		DefValue( NX_RESERVED_CODE1       ),
		DefValue( NX_SOCKET_UNBOUND       ),
		DefValue( NX_NOT_CREATED          ),
		DefValue( NX_SOCKETS_BOUND        ),
		DefValue( NX_NO_RESPONSE          ),
		DefValue( NX_POOL_DELETED         ),
		DefValue( NX_ALREADY_RELEASED     ),
		DefValue( NX_RESERVED_CODE2       ),
		DefValue( NX_MAX_LISTEN           ),
		DefValue( NX_DUPLICATE_LISTEN     ),
		DefValue( NX_NOT_CLOSED           ),
		DefValue( NX_NOT_LISTEN_STATE     ),
		DefValue( NX_IN_PROGRESS          ),
		DefValue( NX_NOT_CONNECTED        ),
		DefValue( NX_WINDOW_OVERFLOW      ),
		DefValue( NX_ALREADY_SUSPENDED    ),
		DefValue( NX_DISCONNECT_FAILED    ),
		DefValue( NX_STILL_BOUND          ),
		DefValue( NX_NOT_SUCCESSFUL       ),
		DefValue( NX_UNHANDLED_COMMAND    ),
		DefValue( NX_NO_FREE_PORTS        ),
		DefValue( NX_INVALID_PORT         ),
		DefValue( NX_INVALID_RELISTEN     ),
		DefValue( NX_CONNECTION_PENDING   ),
		DefValue( NX_TX_QUEUE_DEPTH       ),
		DefValue( NX_NOT_IMPLEMENTED      ),
		DefValue( NX_NOT_SUPPORTED        ),
		DefValue( NX_INVALID_INTERFACE    ),
		DefValue( NX_INVALID_PARAMETERS   ),
		DefValue( NX_NOT_FOUND            ),
		DefValue( NX_CANNOT_START         ),
		DefValue( NX_NO_INTERFACE_ADDRESS ),
		DefValue( NX_INVALID_MTU_DATA     ),
		DefValue( NX_DUPLICATED_ENTRY     ),
		DefValue( NX_PACKET_OFFSET_ERROR  ),
		DefValue( NX_OPTION_HEADER_ERROR  ),
		DefValue( NX_CONTINUE             ),
		DefValue( NX_TCPIP_OFFLOAD_ERROR  ),
};

static const UINT NxStatusCount = sizeof(NxStatus)/sizeof(CODENAME);

const CHAR* const Get_NxStatusName(UINT nx_status){
	return Get_CodeName(nx_status, NxStatus, NxStatusCount);
}


//#include "tx_api.h"
const CODENAME TxStatus[] = {
		DefValue( TX_SUCCESS             ),
		DefValue( TX_DELETED             ),
		DefValue( TX_POOL_ERROR          ),
		DefValue( TX_PTR_ERROR           ),
		DefValue( TX_WAIT_ERROR          ),
		DefValue( TX_SIZE_ERROR          ),
		DefValue( TX_GROUP_ERROR         ),
		DefValue( TX_NO_EVENTS           ),
		DefValue( TX_OPTION_ERROR        ),
		DefValue( TX_QUEUE_ERROR         ),
		DefValue( TX_QUEUE_EMPTY         ),
		DefValue( TX_QUEUE_FULL          ),
		DefValue( TX_SEMAPHORE_ERROR     ),
		DefValue( TX_NO_INSTANCE         ),
		DefValue( TX_THREAD_ERROR        ),
		DefValue( TX_PRIORITY_ERROR      ),
		DefValue( TX_NO_MEMORY           ),
		DefValue( TX_START_ERROR         ),
		DefValue( TX_DELETE_ERROR        ),
		DefValue( TX_RESUME_ERROR        ),
		DefValue( TX_CALLER_ERROR        ),
		DefValue( TX_SUSPEND_ERROR       ),
		DefValue( TX_TIMER_ERROR         ),
		DefValue( TX_TICK_ERROR          ),
		DefValue( TX_ACTIVATE_ERROR      ),
		DefValue( TX_THRESH_ERROR        ),
		DefValue( TX_SUSPEND_LIFTED      ),
		DefValue( TX_WAIT_ABORTED        ),
		DefValue( TX_WAIT_ABORT_ERROR    ),
		DefValue( TX_MUTEX_ERROR         ),
		DefValue( TX_NOT_AVAILABLE       ),
		DefValue( TX_NOT_OWNED           ),
		DefValue( TX_INHERIT_ERROR       ),
		DefValue( TX_NOT_DONE            ),
		DefValue( TX_CEILING_EXCEEDED    ),
		DefValue( TX_INVALID_CEILING     ),
		DefValue( TX_FEATURE_NOT_ENABLED ),
};

static const UINT TxStatusCount = sizeof(TxStatus)/sizeof(CODENAME);

const CHAR* const Get_TxStatusName(UINT tx_status){
	return Get_CodeName(tx_status, TxStatus, TxStatusCount);
}
