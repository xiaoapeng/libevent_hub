/**
 * @file eh.h
 * @brief 主要的头文件实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-13
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#ifndef _EVENT_HUB_H_
#define _EVENT_HUB_H_

#include <stdint.h>

#include "eh_config.h"
#include "eh_types.h"
#include "eh_list.h"
#include "eh_rbtree.h"
#include "eh_error.h"
#include "eh_co.h"
#include "eh_module.h"


typedef struct eh                           eh_t;
typedef uint64_t                            eh_usec_t;
typedef uint64_t                            eh_msec_t;
typedef uint64_t                            eh_clock_t;
typedef int64_t                             eh_sclock_t;
typedef struct eh_task                      eh_task_t;

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


enum EH_TASK_STATE{
    EH_TASK_STATE_READY,                            /* 就绪状态 */
    EH_TASK_STATE_RUNING,                           /* 运行状态 */
    EH_TASK_STATE_WAIT,                             /* 等待状态 */
    EH_TASK_STATE_FINISH,                           /* 结束状态 */
};

/* 永不到期 */
#define EH_TIME_FOREVER                 (-1)

struct eh_platform_port_param{
    void                                (*global_lock)(uint32_t *state);                            /* 上锁， */
    void                                (*global_unlock)(uint32_t state);                           /* 解锁 */
    eh_clock_t                          clocks_per_sec;
    eh_clock_t                          (*get_clock_monotonic_time)(void);                          /* 系统单调时钟的微秒数 */
    void                                (*idle_or_extern_event_handler);                            /* 用户处理空闲和外部事件 */
    void                                (*idle_break)(void);                                        /* 调用此函数通知"platform"从idle_or_extern_event_handler返回 */
    void*                               (*malloc)(size_t size);
    void                                (*free)(void* ptr);
};



/**
 * @brief   微秒转换为时钟数
 * @param   _msec   毫秒数
 * return   eh_clock_t
 */
#define  eh_msec_to_clock(_msec) ({                                                                 \
        eh_clock_t clock = ((eh_msec_t)(_msec)/1000) * EH_CONFIG_CLOCKS_PER_SEC;                    \
        clock += ((((eh_clock_t)(_msec)%1000)) * EH_CONFIG_CLOCKS_PER_SEC)/1000;                    \
        clock ? clock : !!(_msec);                                                                  \
    })

/**
 * @brief   微秒转换为时钟数
 * @param   _usec   微秒数
 * return   eh_clock_t
 */
#define  eh_usec_to_clock(_usec) ({                                                                 \
        eh_clock_t clock = ((eh_usec_t)((_usec)/1000000) * EH_CONFIG_CLOCKS_PER_SEC);               \
        clock += ((((eh_clock_t)(_usec)%1000000)) * EH_CONFIG_CLOCKS_PER_SEC)/1000000;              \
        clock ? clock : !!(_usec);                                                                  \
    })

/**
 * @brief   时钟数转换为毫秒数
 * @param   _clock  时钟数
 * return   eh_msec_t
 */
#define  eh_clock_to_msec(_clock) ({                                                                \
        eh_msec_t msec = ((eh_msec_t)(((eh_clock_t)(_clock)/EH_CONFIG_CLOCKS_PER_SEC) * 1000));              \
        msec += (((eh_clock_t)(_clock)%EH_CONFIG_CLOCKS_PER_SEC) * 1000)/EH_CONFIG_CLOCKS_PER_SEC;                    \
        msec ? msec : !!(_clock);                                                                   \
    })

/**
 * @brief   时钟数转换为微秒数
 * @param   _clock  时钟数
 * return   eh_usec_t
 */
#define  eh_clock_to_usec(_clock) ({                                                                \
        eh_msec_t msec = ((eh_msec_t)(((eh_clock_t)(_clock)/EH_CONFIG_CLOCKS_PER_SEC) * 1000000));           \
        msec += (((eh_clock_t)(_clock)%EH_CONFIG_CLOCKS_PER_SEC) * 1000000)/EH_CONFIG_CLOCKS_PER_SEC;                 \
        msec ? msec : !!(_clock);                                                                   \
    })

/**
 * @brief 让出当前任务
 * @return int 返回0
 */
extern void __async__ eh_task_yield(void);

/**
 * @brief                   使用静态方式创建一个协程任务
 * @param  name             任务名称
 * @param  stack            任务的静态栈
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
extern eh_task_t* eh_task_static_stack_create(const char *name, void *stack, uint32_t stack_size, void *task_arg, int (*task_function)(void*));

/**
 * @brief                   使用动态方式创建一个协程任务
 * @param  name             任务名称
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
extern eh_task_t* eh_task_create(const char *name, uint32_t stack_size, void *task_arg, int (*task_function)(void*));

/**
 * @brief                   退出任务
 * @param  ret              退出返回值
 */
extern void       eh_task_exit(int ret);

/**
 * @brief                   任务获取自己的任务句柄
 * @return eh_task_t*       返回当前的任务句柄
 */
extern eh_task_t* eh_task_self(void);

/**
 * @brief                   阻塞等待任务结束
 * @param  task             被等待的任务句柄,
 * @param  ret              成功返回0，成功时任务将会被 eh_task_destroy 释放掉
 * @return int 
 */
extern int __async__ eh_task_join(eh_task_t *task, int *ret, eh_sclock_t timeout);

/**
 * @brief                   无条件回收任务，十分暴力，被回收的任务资源应该由回收者释放
 */
extern void eh_task_destroy(eh_task_t *task);

/**
 * @brief  初始化event_hub
 *  1.考虑到配合系统多线程编程时在其他线程产生事件的操作，所以设计了互斥锁接口.
 *  2.给lock预留了state参数，考虑在裸机编程时，中断禁止时，需要记录获得锁前的中断状态，所以需要预留state参数。
 * @param  param                     初始化参数
 * @return int 
 */
extern int eh_global_init(void);

/**
 * @brief 反初始化event_hub
 */
extern void eh_global_exit(void);

/**
 * @brief 世界循环，调用后将开始进行协程的调度
 */
extern int eh_loop_run(void);

/**
 * @brief                   退出任务
 * @param  exit_code        退出返回值
 */
extern void eh_loop_exit(int exit_code);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif // _EVENT_HUB_H_