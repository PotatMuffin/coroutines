#include <stdio.h>
#include "coroutines.h"

void counter(void *arg)
{
    long int max = (long int)arg;

    for(int i = 0; i < max; i++, coroutine_yield())
    {
        printf("[%li] %i\n", coroutine_index(), i);
    }
}

int main()
{
    init_coroutine();

    for(long int i = 0; i < 10; i++)
    {
        coroutine_create(counter, (void *)i+1);
    }
    while(coroutine_count() > 1) coroutine_yield();
    return 0;
}