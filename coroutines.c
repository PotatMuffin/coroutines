#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;
    void *stack_end;
} Context;

typedef struct {
    Context items[256];
    size_t count;
    size_t current;
} Contexts;

#define STACK_SIZE 2*1024

void *public_stack;
Contexts contexts = {0};

void switch_context();
void coroutine_finish();

void print_context(Context *c)
{
    printf("rsp = 0x%lx\n", c->rsp);
    printf("rbp = 0x%lx\n", c->rbp);
    printf("rip = 0x%lx\n", c->rip);
}

size_t coroutine_count()
{
    return contexts.count;
}

size_t coroutine_index()
{
    return contexts.current;
}

void init_coroutine()
{
    public_stack = malloc(STACK_SIZE);
    contexts.items[contexts.count++] = (Context){0};
}

void coroutine_create(void (*f)(void), void *arg)
{
    Context *c = &contexts.items[contexts.count++];
    void *stack = malloc(STACK_SIZE);
    void **stack_start = (void **)((uint64_t)stack+STACK_SIZE);
    stack_start--;
    *stack_start = (void*)&coroutine_finish;
    stack_start--;
    *stack_start = arg;

    c->stack_end = stack;
    c->rsp = (uint64_t)stack_start;
    c->rbp = 0;
    c->rip = (uint64_t)f;
}

__attribute__((naked)) void coroutine_yield()
{
    __asm__(
        "pop r10\n\t"
        "mov %0, r10\n\t"
        "mov %1, rsp\n\t"
        "mov %2, rbp\n\t"
        : "=m"(contexts.items[contexts.current].rip), "=m"(contexts.items[contexts.current].rsp), "=m"(contexts.items[contexts.current].rbp)
    );

    contexts.current++;
    contexts.current %= contexts.count;

    register void *rax __asm__("rax");
    rax = (void *)&switch_context;
    __asm__(
        "jmp rax\n\t"
    );
}

__attribute__((naked)) void coroutine_finish()
{
    register void **rsp __asm__("rsp");
    rsp = public_stack;
    free(contexts.items[contexts.current].stack_end);
    contexts.items[contexts.current] = contexts.items[contexts.count-1];
    contexts.count--;

    contexts.current %= contexts.count;

    register void *rax __asm__("rax");
    rax = (void *)&switch_context;
    __asm__(
        "jmp rax\n\t"
    );
}

__attribute__((naked)) void switch_context()
{
    register Context *context __asm__("rdx");
    context = &contexts.items[contexts.current];

    __asm__(
        "mov rsp, %0\n\t"
        "mov rbp, %1\n\t"
        : "=m"(context->rsp), "=m"(context->rbp)
    );

    if (!context->rbp)
    {
        register void **rsp __asm__("rsp"); 
        register void *rdi __asm__("rdi");
        rdi = *(rsp++);
    }

    __asm__(
        "mov rax, %0\n\t"
        "jmp rax\n\t"
        : "=m"(context->rip)
    ); 
}