#ifndef __ltkrn_h__
#define __ltkrn_h__

#include "stm8s.h"

#define KRN_RAMTOP  0x400
#define KRN_STACK_IDLE  0x30
#define KRN_STACKFRAME  (KRN_RAMTOP - KRN_STACK_IDLE)

#define KRN_FREQ  100
#define KRN_THR_RND	0x01
#define KRN_THR_SUSP	0x02
#define KRN_THR_RST	0x04
#define KRN_THR_LOCK	0x08
#define KRN_THR_IDLE	0x10
#define KRN_FLAG_RND	0x01
#define KRN_FLAG_RAN	0x02
#define KRN_FLAG_RST	0x04
#define KRN_FLAG_IDLE	0x10
#define KRN_MUTEX_LOCK	0x01

typedef struct _krn_mutex
{
  uint8_t flag;
  struct _krn_thread *thread;
}
krn_mutex;

typedef struct _krn_thread
{
	uint8_t *sp;
	struct _krn_thread *prev, *next, *t_next;
        krn_mutex *mutex;
	uint8_t tslice;
	uint8_t tslice_c;
	int16_t timer;
	uint8_t flags;
	void *func;
	void* param;
}
krn_thread;

extern krn_thread *krn_thread_first;
extern krn_thread *krn_thread_current;
extern krn_thread *krn_thr_nearest;
extern int16_t krn_timer_nearest;
extern int16_t krn_timer_current;
extern uint16_t krn_timer_cnt;

extern inline void krn_thread_init();
extern inline void krn_thread_insert(krn_thread *thr, krn_thread *after);
extern inline void krn_thread_del(krn_thread *thr);
extern inline void krn_thread_move(krn_thread *thr, krn_thread *after);
extern inline void krn_thread_create(krn_thread *thr, void *func, void* param, uint8_t tslice, void *stack, uint8_t stack_size);
extern inline uint8_t krn_dispatch_h();
extern void krn_context_switch(krn_thread  *from, krn_thread *to);
extern void krn_context_load(krn_thread *load);
extern void krn_uthread_idle(void);
extern void krn_enter_thread(void *func);
extern inline void krn_dispatch();
extern void krn_timer_init();
extern void krn_run();
extern void krn_sleep(int16_t ticks);
extern void krn_mutex_init(krn_mutex *mutex);
extern void krn_mutex_lock(krn_mutex *mutex);
extern void krn_mutex_unlock(krn_mutex *mutex);

#define krn_thread_stop(thr) thr->flags|=KRN_THR_SUSP
#define krn_thread_cont(thr) thr->flags&=~KRN_THR_SUSP
#define krn_thread_lock(thr) thr->flags|=KRN_THR_LOCK
#define krn_thread_unlock(thr) thr->flags&=~KRN_THR_LOCK

#ifdef __IAR_SYSTEMS_ICC__
#define NO_REG_SAVE __task
#else
#define NO_REG_SAVE
#endif

#define krn_tmp_stack()      asm ("ldw x, #0x2ff\nldw SP, x\n")

/* Critical region protection */
/* COSMIC: Use inline assembler */
#if defined(__CSMC__)
#define CRITICAL_STORE      uint8_t ccr
#define CRITICAL_START()    _asm ("push CC\npop a\nld (X),A\nsim", &ccr)
#define CRITICAL_END()      _asm ("ld A,(X)\npush A\npop CC", &ccr)
/* IAR: Use intrinsics */
#elif defined(__IAR_SYSTEMS_ICC__)
#define CRITICAL_STORE      __istate_t _istate
#define CRITICAL_START()    _istate = __get_interrupt_state(); __disable_interrupt()
#define CRITICAL_END()      __set_interrupt_state(_istate)
/* Raisonance: Use intrinsics */
#elif defined(__RCSTM8__)
#define CRITICAL_STORE      unsigned char ccr
#define CRITICAL_START()    ccr = _getCC_(); _sim_()
#define CRITICAL_END()      _setCC_(ccr)
#endif

#endif

