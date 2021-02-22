var test = "";

fn afterFn() {
  test = "after";
}

fn main() {
  defer afterFn;

  test = "before";

  return;
}

main();

print test;
assert.Equals(test, "after");
