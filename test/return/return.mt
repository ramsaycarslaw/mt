fn someFunc() 
{
  return "Correct";
}

fn main() 
{
  return someFunc();  
}

assert.Equals(main(), someFunc());
