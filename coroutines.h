#ifndef COROUTINES
#define COROUTINES

void coroutine_yield();
void coroutine_create(void (*f)(void*), void *arg);
void init_coroutine();
size_t coroutine_count();
size_t coroutine_index();

#endif // COROUTINES