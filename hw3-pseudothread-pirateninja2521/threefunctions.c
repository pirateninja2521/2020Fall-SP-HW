#include "threadutils.h"

void BinarySearch(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);
    /*
    Some initilization if needed.
    */
    if (setjmp(Current->Environment) == 0){
        return;
    }
    Current->y = 0;
    Current->z = 100;
    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        /*
        Do the computation, then output result.
        Call ThreadExit() if the work is done.
        */  
        int M = (Current->y+Current->z)/2;
        printf("BinarySearch: %d\n", M);
        if(M == Current->x){
            break;
        } 
        else if(M > Current->x){
            Current->z = M-1;
        }
        else{
            Current->y = M+1;
        }
        ThreadYield();
    }
    ThreadExit();
}

int h2l(int num){
    if (num == 0) return 0;
    int a = num/100, b = (num%100)/10, c = num%10;
    int out = 0;
    for (int i=9; i>=0; i--){
        if(a==i) out = 10*out+i;
        if (b==i) out = 10*out+i;
        if (c==i) out = 10*out+i;
    }
    while (out/100==0) out*=10;
    return out;
}
int l2h(int num){
    int a = num/100, b = (num%100)/10, c = num%10;
    int out = 0;
    for (int i=0; i<=9; i++){
        if(a==i) out = 10*out+i;
        if (b==i) out = 10*out+i;
        if (c==i) out = 10*out+i;
    }
    return out;
}
void BlackholeNumber(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);
    /*
    Some initilization if needed.
    */
    if (setjmp(Current->Environment) == 0){
        return;
    }
    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        /*
        Do the computation, then output result.
        Call ThreadExit() if the work is done.
        */
        Current->x = h2l(Current->x)-l2h(Current->x);
        printf("BlackholeNumber: %d\n", Current->x);
        if (Current->x == 495) break;
        ThreadYield();
    }
    ThreadExit();
}

void FibonacciSequence(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);
    /*
    Some initilization if needed.
    */
    if (setjmp(Current->Environment) == 0){
        return;
    }
    Current->x = 0; Current->y = 1;
    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        /*
        Do the computation, then output result.
        */
        Current->z = Current->x + Current->y;
        printf("FibonacciSequence: %d\n", Current->z);
        Current->x = Current->y;
        Current->y = Current->z;
        ThreadYield();
    }
    ThreadExit();
}
