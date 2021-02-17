for (var i = 0; i < 100; i = i + 1) {
  if (i > 50) {
    break;
  }
  print i;
}

var x = 0;

// test continue
while (x < 100) {
  if (x%2 == 0) {
    continue;
  } else {
    print x;
  }
  x = x + 1;
}
