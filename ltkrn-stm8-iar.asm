  name liten_kjerne
  section .near_func.text:code

#include "vregs.inc"

  public krn_context_switch
  public krn_context_load
  public krn_uthread_idle
  public krn_enter_thread

krn_uthread_idle:
  wfi
  jp krn_uthread_idle

krn_enter_thread:
  popw y
  pushw x
  ret

krn_context_switch:
;x - current thread context, y - new context
  push ?b8
  push ?b9
  push ?b10
  push ?b11
  push ?b12
  push ?b13
  push ?b14
  push ?b15
  ldw ?b0, y
  ldw y, sp
  ldw (x), y
  ldw x, ?b0
krn_context_load:
  ;x - thread context
  ldw x, (x)
  ldw sp, x
  pop ?b15
  pop ?b14
  pop ?b13
  pop ?b12
  pop ?b11
  pop ?b10
  pop ?b9
  pop ?b8
  ret

  end

