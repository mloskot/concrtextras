// event.cpp : Defines the entry point for the console application.
//
// compile with: /EHsc
#include <windows.h>
#include <concrt.h>
#include <concrtrm.h>
#include <ppl.h>

using namespace Concurrency;
using namespace std;

class WindowsEvent
{
    HANDLE m_event;
public:
    WindowsEvent()
        :m_event(CreateEvent(NULL,TRUE,FALSE,TEXT("WindowsEvent")))
    {
    }

    ~WindowsEvent()
    {
        CloseHandle(m_event);
    }

    void set()
    {
        SetEvent(m_event);
    }

    void wait(int count = INFINITE)
    {
        WaitForSingleObject(m_event,count);
    }
};

template<class EventClass>
void DemoEvent()
{
    EventClass e;
    volatile long taskCtr = 0;

    //create a taskgroup and schedule multiple copies of the task
    task_group tg;
    for(int i = 1;i <= 8; ++i)
        tg.run([&e,&taskCtr]{            
            
			//Simulate some work
            Sleep(100);

            //increment our task counter
            long taskId = InterlockedIncrement(&taskCtr);
            printf_s("\tTask %d waiting for the event\n", taskId);

            e.wait();

            printf_s("\tTask %d has received the event\n", taskId);

    });

    //pause noticably before setting the event
    Sleep(1500);

    printf_s("\n\tSetting the event\n");

    //set the event
    e.set();

    //wait for the tasks
    tg.wait();
}

int main ()
{
    //Create a scheduler that uses two and only two threads.
    CurrentScheduler::Create(SchedulerPolicy(2, MinConcurrency, 2, MaxConcurrency, 2));
    
    //When the cooperative event is used, all tasks will be started
    printf_s("Cooperative Event\n");
    DemoEvent<event>();

    //When a Windows Event is used, unless this is being run on Win7 x64
    //ConcRT isn't aware of the blocking so only the first 2 tasks will be started.
    printf_s("Windows Event\n");
    DemoEvent<WindowsEvent>();

    return 0;
}