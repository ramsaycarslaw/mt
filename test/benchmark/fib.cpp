#include <iostream>
#include <chrono>

using namespace std::chrono;
using namespace std;

class GFG{
     
public:
long fib(long n) {
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
};

int main(void) {
  
  auto start = high_resolution_clock::now(); 
  GFG gfg;
  cout << gfg.fib(50) << endl;

  auto end = high_resolution_clock::now(); 
  auto duration = duration_cast<nanoseconds>(end-start);
 
  cout << "Elapsed (ns): ";
  cout << duration.count() << endl;

  return 0;
}
