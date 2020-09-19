// integer division of two numbers
fn div(x, y) 
{
    if (x == 0) 
    {
        return 0;
    }

    var divCount = 0;

    while (x - y > y) 
    {
        x = x - y;
        divCount = divCount + 1;
    }
    x = x - y;
    divCount = divCount + 1;

    if (x - y == 0) 
    {
        divCount = divCount + 1;
    }
    return divCount;
}

// prints the minumum of two numbers
fn min(x, y) 
{
    if (x > y) 
    {
        return y;
    }
    return x;
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

print div(100, 3);
print min(10, 2);
print mod(263, 7);

