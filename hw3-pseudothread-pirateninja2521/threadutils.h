#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

extern int timeslice;
extern int switchmode;

typedef struct TCB_NODE *TCB_ptr;
typedef struct TCB_NODE
{
    jmp_buf  Environment;   // The jmp_buf of each function
    int      Thread_id;     // Not necessary for any tasks, but one may found it useful while debuging
    TCB_ptr  Next;          // TCB_ptr points to next TCB_NODE in the circular linked-list
    TCB_ptr  Prev;          // TCB_ptr points to previous TCB_NODE in the circular linked-list
    int i,N;                // See threefunctions.c for their usage
    int x,y,z;              // Three integers for you to store data you want.

}TCB;

extern jmp_buf MAIN;        // The jmp_buf for main()
extern jmp_buf SCHEDULER;   // The jmp_buf for scheduler()

extern TCB_ptr Head;        // Three TCB_ptr for you to initialize your circular linked-list
extern TCB_ptr Current;     // The Current TCB_ptr will move during "context switch"
extern TCB_ptr Work;



extern sigset_t base_mask, waiting_mask;   // See main for meanings of each sigset_t
extern sigset_t tstp_mask, alrm_mask;


void sighandler(int signo);
void scheduler();

/*
Call function in the argument that is passed in
*/
#define ThreadCreate(function,thread_id,init,maxiter)  \
{                                                      \
    function(thread_id, init, maxiter);                \
}
/*
Build up TCB_NODE for each function, insert it into circular linked-list
*/
#define ThreadInit(thread_id,init,maxiter)            \
{                                                     \
    Work = malloc(sizeof(TCB));                       \
     Work->Thread_id = thread_id;                     \
    if (Head == NULL){                                \
        Head = Current = Work;                        \
        Head->Next = Head->Prev = Head;               \
    }                                                 \
    else{                                             \
        Current->Next = Work;                         \
        Work->Prev = Current;                         \
        Work->Next = Head;                            \
        Head->Prev = Work;                            \
        Current = Work;                               \
    }                                                 \
    Work->N = maxiter;                                \
    Work->x = init;                                   \
}
/*
Call this while a thread is terminated
*/
#define ThreadExit()                                   \
{                                                      \
    longjmp(SCHEDULER, 2);                             \
}
/*
Decided whether to "context switch" based on the switchmode argument passed in main.c
*/
#define ThreadYield()                                       \
{                                                           \
    if (setjmp(Current->Environment) == 0){                 \
        if (switchmode == 0){                               \
            longjmp(SCHEDULER, 1);                          \
        }                                                   \
        else{                                               \
            sigprocmask(SIG_SETMASK, &alrm_mask, NULL);     \
            sigprocmask(SIG_SETMASK, &tstp_mask, NULL);     \
            sigprocmask(SIG_SETMASK, &base_mask, NULL);     \
        }                                                   \
    }                                                       \
}








