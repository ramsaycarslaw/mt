use "otherfile.mt";
use "nested/subtract.mt";
use "duplicate.mt";

if (10 < 12) 
{
  use "conditional.mt";
}

assert.Equals(add(10, 12), 22);
assert.Equals(subtract(12, 10), 2);
assert.Equals(multiply(10, 12), 120);
