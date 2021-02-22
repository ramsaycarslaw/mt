var add = \x, y -> {
  return x + y;
};

var hello = \x -> return x += 1;

print add(10, 12);

print hello(10);

