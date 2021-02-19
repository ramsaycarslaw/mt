fn fib(n) {
  var a = 0;
  var b = 1;

  for (var i = 0; i < n; i += 1) {
    var temp = a + b;
    a = b;
    b = temp;
  }
  return a;
}


var start = clock();
var answer =  fib(50);
var elapsed = clock() - start;
print "Found answer " + string(answer);
print "Elapsed: " + string(elapsed);

