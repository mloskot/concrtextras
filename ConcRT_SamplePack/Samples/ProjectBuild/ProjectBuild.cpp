// ProjectBuild.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\..\concrtextras\ppltasks.h"

using namespace std;
using namespace Concurrency::samples;

struct Project
{
    string Name;
};

int Build(const Project& project)
{
    printf("Start building project '%s'\n", project.Name.c_str());
    Sleep(1000);
    printf("End building project '%s'\n", project.Name.c_str());
    return 0;
}

void main()
{
    Project A = {"A"}; //       A   B
    Project B = {"B"}; //      / \ /
    Project C = {"C"}; //     C   D
    Project D = {"D"}; //

    task<void> a([=]() {
        Build(A);
    });

    task<void> b([=]() {
        Build(B);
    });

    // Build C after A
    auto c = a.continue_with([=]() {
        Build(C);
    });

    // Build D after both A and B
    auto d = (a && b).continue_with([=]() {
        Build(D);
    });

    (c && d).wait();
}

