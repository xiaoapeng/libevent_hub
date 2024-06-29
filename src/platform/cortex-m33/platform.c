
#include <stdlib.h>
#include "eh.h"
#include "eh_event.h"
#include "eh_timer.h"
#include "eh_platform.h"


__attribute__((naked))  eh_save_state_t  platform_enter_critical(void){
    __asm__ volatile(
        "	.syntax unified									\n"
        "													\n"
        "	mrs r0, primask									\n"/* 保存中断状态在R0中 */
        "	cpsid   i										\n"/* 失能中断 */
        "	dsb												\n"
        "	isb												\n"
        "	bx lr											\n"/* Return. */
        ::: "r0", "memory"
    );
}

__attribute__((naked)) void  platform_exit_critical(eh_save_state_t state){
    __asm__ volatile(
        "	.syntax unified									\n"
        "													\n"
        "	msr primask, r0									\n"/* 恢复中断状态 */
        "	dsb												\n"
        "	isb												\n"
        "	bx lr											\n"/* Return. */
        ::: "memory"
    );
}

eh_clock_t  platform_get_clock_monotonic_time(void){
    return 0;
}


void* platform_malloc(size_t size){
    void *new_ptr;
    eh_save_state_t state = platform_enter_critical();
    new_ptr = malloc(size);
    platform_exit_critical(state);
    return new_ptr;
}
void  platform_free(void* ptr){
    eh_save_state_t state = platform_enter_critical();
    free(ptr);
    platform_exit_critical(state);
}
void  platform_idle_break(void){

}

void  platform_idle_or_extern_event_handler(void){
    
}


static void generic_idle_or_extern_event_handler(void){
    //(void)eh_clock_to_usec(eh_get_loop_idle_time());
}
static void generic_idle_break( void ){

}


static int  __init generic_platform_init(void){

}

static void __exit generic_platform_deinit(void){
    
}

eh_core_module_export(generic_platform_init, generic_platform_deinit);