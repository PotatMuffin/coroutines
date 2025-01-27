gcc -c coroutines.c -o coroutines.o -masm=intel
gcc main.c coroutines.o -o main