var test = "";

fn main() {
  defer test="after";

  test = "before";

  return;
}

main();

print test;
assert.Equals(test, "after");
