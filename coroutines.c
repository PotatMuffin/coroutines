#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint64_t rip;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
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
    register Context *context __asm__("rax");
    context = &contexts.items[contexts.current];

    __asm__(
        "pop r11\n\t"
        "mov [rax+0x00], r11\n\t"
        "mov [rax+0x08], rsp\n\t"
        "mov [rax+0x10], rbp\n\t"
        "mov [rax+0x18], rbx\n\t"
        "mov [rax+0x20], r12\n\t"
        "mov [rax+0x28], r13\n\t"
        "mov [rax+0x30], r14\n\t"
        "mov [rax+0x38], r15\n\t"
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
    register Context *context __asm__("rax");
    context = &contexts.items[contexts.current];

    __asm__(
        "mov rsp, [rax+0x08]\n\t"
        "mov rbp, [rax+0x10]\n\t"
        "mov rbx, [rax+0x18]\n\t"
        "mov r12, [rax+0x20]\n\t"
        "mov r13, [rax+0x28]\n\t"
        "mov r14, [rax+0x30]\n\t"
        "mov r15, [rax+0x38]\n\t"
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