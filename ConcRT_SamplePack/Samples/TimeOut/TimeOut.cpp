#include "stdafx.h"
#include "..\..\concrtextras\ppltasks.h"

using namespace Concurrency;
using namespace Concurrency::samples;

int DownloadUrl(LPTSTR szUrl);

//
// Run function func with a given timeout, in milliseconds. If the function call times out, return the default value
//
template<typename Func>
auto run_function_with_timeout(Func func, decltype(func()) defaultValue, DWORD dwTimeout) -> decltype(func())
{
    typedef decltype(func()) Result;

    // task completion event that will be set when the timeout fires
    task_completion_event<Result> tce_timeout;

    // timer target is a dataflow block
    call<int> timer_target([=](int) {
        wprintf(L"timeout!\n");
        tce_timeout.set(defaultValue);
    });

    // create a start the timer
    timer<int> timeout_timer(dwTimeout, 0, &timer_target);
    timeout_timer.start();

    // wrap the function into a task
    task<Result> completion_task([=]() {
        return func();
    });

    // create the timeout task from the tce set in the call block
    task<Result> timeout_task(tce_timeout);

    // choice: return the result of whichever task completes first
    return (completion_task || timeout_task).
                continue_with([&](Result result) -> Result {
                    timeout_timer.stop();
                    return result;
                }).get();
}

//
// Given a task, return the same task with the timeout
//
template<typename T>
task<T> with_timeout(task<T> t, T defaultValue, DWORD dwTimeout)
{
    // task completion event that will be set when the timeout fires
    task_completion_event<T> tce_timeout;

    // timer target is a dataflow block
    auto timer_target = new call<int> ([=](int) {
        wprintf(L"timeout!\n");
        tce_timeout.set(defaultValue);
    });

    // create a start the timer
    auto timeout_timer = new timer<int> (dwTimeout, 0, timer_target);
    timeout_timer->start();

    // create the timeout task from the tce set in the call block
    task<T> timeout_task(tce_timeout);

    // The resulting task is a choice of two tasks
    return (t || timeout_task).
                continue_with([=](T result) -> T {
                    timeout_timer->stop();
                    delete timer_target;
                    delete timeout_timer;
                    return result;
                });
}

int main()
{
    LPTSTR urls[] = {
        L"http://www.microsoft.com",
        L"http://www.bing.com",
        L"http://www.google.com",
        L"http://http://www.wikipedia.org",
    };

    DWORD dwTimeout = 500; // in milliseconds

    for each(LPTSTR url in urls)
    {
        int result = run_function_with_timeout( ([=]() { return DownloadUrl(url);} ), -1, dwTimeout);

        if( result == -1 ) wprintf(L"Read from '%s' timed out\n", url);
        wprintf(L"Read %d bytes from '%s'\n", result, url);

        task<int> long_task([=]() {
            return DownloadUrl(url);
        });

        task<int> long_task_with_timeout = with_timeout(long_task, -1, dwTimeout);

        auto report = long_task_with_timeout.continue_with([=](int result) {
            if( result == -1 ) wprintf(L"Read from '%s' timed out\n", url);
            wprintf(L"Read %d bytes from '%s'\n", result, url);
        });

        report.wait(); // only necessary to allow the last task to finish before exiting the process
    }

    return 0;
}

