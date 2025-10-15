let arr = [1,2,3,4,5,6,7,8,9];

// we have the basic template
print "sugar";
for x in arr {
  print x;
}

// this can compile to
print "built in"
let i = 0;
while (i < len(arr)) {
    print x;
    i++;
}
