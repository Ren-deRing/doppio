#pragma once

extern void context_switch(struct thread *prev, struct thread *next);

void sched_enqueue(struct thread *t);
struct thread* sched_dequeue(void);
void schedule(void);