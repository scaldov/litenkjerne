#include "ltkrn.h"
#include "hd44780.h"
#include "stm8s_tim4.h"

uint8_t krn_timer_warp;
uint16_t krn_timer_cnt;
uint8_t krn_dispatch_flag;
krn_thread thr_uthread_idle;
krn_thread *krn_thread_current;
krn_thread *krn_thread_first;
krn_thread *krn_thread_nearest;
int16_t krn_timer_nearest;
int16_t krn_timer_current;

inline void krn_thread_init()
{
	krn_dispatch_flag = 0;
	krn_thread_first = 0;
        krn_timer_nearest = 0;
        krn_timer_current = 0;
}

inline void krn_thread_insert(krn_thread *thr, krn_thread *after)
{
	krn_thread *prev, *next;
	if( krn_thread_first == 0)
	{
		after = krn_thread_first = thr;
		krn_thread_first->prev = krn_thread_first->next = thr;
	}
	prev = after;
	next = after->next;
	prev->next = thr;
	next->prev = thr;
	thr->prev = prev;
	thr->next = next;
}

inline void krn_thread_del(krn_thread *thr)
{
	krn_thread *prev, *next;
	prev = thr->prev;
	next = thr->next;
	prev->next = next;
	next->prev = prev;
}

inline void krn_thread_move(krn_thread *thr, krn_thread *after)
{
	krn_thread *prev, *next;
	prev = thr->prev;
	next = thr->next;
	prev->next = next;
	next->prev = prev;
        next = after->next;
        after->next = thr;
        next->prev = thr;
        thr->prev = after;
        thr->next = next;
	//thr->tslice_c = thr->tslice;
	thr->flags &= ~(KRN_THR_RND );
	thr->flags |= (next->flags & (KRN_THR_RND ));
}

static NO_REG_SAVE void krn_thread_shell (void)
{
  //enable interrupts before thread start
#if defined(__CSMC__)
    _asm("rim");
#elif defined(__IAR_SYSTEMS_ICC__)
    rim();
#elif defined(__RCSTM8__)
    _rim_();
#endif

    //thread entry
    if (krn_thread_first && krn_thread_first->func)
    {
      krn_enter_thread(krn_thread_first->func);
    }
}

inline void krn_thread_create(krn_thread *thr, void *func, void* param, uint8_t tslice, void *stack, uint8_t stack_size)
{
  krn_thread_insert(thr, krn_thread_first);
  thr->func = func;
  thr->param = param;
  thr->tslice = thr->tslice_c = tslice;
  thr->t_next = 0;
  thr->timer = 0;
  krn_thread_first->flags = (krn_dispatch_flag ^ KRN_FLAG_RND) & KRN_THR_RND;
  krn_thread_first->flags = (krn_dispatch_flag & ~KRN_FLAG_RST) | KRN_THR_RST;
  uint8_t *stack_ptr;
  stack_ptr = (uint8_t *)stack + stack_size - 1;
  *stack_ptr-- = (uint8_t)((uint16_t)krn_thread_shell & 0xFF);
  *stack_ptr-- = (uint8_t)(((uint16_t)krn_thread_shell >> 8) & 0xFF);
#if defined(__IAR_SYSTEMS_ICC__)
  stack_ptr -= 8; //8 IAR virtual registers
#endif
  thr->sp = stack_ptr;
}

inline uint8_t krn_dispatch_h()
{
        krn_thread *old;
	krn_thread_first = krn_thread_first->next;
	if(((krn_thread_first->flags ^ krn_dispatch_flag) & KRN_THR_RST))
	{
		krn_thread_first->flags ^= KRN_THR_RST;
                if(krn_thread_first->tslice_c == 0) krn_thread_first->tslice_c = krn_thread_first->tslice;
	}
	if(((krn_thread_first->flags ^ krn_dispatch_flag) & KRN_THR_RND) == 0)
	{
		krn_dispatch_flag ^= KRN_FLAG_RND;
		if((krn_dispatch_flag & KRN_FLAG_RAN) == 0)
		{
			krn_dispatch_flag ^= KRN_FLAG_RST;
                        krn_dispatch_flag |= KRN_FLAG_IDLE;
		}
		krn_dispatch_flag &= ~KRN_FLAG_RAN;
		krn_thread_first = krn_thread_first->prev;
		return 0;
	}
	krn_thread_first->flags ^= KRN_THR_RND;
        if(krn_thread_first->flags & (KRN_THR_SUSP | KRN_THR_LOCK)) return 0;
	if(krn_thread_first->tslice_c != 0)
	{
                old = krn_thread_current;
		krn_dispatch_flag |= KRN_FLAG_RAN;
		krn_thread_first->tslice_c--;
                if(krn_thread_first == &thr_uthread_idle)
                {
                  if((krn_dispatch_flag & KRN_FLAG_IDLE) == 0) return 0;
                } else krn_dispatch_flag &= ~KRN_FLAG_IDLE;
                krn_thread_current = krn_thread_first;
                krn_context_switch(old, krn_thread_current);
		return 1;
	}
	else
	{
                if(((krn_thread_first->flags ^ krn_dispatch_flag) & KRN_THR_RST)) krn_thread_first->flags ^= KRN_THR_RST;
                return 0;
	}
}

