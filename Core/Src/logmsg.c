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
int __io_putchar(int ch);

static size_t __write(int file, unsigned char const *pdata, size_t len){
	(void)file;
	for (size_t idx = 0; idx < len; idx++){
		if (*pdata == '\n')
			__io_putchar('\r');
		__io_putchar(*pdata);
		pdata++;
	}
	__io_putchar('\r');
	__io_putchar('\n');
	return len;
}

// printfの出力のインターセプト関数
__attribute__((weak)) int __write_message(unsigned char const *ptr, int len){
	(void)ptr;
	(void)len;
	return -1;
}

// この関数がprintfの出力で使用される
size_t _write(int file, unsigned char const *ptr, size_t len){
    tx_mutex_get(&lock, NX_WAIT_FOREVER); //lockして同時アクセスを防ぐ
	(void)file;
	int length = __write_message(ptr, len);
	if (length < 0)
		length = __write(file, ptr, len);
    tx_mutex_put(&lock);
	return length;
}

void LogMsg_Init(){
	tx_mutex_create(&lock, "LOGMSG Mutex", 1);
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

//static void logmsg(const char* fmt, __VALIST) _ATTRIBUTE ((__format__ (__printf__, 1, 0)));
void lm_vprintf(const char* fmt, va_list args)
{
    tx_mutex_get(&lock, NX_WAIT_FOREVER); //lockして同時アクセスを防ぐ
	static char buf[300];
    vsnprintf( buf, sizeof(buf), fmt, args );
    _write(0, (unsigned char*)buf, strlen(buf));
    tx_mutex_put(&lock);
}

void lm_printf(const char* fmt, ...)
{
    va_list args;
    va_start( args, fmt );
	lm_vprintf(fmt, args);
    va_end( args );
}

//void LogMsg(int level, const char* fmt, __VALIST ap) _ATTRIBUTE ((__format__ (__printf__, 2, 0)));
void LogMsg(int level, const char* fmt, ...)
{
	if (level < LOGMSG_LEVEL)
		return;
    va_list args;
    va_start( args, fmt );
    lm_vprintf(fmt, args);
    va_end( args );
}

void DbgMsg(int level, const char* file, int line, const char* func, const char* fmt, ...){
	if (level < LOGMSG_LEVEL)
		return;
	lm_printf("%s:%d [%s] %s()\n", file, line, LevelMsg[level], func);
    va_list ap;
    va_start( ap, fmt );
    lm_vprintf( fmt, ap );
    va_end( ap );
}

void AstMsg(int status, int level, const char* file, int line, const char* func, const char* fmt, ...){
	if (level < ERRMSG_LEVEL)
		return;
    lm_printf("%s:%d [%s] %s(), status:%d\n", file, line, LevelMsg[level], func, status);
    va_list ap;
    va_start( ap, fmt );
    lm_vprintf( fmt, ap );
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
