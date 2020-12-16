fn sumToN(n) {
   var sum = 0;
   for (var i = 0; i < n; i = i + 1) {
       sum = sum + i;
   }
   return sum;
}

print sumToN(1000);
