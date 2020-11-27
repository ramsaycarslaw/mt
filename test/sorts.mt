fn bubble(arr) 
{
    for (var i = 0; i < len(arr); i = ++i) 
    {
        for (var j = i; j < len(arr); j = ++j) 
        {
            if (arr[i] > arr[j]) 
            {
                var temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
    return arr;    
}

var foo = [1,3847,398,1238,538998,23,81734,38947,43783];

var start = clock();
bubble(foo);
var elapsed = clock() - start;
print elapsed;
