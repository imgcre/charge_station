#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Generated by Kconfiglib (https://github.com/ulfalizer/Kconfiglib) */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 150
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 1500
#define RT_DEBUG
#define RT_DEBUG_COLOR

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE
/* end of Inter-Thread communication */

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_HEAP
/* end of Memory Management */

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "uart1"
/* end of Kernel Device Object */
#define RT_VER_NUM 0x40002
/* end of RT-Thread Kernel */
#define ARCH_ARM
#define RT_USING_CPU_FFS
#define ARCH_ARM_CORTEX_M
#define ARCH_ARM_CORTEX_M3

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 1700
#define RT_MAIN_THREAD_PRIORITY 10

/* C++ features */

#define RT_USING_CPLUSPLUS
/* end of C++ features */

/* Command shell */

/* end of Command shell */

/* Device virtual file system */

/* end of Device virtual file system */

/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_PIPE_BUFSZ 512
#define RT_USING_SERIAL
#define RT_SERIAL_USING_DMA
#define RT_SERIAL_RB_BUFSZ 256
#define RT_USING_HWTIMER
#define RT_USING_I2C
#define RT_USING_I2C_BITOPS
#define RT_USING_PIN
#define RT_USING_WDT

/* Using USB */

/* end of Using USB */
/* end of Device Drivers */

/* POSIX layer and C standard library */

#define RT_USING_LIBC
/* end of POSIX layer and C standard library */

/* Network */

/* Socket abstraction layer */

/* end of Socket abstraction layer */

/* Network interface device */

/* end of Network interface device */

/* light weight TCP/IP stack */

/* end of light weight TCP/IP stack */

/* AT commands */

#define RT_USING_AT
#define AT_DEBUG
#define AT_USING_CLIENT
#define AT_CLIENT_NUM_MAX 1
#define AT_PRINT_RAW_CMD
#define AT_CMD_MAX_LEN 512
#define AT_SW_VERSION_NUM 0x10300
/* end of AT commands */
/* end of Network */

/* VBUS(Virtual Software BUS) */

/* end of VBUS(Virtual Software BUS) */

/* Utilities */

#define RT_USING_ULOG
#define ULOG_OUTPUT_LVL_D
#define ULOG_OUTPUT_LVL 7
#define ULOG_USING_ISR_LOG
#define ULOG_ASSERT_ENABLE
#define ULOG_LINE_BUF_SIZE 512

/* log format */

#define ULOG_OUTPUT_FLOAT
#define ULOG_USING_COLOR
#define ULOG_OUTPUT_TAG
/* end of log format */
#define ULOG_BACKEND_USING_CONSOLE
#define ULOG_USING_FILTER
/* end of Utilities */
/* end of RT-Thread Components */

/* RT-Thread online packages */

/* IoT - internet of things */

#define PKG_USING_CJSON
#define PKG_USING_CJSON_V102

/* Wi-Fi */

/* Marvell WiFi */

/* end of Marvell WiFi */

/* Wiced WiFi */

/* end of Wiced WiFi */
/* end of Wi-Fi */

/* IoT Cloud */

/* end of IoT Cloud */
/* end of IoT - internet of things */

/* security packages */

#define PKG_USING_TINYCRYPT
#define PKG_USING_TINYCRYPT_V100
#define TINY_CRYPT_MD5
/* end of security packages */

/* language packages */

/* end of language packages */

/* multimedia packages */

/* end of multimedia packages */

/* tools packages */

/* end of tools packages */

/* system packages */

/* end of system packages */

/* peripheral libraries and drivers */

#define PKG_USING_AT24CXX
#define PKG_USING_AT24CXX_LATEST_VERSION
/* end of peripheral libraries and drivers */

/* miscellaneous packages */


/* samples: kernel and components samples */

/* end of samples: kernel and components samples */
/* end of miscellaneous packages */
/* end of RT-Thread online packages */

/* samples: kernel and components samples */

/* end of samples: kernel and components samples */
#define RT_STUDIO_BUILT_IN

#endif
