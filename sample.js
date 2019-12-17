function exponent(base, exp)
{
    var result = base;
    // x = y;
    for (var i=0; i < exp; i++) {
        result *= base;
    }
    return result;
}


var i = exponent(10, 4);

print("Result = " + i);