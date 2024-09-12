/**
  ******************************************************************************
  * @file           : logmsg.c
  * @brief          : Message logging functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 ART Finex Co.,Ltd.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "app_netxduo.h"
//#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include "logmsg.h"
//#include "nx_ip.h"
//#include "nx_system.h"

const char* const LevelMsg[] = {"DEBUG", "INFO", "MESSAGE", "RESERVED", "NOERROR", "WARNING", "ERROR", "FATAL" };

static TX_MUTEX    lock;

void LogMsg_Init(){
	tx_mutex_create(&lock, "LOGMSG Mutex", 1);
    LOGMSG_ERROR("Hellow %s", "Abc");
	LOGMSG_ERROR(" World");
}
/*
void LogMsg(int level, const char* fmt){
	if (level < LOGMSG_LEVEL)
		return;
    tx_mutex_get(&lock, NX_WAIT_FOREVER); //lockして同時アクセスを防ぐ
    vprintf( fmt );
    printf("\r\n");
    tx_mutex_put(&lock);
}
*/
void LogMsg(int level, const char* fmt, ...){
	if (level < LOGMSG_LEVEL)
		return;
    va_list ap;
    va_start( ap, fmt );
    tx_mutex_get(&lock, NX_WAIT_FOREVER); //lockして同時アクセスを防ぐ
    vprintf( fmt, ap );
    printf("\r\n");
    tx_mutex_put(&lock);
    va_end( ap );
}

void DbgMsg(int level, const char* file, int line, const char* func, const char* fmt, ...){
	if (level < LOGMSG_LEVEL)
		return;
    va_list ap;
    va_start( ap, fmt );
    tx_mutex_get(&lock, NX_WAIT_FOREVER); //lockして同時アクセスを防ぐ
	printf("%s:%d [%s] %s() ", file, line, LevelMsg[level], func);
    vprintf( fmt, ap );
    printf("\r\n");
    tx_mutex_put(&lock);
    va_end( ap );
}

void AstMsg(int status, int level, const char* file, int line, const char* func, const char* fmt, ...){
	if (level < ERRMSG_LEVEL)
		return;
    va_list ap;
    va_start( ap, fmt );
    tx_mutex_get(&lock, NX_WAIT_FOREVER); //lockして同時アクセスを防ぐ
    printf("%s:%d [%s] %s(), status:%d  ", file, line, LevelMsg[level], func, status);
    vprintf( fmt, ap );
    printf("\r\n");
    tx_mutex_put(&lock);
    va_end( ap );
}

int Assert(int status, int level, const char* file, int line, const char* func, const char* fmt, ...){
	if (status ==0)
		level = MSG_LEVEL_NOERROR;
    va_list ap;
    va_start( ap, fmt );
	AstMsg(status, level, file, line, func, fmt, ap);
    va_end( ap );
    if (level >= MSG_LEVEL_FATALERROR){
    	ERROR_HANDLER();
    }
	return status;
}
