var x = 10;
var str = "";

switch (x) 
{
  case 0:
    str = "0";

  case 1:
    str = "1";

  case 10:
    str = "10";

  default:
    str = "Not Found";
}

assert.Equals("10", str);
