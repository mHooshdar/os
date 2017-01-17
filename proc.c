#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "stdbool.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

struct proc* queue[NPROC];
struct proc* highQueue[NPROC];
struct proc* midQueue[NPROC];
struct proc* lowQueue[NPROC];
int front = 0;
int rear = -1;
int frontHigh = 0;
int rearHigh = -1;
int frontMid = 0;
int rearMid = -1;
int frontLow = 0;
int rearLow = -1;
int itemCount = 0;
int highItemCount = 0;
int midItemCount = 0;
int lowItemCount = 0;

struct proc* peek(struct proc* q[NPROC]){
  return q[front];
}
bool isEmtpy(struct proc* q[NPROC]){
  if(q == queue)
    return itemCount == 0;
  else if(q == highQueue)
    return highItemCount == 0;
  else if(q == midQueue)
    return midItemCount == 0;
  else if(q == lowQueue)
    return lowItemCount == 0;
  return false;
}
bool isFull(struct proc* q[NPROC]){
  if(q == queue)
    return itemCount == NPROC;
  else if(q == highQueue)
    return highItemCount == NPROC;
  else if(q == midQueue)
    return midItemCount == NPROC;
  else if(q == lowQueue)
    return lowItemCount == NPROC;
  return  false;
}
int size(struct proc* q[NPROC]){
  if(q == queue)
    return itemCount;
  else if(q == highQueue)
    return highItemCount;
  else if(q == midQueue)
    return midItemCount;
  else if(q == lowQueue)
    return lowItemCount;
  return 0;
}
void addToQueue(struct proc* q[NPROC], struct proc* data){
  if(!isFull(q)){
    if(q == queue){
      if(rear == NPROC - 1){
        rear = -1;
      }
      queue[++rear] = data;
      itemCount++;
    }
    else if(q == highQueue){
      if(rearHigh == NPROC - 1){
        rearHigh = -1;
      }
      highQueue[++rearHigh] = data;
      highItemCount++;
    }
    else if(q == midQueue){
      if(rearMid == NPROC - 1){
        rearMid = -1;
      }
      midQueue[++rearMid] = data;
      midItemCount++;
    }
    else if(q == lowQueue){
      if(rearLow == NPROC - 1){
        rearLow = -1;
      }
      lowQueue[++rearLow] = data;
      lowItemCount++;
    }
  }
}
struct proc* removeFromQueue(struct proc* q[NPROC]){
  struct proc* data = 0;
  if(q == queue){
    data = queue[front++];
    if(front == NPROC){
      front = 0;
    }
    itemCount--;
  }
  else if(q == highQueue){
    data = highQueue[frontHigh++];
    if(frontHigh == NPROC){
      frontHigh = 0;
    }
    highItemCount--;
  }
  else if(q == midQueue){
    data = midQueue[frontMid++];
    if(frontMid == NPROC){
      frontMid = 0;
    }
    midItemCount--;
  }
  else if(q == lowQueue){
    data = lowQueue[frontLow++];
    if(frontLow == NPROC){
      frontLow = 0;
    }
    lowItemCount--;
  }
  return data;
}
void removeAllFromQueue(){
  int i = 0;
  for(i = 0; i < NPROC; i++){
    queue[i] = 0;
    highQueue[i] = 0;
    midQueue[i] = 0;
    lowQueue[i] = 0;
  }
  front = 0;
  rear = -1;
  itemCount = 0;

  frontHigh = 0;
  rearHigh = -1;
  highItemCount = 0;

  frontMid = 0;
  rearMid = -1;
  midItemCount = 0;

  frontLow = 0;
  rearLow = -1;
  lowItemCount = 0;
}
void display(){
    int i = 0;
    for(i = front; i < rear; i++){
        cprintf("%d, ", queue[i]->pid);
    }
    if(front <= 0 || rear <= 0 || front > rear){

    }
    else{
        cprintf("\n\n\n");
    }
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32fr
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  p->ctime = ticks;
  p->etime = 0;
  p->rtime = 0;
  p->priority = HIGH_PRIORITY;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  if(SCHEDFLAG == FRR || (SCHEDFLAG == Q3 && p->priority == MID_PRIORITY))
    addToQueue(queue, p);
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  if(SCHEDFLAG == FRR || (SCHEDFLAG == Q3 && np->priority == MID_PRIORITY))
    addToQueue(queue, np);

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  proc->etime = ticks;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();
    if(SCHEDFLAG == GRT){
      acquire(&ptable.lock);
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(ticks - p->ctime == 0){
          p->gtime = 99999;
        }
        else{
          p->gtime = p->rtime / (ticks - p->ctime);
        }
      }
      release(&ptable.lock);
      float min;
      struct proc* minProc = ptable.proc;
      min = 100000;
      acquire(&ptable.lock);
      for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state != RUNNABLE){
          continue;
        }
        if (p->gtime <= min) {
          minProc = p;
          min = p->gtime;
        }
      }
      if(minProc->state == RUNNABLE){
        p = minProc;
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, p->context);
        switchkvm();
        proc = 0;
      }
      release(&ptable.lock);
    }
    else if(SCHEDFLAG == Q3){
      removeAllFromQueue();
      acquire(&ptable.lock);
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state != RUNNABLE)
          continue;
        if(p->priority == HIGH_PRIORITY){
          addToQueue(highQueue, p);
        }
        else if(p->priority == MID_PRIORITY){
          addToQueue(midQueue, p);
        }
        else if(p->priority == LOW_PRIORITY){
          addToQueue(lowQueue, p);
        }
      }
      release(&ptable.lock);

      if(!isEmtpy(highQueue)){
        acquire(&ptable.lock);
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
          if(ticks - p->ctime == 0){
            p->gtime = 99999;
          }
          else{
            p->gtime = p->rtime / (ticks - p->ctime);
          }
        }
        release(&ptable.lock);
        int min;
        struct proc* minProc = ptable.proc;
        min = 100000;
        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
          if(p->state != RUNNABLE || p->priority != HIGH_PRIORITY){
            continue;
          }
          if (p->gtime <= min) {
            minProc = p;
            min = p->gtime;
          }
        }
        if(minProc->state == RUNNABLE){
          p = minProc;
          proc = p;
          switchuvm(p);
          p->state = RUNNING;
          removeFromQueue(highQueue);
          swtch(&cpu->scheduler, p->context);
          switchkvm();
          proc = 0;
        }
        release(&ptable.lock);
      }
      if(!isEmtpy(midQueue)){
        acquire(&ptable.lock);
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
          if (p->state != RUNNABLE || p->priority != MID_PRIORITY)
            continue;
          if(isEmtpy(queue) || p != peek(queue)){
            continue;
          }
          proc = p;
          switchuvm(p);
          p->state = RUNNING;
          removeFromQueue(midQueue);
          removeFromQueue(queue);
          swtch(&cpu->scheduler, p->context);
          switchkvm();
          proc = 0;
        }
        release(&ptable.lock);
      }
      if(!isEmtpy(lowQueue)){
        acquire(&ptable.lock);
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
          if (p->state != RUNNABLE || p->priority != LOW_PRIORITY)
            continue;
          if(ticks % QUANTA == 0){
            proc = p;
            switchuvm(p);
            p->state = RUNNING;
            removeFromQueue(lowQueue);
            swtch(&cpu->scheduler, p->context);
            switchkvm();
            proc = 0;
          }
        }
        release(&ptable.lock);
      }
    }
    else{
      // Loop over process table looking for process to run.
      acquire(&ptable.lock);
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE)
          continue;
        if(SCHEDFLAG == DEFAULT_PLOICY){
          // Switch to chosen process.  It is the process's job
          // to release ptable.lock and then reacquire it
          // before jumping back to us.
          proc = p;
          switchuvm(p);
          p->state = RUNNING;
          swtch(&cpu->scheduler, p->context);
          switchkvm();

          // Process is" done running for now.
          // It should have changed its p->state before coming back.
          proc = 0;
        }
        else if(SCHEDFLAG == RR){
          //if(ticks % QUANTA == 0){
            proc = p;
            switchuvm(p);
            p->state = RUNNING;
            swtch(&cpu->scheduler, p->context);
            switchkvm();
            proc = 0;
          //}
        }
        else if(SCHEDFLAG == FRR){
          if(isEmtpy(queue) || p != peek(queue)){
            continue;
          }
          proc = p;
          switchuvm(p);
          p->state = RUNNING;
          removeFromQueue(queue);
          if(printIsValid){
            display();
          }
          swtch(&cpu->scheduler, p->context);
          switchkvm();
          proc = 0;
        }
      }
      release(&ptable.lock);
    }
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  if(SCHEDFLAG == FRR || (SCHEDFLAG == Q3 && proc->priority == MID_PRIORITY))
    addToQueue(queue, proc);
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      if(SCHEDFLAG == FRR || (SCHEDFLAG == Q3 && p->priority == MID_PRIORITY))
        addToQueue(queue, p);
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING){
        p->state = RUNNABLE;
        if(SCHEDFLAG == FRR || (SCHEDFLAG == Q3 && p->priority == MID_PRIORITY))
          addToQueue(queue, p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  p->etime = ticks;
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
// Return -1 if this process has no children.
int
wait2()
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;

        char* wtime=0;
        char* rtime=0;
        argptr(0,&wtime,sizeof(int));
        argptr(1,&rtime,sizeof(int));

        *rtime = p->rtime;
        *wtime = (ticks - p->ctime)-(p->rtime);

        release(&ptable.lock);
        return pid;
      }
    }
  if(!havekids || proc->killed){
    release(&ptable.lock);
    return -1;
  }
  // Wait for children to exit.  (See wakeup1 call in proc_exit.)
  sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
