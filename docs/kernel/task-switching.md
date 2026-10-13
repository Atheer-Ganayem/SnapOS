# Task Switching

## Creating the idle task.

This task as what we are already executing in.
We dont need to allocate a stack because we already have one.
We call `schedule()` in `kmain()`.
`Schedule` calls `arch_switch_to(current, next)`.
`arch_switch_to` checks `next->kernel_stack` and sets the TSS `rsp0` to it.
After checking, `arch_switch_to` calls `-__arch_switch_to`, we push rflags and the callee saved
registers, swap `RSP`, and now we are in the next task's context, we pop the callee saved registers
and rflags, and return.
Timer interrupt happens, pushes the 5 registers (that inlcude CS) and i push the rest of the rigesters.
There's an escelation in privlages (from ring 3 to 0), so it looks at the IDT entry,
sees that the selector is `0x08` (which is the kernel CS) and uses it.
In the timer IRQ we call `schedule()`, `schedule()` calls `arch_switch_to` which class `-__arch_switch_to`,
where we push the callee saved registers with rflags, switch context (`RSP`), and now we are in the idle
task context, pop the callee saved registers and rflags (which crucially containts if=enbaled),
return to `arch_switch_to`, return to `schedule()`, and return to `kmain()`.
In `kmain()` we loop loop loop..... then we get a timer interrupt,
but this time, there is no privlage level escelation, so the CPU doesn't look at the TSS
nor does it switch `rsp` to `tss.rsp0`.
The ISR pushes the rigesters which contain the kernel CS and we continue as normal.
So everytime (after the first cycle of schedule then deschedule. the first time what happens is that i just explained above.) we schedule the idle task we pop the kernel CS, and everytime we
get interrupted there's no privlage escelation so no need to look at `tss.rsp0`.


## Timer IRQ

