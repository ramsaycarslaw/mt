var higherOrderFunc = \x, func -> {
  return x + func(x);
};

print higherOrderFunc(2, \x -> {return x * x; });  // prints 6

print higherOrderFunc(2, \x -> {return x + 3; });  // prints 7
