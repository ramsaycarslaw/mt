#include <time.h>
#include <stdio.h>

long fib(long n) 
{
  long  a = 0;
  long b = 1;
  long temp = 0;

  for (long i = 0; i < n; i++) 
  {
    temp = a+b; 
    a=b;
    b=temp;
  }
  return a;
}

int main(void) 
{
  clock_t start = clock();

  long answer = fib(50);

  clock_t end = clock();
  double elapsed = (double)(end-start) / CLOCKS_PER_SEC;

  printf("Found answer %ld\nElapsed %lf\n", answer, elapsed);

  return 0;
}

