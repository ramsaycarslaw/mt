public class fib {
  static long fibbonacci(long n) {   
    long  a = 0;
    long b = 1;
    long temp = 0;

    for (long i = 0; i < n; i++) {
      temp = a+b; 
      a=b;
      b=temp;
    }
    return a;
  }

  public static void main( String[] args ) {

    long startTime = System.nanoTime();
    long answer = fibbonacci(50);
    long endTime = System.nanoTime();

    System.out.print("Found answer: ");
    System.out.println(answer);
    System.out.print("Elapsed: ");
    // get into seconds
    System.out.println((endTime - startTime) / 1000000);
  }
}
