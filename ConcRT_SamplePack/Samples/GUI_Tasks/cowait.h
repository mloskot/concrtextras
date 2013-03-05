#include "..\..\concrtextras\ppltasks.h"
using namespace Concurrency::samples;

template<typename T>
void cowait( task<T> task )
{
    HANDLE handle = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        L"task_event");

    task.continue_with( [=](T t) 
    {
        SetEvent(handle);
    });

    MSG msg;
    while( true )
    {
        DWORD dwResult = MsgWaitForMultipleObjects( 1,
                                    &handle,
                                    FALSE,
                                    INFINITE,
                                    QS_ALLEVENTS );

        if( dwResult == WAIT_OBJECT_0+1 )
        {
            // Check if message arrived or not
            if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                // Translate and dispatch message
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            CloseHandle(handle);
            break;
        }
    }
}
