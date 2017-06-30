function exponent(base, exp)
{
    var result = 1;
    for (var i=0; i < exp; i++) {
        result *= base;
    }
    return result;
}
