print("hello world");

function exponent(base, exp)
{
    var result = 1;
    for (var i=0; i < exp; i++) {
        result *= base;
    }

    return result;
}

for(var i = 0; i < 1000000; i++) {
    print("debugging ");
    print(i);
    print("\n");
    print(exponent(2, i));
    print("\n");
}
