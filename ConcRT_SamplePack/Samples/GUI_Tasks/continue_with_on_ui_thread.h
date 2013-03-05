#include "..\..\concrtextras\ppltasks.h"
using namespace Concurrency::samples;

template<typename T, typename _Function>
void continue_with_on_ui_thread( task<T> task, _Function func )
{
    HANDLE handle = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        L"task_event");

    T result;
    task.continue_with( [&result, handle](T t) 
    {
        result = t;
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
            func(result);
            break;
        }
    }
}