inline void krn_dispatch()
{
  krn_thread *old;
  CRITICAL_STORE;
  CRITICAL_START();
  if(krn_thread_nearest != 0)
  {
    if(krn_timer_nearest <= krn_timer_current)
    {
      krn_thread_move(krn_thread_nearest, krn_thread_current); //for hard realtime
      krn_thread_cont(krn_thread_nearest);
      old = krn_thread_nearest;
      krn_thread_nearest = krn_thread_nearest->t_next;
      krn_timer_nearest = krn_thread_nearest->timer;
      old->t_next = 0;
    }
  }
  while (!krn_dispatch_h());
  CRITICAL_END();
}

#if defined(__IAR_SYSTEMS_ICC__)
//#pragma vector = 13
#pragma vector = 25
#endif
INTERRUPT void krn_timer_isr (void)
#if defined(__RCSTM8__)
//interrupt 11
interrupt 23
#endif
{
  krn_timer_warp --;
  if (krn_timer_warp == 0)
  {
    krn_timer_cnt ++;
    krn_timer_current ++;
    krn_timer_warp = 5;
    krn_dispatch();
  }
#ifdef HD44780_TX
  if(hd44780_flags & HD44780_TX) hd44780_tx_proc();
#endif
  TIM4->SR1 = (uint8_t)(~(uint8_t)TIM4_IT_UPDATE);
}

void krn_timer_init()
{
  uint8_t presc;
  switch ((uint8_t)(CLK->CKDIVR & CLK_CKDIVR_HSIDIV))
  {
  case CLK_PRESCALER_HSIDIV1:
    presc = TIM4_PRESCALER_128; break;
  case CLK_PRESCALER_HSIDIV2:
    presc = TIM4_PRESCALER_64; break;
  case CLK_PRESCALER_HSIDIV4:
    presc = TIM4_PRESCALER_32; break;
  case CLK_PRESCALER_HSIDIV8:
  default:
    presc = TIM4_PRESCALER_16; break;
  }
  TIM4->PSCR = presc;
  TIM4->ARR = 16000000 / 128 / KRN_FREQ / 5;
  TIM4->IER = (uint8_t)(TIM4_IER_UIE);
  TIM4->CR1 = (uint8_t)(TIM4_CR1_CEN | TIM4_CR1_ARPE);
}

void krn_run()
{
  krn_thread_create(&thr_uthread_idle, (void*)krn_uthread_idle, (void*)0, 1, (void*)KRN_STACKFRAME, KRN_STACK_IDLE);
  thr_uthread_idle.flags |= KRN_THR_IDLE;
  krn_thread_current = krn_thread_first;
  krn_context_load(krn_thread_first);
}

//thread is put in right place of timer stack
void krn_sleep(int16_t ticks)
{
  krn_thread *old, *post;
  CRITICAL_STORE;
  CRITICAL_START();
  post = 0;
  if(krn_thread_nearest)
  {
    old = krn_thread_nearest;
    do
    {
      old->timer -= krn_timer_current;
      if(post == 0)
      {
        if(ticks >= old->timer)
        {
          if(old->t_next)
          {
            if((old->t_next->timer - krn_timer_current) > ticks) post = old;
          } else post = old;
        }
      }
      old = old->t_next;
    }while(old);
  }
  krn_timer_current = 0;
  if(post != 0)
  {
    krn_thread_current->t_next = post->t_next;
    post->t_next = krn_thread_current;
    krn_thread_current->timer = ticks;
    krn_timer_nearest = krn_thread_nearest->timer;
  }
  else
  {
    krn_thread_current->t_next = krn_thread_nearest;
    krn_thread_current->timer = ticks;
    krn_thread_nearest = krn_thread_current;
    krn_timer_nearest = ticks;
  }
  krn_thread_stop(krn_thread_current);
  CRITICAL_END();
  krn_dispatch();
}

void krn_mutex_init(krn_mutex *mutex)
{
  mutex->flag = 0;
}

void krn_mutex_lock(krn_mutex *mutex)
{
  CRITICAL_STORE;
  CRITICAL_START();
  mutex->flag ++;
  if(mutex->flag > 1)
  {
    krn_thread_lock(krn_thread_current);
    krn_thread_current->mutex = mutex;
    krn_dispatch();
  }
  else mutex->thread = krn_thread_current;
  CRITICAL_END();
}

void krn_mutex_unlock(krn_mutex *mutex)
{
  krn_thread *first, *cur;
  CRITICAL_STORE;
  char flag = 0;
  CRITICAL_START();
  mutex->flag --;
  cur = first = krn_thread_first;
  do
  {
    if(cur->mutex == mutex)
    {
      cur->mutex = 0;
      krn_thread_unlock(cur);
      if(krn_thread_first != cur | 1) krn_thread_move(cur, krn_thread_current); //for hard realtime
      flag = 1;
      krn_dispatch();
      break;
    }
    cur = cur->next;
  }while(cur != first);
  //if(flag) krn_dispatch();
  CRITICAL_END();
}

