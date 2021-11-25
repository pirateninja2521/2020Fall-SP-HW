#include "threadutils.h"

/*
0. You should state the signal you received by:
   printf('TSTP signal!\n') or printf('ALRM signal!\n')
1. If you receive SIGALRM, you should reset alarm() by timeslice argument passed in ./main
2. You should longjmp(SCHEDULER,1) once you're done
*/
void sighandler(int signo)
{
   if (signo == SIGTSTP)
      printf("TSTP signal!\n");
   else if (signo == SIGALRM){
      printf("ALRM signal!\n");
      alarm(timeslice);
   }
   longjmp(SCHEDULER, 1);
}

/*
0. You are stronly adviced to make setjmp(SCHEDULER) = 1 for ThreadYield() case
                                   setjmp(SCHEDULER) = 2 for ThreadExit() case
1. Please point the Current TCB_ptr to correct TCB_NODE
2. Please maintain the circular linked-list here
*/
void scheduler()
{
   int a;
   a = setjmp(SCHEDULER);
   if (a == 1 || a == 0){ // ThreadYield() case
      sigprocmask(SIG_SETMASK, &base_mask, NULL);
      Current = Current->Next;
      longjmp(Current->Environment, 1);
   }
   else if (a == 2){ // ThreadExit() case
      if(Current->Next == Current){
         free(Current); 
         longjmp(MAIN, 1);
      }
      else{
         Current->Prev->Next = Current->Next;
         Current->Next->Prev = Current->Prev;
         Work = Current;
         Current = Current->Next;
         free(Work);
         longjmp(Current->Environment, 1);
      }
   }

   return;
}