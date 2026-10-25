#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// Pseudo-rand number generator (adjusted to use uint32)
uint32 lfsr = 0xACE1u;
uint32 rand() {
    uint32 bit;
    bit = ((lfsr >> 0) ^ (lfsr >> 10) ^ (lfsr >> 30) ^ (lfsr >> 31)) & 1;
    return lfsr = (lfsr >> 1) | (bit << 31);
}

// Lottery scheduler
void lottery() {
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;

  // Iterate through all runnable procs to gather total tickets
  uint64 total_tix = 0;
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == RUNNABLE && p->tickets > 0) total_tix += p->tickets;
    release(&p->lock);
  }

  if (total_tix == 0) {
    // No processes with tickets exist - stop running core
    intr_on();
    asm volatile("wfi");
    return;
  }

  uint64 winning_ticket = rand() % total_tix;
  uint64 parsed_tix = 0;

  // Iterate again to determine which proc won
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
      if (p->state == RUNNABLE && p->tickets > 0) {
        parsed_tix += p->tickets;

        // When the # of parsed tickets overtakes the winner, that proc is selected
        if (winning_ticket < parsed_tix) {
          p->state = RUNNING;
          p->ticks += 1;
          c->proc = p;
          swtch(&c->context, &p->context);

          c->proc = 0;
          release(&p->lock);
          break;
        }
      }
    release(&p->lock);
  }
}

// Stride scheduler
void stride() {
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;

  // Iterate through all runnable procs to gather total processes, set pass values if unset
  uint64 total_procs = 0;
  struct proc *winner = proc;
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == RUNNABLE && p->tickets > 0) { 
      total_procs += 1;
      if (winner->pass > p->pass) winner = p;
    }
    release(&p->lock);
  }
   
  if (total_procs == 0) {
    // No processes with tickets exist - stop running core
    intr_on();
    asm volatile("wfi");
    return;
  }

  // Switch context to winner (last element in the case of a tie)
  acquire(&winner->lock);
  winner->state = RUNNING;
  winner->ticks += 1;
  winner->pass += winner->stride;
  c->proc = winner;
  swtch(&c->context, &winner->context);

  c->proc = 0;
  release(&winner->lock);
}
