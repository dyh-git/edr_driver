#ifndef __PG_TYPEDEFS_H__
#define __PG_TYPEDEFS_H__

#include <linux/printk.h>

#define DEBUG_INFO	1

#define printk_err(fmt, args...) 		printk("\033[1;31m""[%s, %d]Error:"fmt"\033[0m", __func__, __LINE__, ##args)	//red
#ifdef DEBUG_INFO
#define printk_dbg(fmt, args...) 		printk("\033[1;32m""[%s, %d]"fmt"\033[0m", __func__, __LINE__, ##args)	        //green
#else
#define printk_dbg(fmt, args...)
#endif
#define printk_log(fmt, args...) 		printk("\033[1;32m""[%s, %d]INFO:"fmt"\033[0m", __func__, __LINE__, ##args)	//green
#define printk_warn(fmt, args...) 	    printk("\033[1;33m""[%s, %d]WARNING:"fmt"\033[0m", __func__, __LINE__, ##args)	//yellow

#define NOP() __NOP()

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct reg {
	uint8_t *buf;
	int len;
}reg_t;

typedef struct buf {
	uint8_t *buf;
	int len;
}buf_t;

#endif
