// program to test tuple implementation

var tuple = (1,2,3,4,5,6,7,8,9,10);

for (var i = 0; i < len(tuple); i += 1) {
  assert.Equals(tuple[i], i+1);
} 
