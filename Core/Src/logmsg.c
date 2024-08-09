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
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include "logmsg.h"

const char* const LevelMsg[] = {"DEBUG", "INFO", "MESSAGE", "RESERVED", "NOERROR", "WARNING", "ERROR", "FATAL" };

static pthread_mutex_t lock;

void LogMsg_Init(){
    pthread_mutex_init(&lock, NULL);
    LOGMSG_ERROR("Hellow %s", "Abc");
	LOGMSG_ERROR(" World")
    printf
}


static void _LogMsg(const char* fmt, ...){
    va_list ap;
    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );
}

void LogMsg(int level, const char* fmt, ...){
	if (level < LOGMSG_LEVEL)
		return;
    va_list ap;
    va_start( ap, fmt );
    pthread_mutex_lock(&lock); //lockして同時アクセスを防ぐ
    _LogMsg( fmt, ap );
    _LogMsg( "\r\n" );
    pthread_mutex_unlock(&lock);
    va_end( ap );
}

void DbgMsg(int level, const char* file, int line, const char* func, const char* fmt, ...){
	if (level < LOGMSG_LEVEL)
		return;
	_LogMsg("%s:%d [%s] %s() ", file, line, LevelMsg[level], func);
    va_list ap;
    va_start( ap, fmt );
    pthread_mutex_lock(&lock); //lockして同時アクセスを防ぐ
    _LogMsg( fmt, ap );
    _LogMsg( "\r\n" );
    pthread_mutex_unlock(&lock);
    va_end( ap );
}

void AstMsg(int status, int level, const char* file, int line, const char* func, const char* fmt, ...){
	if (level < ERRMSG_LEVEL)
		return;
    va_list ap;
    va_start( ap, fmt );
    pthread_mutex_lock(&lock); //lockして同時アクセスを防ぐ
	_LogMsg("%s:%d [%s] %s(), status:%d  ", file, line, LevelMsg[level], func, status);
    _LogMsg( fmt, ap );
    _LogMsg( "\r\n" );
    pthread_mutex_unlock(&lock);
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
