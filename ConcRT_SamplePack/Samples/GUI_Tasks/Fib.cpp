#include "stdafx.h"
//Computes the fibonacci number of 'n' serially
int fib(int n)
{
    if (n< 2)
        return n;
    int n1, n2;
    n1 = fib(n-1);
    n2 = fib(n-2);
    return n1 + n2;
}
