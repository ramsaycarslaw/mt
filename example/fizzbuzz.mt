fn fizzBuzz(n) 
{
    for (var i = 0; i < n; i = i + 1) 
    {
        var buffer = "";

        if (mod(i, 3) == 0) 
        {
            buffer = buffer + "fizz";
        }

        if (mod(i, 5) == 0) 
        {
            buffer = buffer + "buzz";
        }

        print buffer;
        print i;
    }    
}

// modulus of two numbers x and y
fn mod(x, y) 
{
    // check for division by zero
    if (x == 0) 
    {
        return 0;
    }

    while (x - y > y) 
    {
        x = x - y;
    }
    x = x - y;

    if (x - y == 0) 
    {
        return 0;
    }
    return x;
}

fizzBuzz(16);
