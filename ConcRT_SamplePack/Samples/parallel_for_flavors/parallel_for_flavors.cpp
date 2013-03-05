// parallel_for_flavors.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\..\ConcRTExtras\ppl_extras.h"

using namespace Concurrency;

int main()
{
    printf("parallel_for:\n");
    parallel_for(0,10,[=](int i) {
        printf("i=%d\n",i);
    });

    printf("parallel_for_fixed:\n");
    samples::parallel_for_fixed(0,10,[=](int i) {
        printf("i=%d\n",i);
    });

    const int size = 5;
    char myArray[size] = {'a', 'b', 'c', 'd', 'e'};

    printf("parallel_for_each_fixed:\n");
    samples::parallel_for_each_fixed(&myArray[0], &myArray[size], [=](char c) {
        printf("%c\n",c);
    });

}

