// make.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "DataReader.h"
#include "..\..\concrtextras\ppltasks.h"

using namespace std;
using namespace Concurrency::samples;

#pragma optimize("", off)
int fib(int n) // use this to simulate a slow operation
{
    if(n<2) return n;
    return fib(n-1) + fib(n-2);
}
#pragma optimize("",on)

int Build(string project)
{
    printf("Start building '%s'\n", project.c_str());
    fib(40);
    printf("End building '%s'\n", project.c_str());
    return 0;
}

map<string,task<void>> all_tasks;

task<void> schedule_task(string_string_list_map solution, string name)
{
	auto it=all_tasks.find(name);
	if(it != all_tasks.end()) // already scheduled? then don't schedule again
	{
		return it->second;
	}

	vector<task<void>> dependencies;
	for each( auto dep in solution[name] )
	{
		auto proj_name = dep;
		auto t = schedule_task(solution,proj_name);
		dependencies.push_back(t);
	}

	auto t = when_all(dependencies.begin(), dependencies.end()).continue_with([=] {
		Build(name);
	});

	all_tasks.insert(std::map<string,task<void>>::value_type ( name, t ));
	return t;
}

int wmain(int argc, wchar_t *argv[])
{
	if( argc != 2 ) return -1;

	auto solution = XMLReader(argv[1]).GetParams();

	for each( auto it in solution )
	{
		schedule_task(solution,it.first);
	}

	for each(auto t in all_tasks)
	{
		t.second.wait();
	}

	return 0;
}

