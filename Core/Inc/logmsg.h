/*
 * logmsg.h
 *
 *  Created on: Aug 9, 2024
 *      Author: a-yam
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LOGMSG_H
#define __LOGMSG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#define MSG_LEVEL_DEBUG			0	// デバッグメッセージ
#define MSG_LEVEL_INFOMATION		1	// 情報レベルメッセージ 主にデバッグ用
#define MSG_LEVEL_MESSAGE		2	// 能動的なメッセージ
#define MSG_LEVEL_RESERVED		3	// 未使用
#define MSG_LEVEL_NOERROR		4	// 正常終了
#define MSG_LEVEL_WARNING		5	// 警告メッセージ 	軽微な問題
#define MSG_LEVEL_ERROR			6	// エラーメッセージ エラーだが継続可能
#define MSG_LEVEL_FATALERROR		7	// エラーメッセージ 継続不可能なエラー
#define MSG_LEVEL_SILENCE		8	// 全てのエラーメッセージを出力しない

#define LOGMSG_LEVEL				MSG_LEVEL_DEBUG	// コードを有効にするメッセージレベル
#define DBGMSG_LEVEL				MSG_LEVEL_DEBUG	// コードを有効にするメッセージレベル
#define ERRMSG_LEVEL				MSG_LEVEL_DEBUG	// エラーメッセージを表示するメッセージレベル

void lm_vprintf(const char* fmt, va_list args);
void lm_printf(const char* fmt, ...);


#if LOGMSG_LEVEL > MSG_LEVEL_DEBUG
	#define LOGMSG_DEBUG(fmt,...)
#else
	#define LOGMSG_DEBUG(fmt,...)			LogMsg(MSG_LEVEL_DEBUG,fmt, ##__VA_ARGS__)
#endif

#if LOGMSG_LEVEL > MSG_LEVEL_INFOMATION
	#define LOGMSG_INFOMATION(fmt,...)
#else
	#define LOGMSG_INFOMATION(fmt,...)		LogMsg(MSG_LEVEL_INFOMATION,fmt, ##__VA_ARGS__)
#endif

#if LOGMSG_LEVEL > MSG_LEVEL_MESSAGE
	#define LOGMSG_MESSAGE(fmt,...)
#else
	#define LOGMSG_MESSAGE(fmt,...)			LogMsg(MSG_LEVEL_MESSAGE,fmt, ##__VA_ARGS__)
#endif

#if LOGMSG_LEVEL > MSG_LEVEL_RESERVED
	#define LOGMSG_RESERVED(fmt,...)
#else
	#define LOGMSG_RESERVED(fmt,...)		LogMsg(MSG_LEVEL_RESERVED,fmt, ##__VA_ARGS__)
#endif

#if LOGMSG_LEVEL > MSG_LEVEL_NOERROR
	#define LOGMSG_NOERROR(fmt,...)
#else
	#define LOGMSG_NOERROR(fmt,...)			LogMsg(MSG_LEVEL_NOERROR,fmt, ##__VA_ARGS__)
#endif

#if LOGMSG_LEVEL > MSG_LEVEL_WARNING
	#define LOGMSG_WARNING(fmt,...)
#else
	#define LOGMSG_WARNING(fmt,...)			LogMsg(MSG_LEVEL_WARNING,fmt, ##__VA_ARGS__)
#endif

#if LOGMSG_LEVEL > MSG_LEVEL_ERROR
	#define LOGMSG_ERROR(fmt,...)
#else
	#define LOGMSG_ERROR(fmt,...)			LogMsg(MSG_LEVEL_ERROR,fmt, ##__VA_ARGS__)

#endif

#if LOGMSG_LEVEL > MSG_LEVEL_FATALERROR
	#define LOGMSG_FATALERROR(fmt,...)
#else
	#define LOGMSG_FATALERROR(fmt,...)		LogMsg(MSG_LEVEL_FATALERROR,fmt, ##__VA_ARGS__)
#endif

//void LogMsg(int level, const char* fmt);
void LogMsg(int level, const char* fmt, ...);
//#define LOGMSG(lv,msg,...) 					LogMsg(lv, msg, ##__VA_ARGS__)




#if DBGMSG_LEVEL > MSG_LEVEL_DEBUG
	#define DBGMSG_DEBUG(...)
#else
	#define DBGMSG_DEBUG(...)			DBGMSG(MSG_LEVEL_DEBUG, ##__VA_ARGS__)
#endif

#if DBGMSG_LEVEL > MSG_LEVEL_INFOMATION
	#define DBGMSG_INFOMATION(...)
#else
	#define DBGMSG_INFOMATION(...)		DBGMSG(MSG_LEVEL_INFOMATION, ##__VA_ARGS__)
#endif

#if DBGMSG_LEVEL > MSG_LEVEL_MESSAGE
	#define DBGMSG_MESSAGE(...)
#else
	#define DBGMSG_MESSAGE(...)			DBGMSG(MSG_LEVEL_MESSAGE, ##__VA_ARGS__)
#endif

#if DBGMSG_LEVEL > MSG_LEVEL_RESERVED
	#define DBGMSG_RESERVED(...)
#else
	#define DBGMSG_RESERVED(...)		DBGMSG(MSG_LEVEL_RESERVED, ##__VA_ARGS__)
#endif

#if DBGMSG_LEVEL > MSG_LEVEL_NOERROR
	#define DBGMSG_NOERROR(...)
#else
	#define DBGMSG_NOERROR(...)			DBGMSG(MSG_LEVEL_NOERROR, ##__VA_ARGS__)
#endif

#if DBGMSG_LEVEL > MSG_LEVEL_WARNING
	#define DBGMSG_WARNING(...)
	#define ASSERT_WARNING(...)
#else
	#define DBGMSG_WARNING(...)			DBGMSG(MSG_LEVEL_WARNING, ##__VA_ARGS__)
//	#define ASSERT_WARNING(status,...)	ASSERT(status, MSG_LEVEL_WARNING, __VA_ARGS__)
	#define ASSERT_WARNING(msg,status)	ASSERT(status, MSG_LEVEL_WARNING, msg)
#endif

#if DBGMSG_LEVEL > MSG_LEVEL_ERROR
	#define DBGMSG_ERROR(...)
#else
	#define DBGMSG_ERROR(...)			DBGMSG(MSG_LEVEL_ERROR, ##__VA_ARGS__)
//	#define ASSERT_ERROR(status,...)	ASSERT(status, MSG_LEVEL_WARNING, __VA_ARGS__)
	#define ASSERT_ERROR(msg,status)	ASSERT(status, MSG_LEVEL_WARNING, msg)
#endif

#if DBGMSG_LEVEL > MSG_LEVEL_FATALERROR
	#define DBGMSG_FATALERROR(...)
	#define ASSERT_FATALERROR(msg,status)		{ if(status!=0) ERROR_HANDLER(); }
	#define ASSERT_FATALERROR(msg,status,...)	{ if(status!=0) ERROR_HANDLER(); }
#else
	#define DBGMSG_FATALERROR(...)			DBGMSG(MSG_LEVEL_FATALERROR, ##__VA_ARGS__)
//	#define ASSERT_FATALERROR(status,...)	ASSERT(status, MSG_LEVEL_WARNING, __VA_ARGS__)
	#define ASSERT_FATALERROR(msg,status)	ASSERT(status, MSG_LEVEL_WARNING, msg)
#endif

//#define DBGMSG(...) 					_DBGMSG(__VA_ARGS__,_DBGMSG4,_DBGMSG3,_DBGMSG2,_DBGMSG1,_DBGMSG0)(__VA_ARGS__)
//#define _DBGMSG(_1,_2,_3,_4,_5,_6,name,...) name
//#define _DBGMSG0(lv,msg) 				DbgMsg(lv, __FILE__, __LINE__, __func__, msg )
//#define _DBGMSG1(lv,msg,...)			DbgMsg(lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
//#define _DBGMSG2(lv,msg,...)			DbgMsg(lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
//#define _DBGMSG3(lv,msg,...)			DbgMsg(lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
//#define _DBGMSG4(lv,msg,...)			DbgMsg(lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
void DbgMsg(int level, const char* file, int line, const char* func, const char* fmt, ...);
#define DBGMSG(status,lv,msg) 			DbgMsg(status, lv, __FILE_NAME__, __LINE__, __func__, msg)



//#define ASSERT(...) 					_ASSERT(__VA_ARGS__,_ASSERT4,_ASSERT3,_ASSERT2,_ASSERT1,_ASSERT0)(__VA_ARGS__)
//#define _ASSERT(_1,_2,_3,_4,_5,_6,name,...) name
//#define _ASSERT0(status,lv,msg) 		Assert(status, lv, __FILE__, __LINE__, __func__, msg )
//#define _ASSERT1(status,lv,msg,...)	Assert(status, lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
//#define _ASSERT2(status,lv,msg,...)	Assert(status, lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
//#define _ASSERT3(status,lv,msg,...)	Assert(status, lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
//#define _ASSERT4(status,lv,msg,...)	Assert(status, lv, __FILE__, __LINE__, __func__, msg, __VA_ARGS__)
void AstMsg(int status, int level, const char* file, int line, const char* func, const char* fmt, ...);
int Assert(int status, int level, const char* file, int line, const char* func, const char* fmt, ...);
#define ASSERT(status,lv,msg) 			Assert(status, lv, __FILE_NAME__, __LINE__, __func__, msg)


void Error_Handler(void);
#define ERROR_HANDLER()					Error_Handler()



#ifdef __cplusplus
}
#endif

#endif /* __LOGMSG_H */
