gcc -c coroutines.c -o coroutines.o -masm=intel -O1
gcc main.c coroutines.o -o main