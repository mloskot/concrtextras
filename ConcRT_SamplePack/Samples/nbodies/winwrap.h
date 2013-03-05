// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: winwrap.h
//
//
//--------------------------------------------------------------------------
#pragma once
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#pragma comment( lib, "dwmapi" )
#include <agents.h>
#include <string>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <agents.h>
#include <ppl.h>
#include <functional>
#include "d2wrap.h"
#include <tuple>
#include "vector.h"
#include <sstream>
#include <iomanip>

#include "..\..\concrtextras\concrt_extras.h"
namespace winwrap
{
    using namespace std;
    using namespace Concurrency;
    using namespace ray_lib;
    class message_handler;
    class window_class;
    class window_agent;
    namespace details
    {
#ifndef HINST_THIS
        EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THIS ((HINSTANCE)&__ImageBase)
#endif

        template <class WindProc>
        WNDCLASSEX create_registered_window_class(const string& class_name, WindProc& proc)
        {
            // Register the window class.
            WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
            wcex.style         = CS_HREDRAW | CS_VREDRAW;
            wcex.lpfnWndProc   = proc;
            wcex.cbClsExtra    = 0;
            wcex.cbWndExtra    = sizeof(LONG_PTR);
            wcex.hInstance     = HINST_THIS;
            wcex.hbrBackground = NULL;
            wcex.lpszMenuName  = NULL;
            wcex.hCursor       = LoadCursor(NULL, IDI_APPLICATION);
            wcex.lpszClassName = (LPCSTR)class_name.c_str();

            RegisterClassEx(&wcex);

            return wcex;
        }
        template <typename string_type>
        HWND create_overlapped_window(const string_type& className,const string_type& windowName, size_t width, size_t height, void* data)
        {
            return CreateWindow((LPCSTR)className.c_str(),
                (LPCSTR)windowName.c_str(),
                //WS_POPUP,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                (int)width,
                (int)height,
                NULL,
                NULL,
                HINST_THIS,
                data
                );
        }
        inline void set_window_ptr(HWND hwnd, void* data)
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (long) data);
        }
        template<class Type>
        Type* get_window_ptr(HWND hwnd)
        {
            auto ptr = (Type*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (ptr != NULL)
                return ptr;
            return nullptr;
        }
        inline HRESULT remove_window_frame(HWND hwnd)
        {
            HRESULT hr = NULL;
            MARGINS margins = { -1 };
            hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
            return hr;
        }
        inline bool show_and_update_window(HWND hwnd)
        {
            if(TRUE == ShowWindow(hwnd, SW_SHOWNORMAL))
                return (TRUE == UpdateWindow(hwnd));
            return false;
        }
       inline void run_message_loop_inline()
        {
            //GetMessage is a blocking API
            Concurrency::scoped_oversubcription_token t;

            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        struct dimensions
        {
            dimensions(size_t w, size_t h):width(w),height(h){}
            size_t width;
            size_t height;
        };
        struct win_params
        {
        public:
            win_params(WPARAM w, LPARAM l):wparam(w),lparam(l){}
            WPARAM wparam;
            LPARAM lparam;
        };
        template <class Type>
        LRESULT CALLBACK window_agent_wnd_proc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (message == WM_CREATE)
            {
                return 1;
            }
            Type* pThis = get_window_ptr<Type>(hwnd);
            if (pThis)
            {   
                if (pThis->process_messages(message,wParam,lParam))
                {
                    return DefWindowProc(hwnd, message, wParam, lParam);
                }
                return 0;
            }

            return DefWindowProc(hwnd, message, wParam, lParam);
        };
        template <class derived>
        class window
        {
        protected:
            shared_ptr<window_class> m_class;
            string m_className;

            message_handler m_messageHandler;
        public:
            overwrite_buffer<dimensions> size_buffer;
            d2d::render_target m_renderTarget;
            HWND m_hwnd;

            window(const string& className,size_t width = 300, size_t height= 300):m_className(className),m_class(nullptr)
            {
                send(size_buffer,dimensions(width,height));

            }
            void init()
            {
                dimensions m_windowSize = receive(size_buffer);

                string windowName(m_className);
                m_class = shared_ptr<window_class>(new window_class(m_className, window_agent_wnd_proc<derived>));

                m_hwnd = create_overlapped_window( m_className, 
                    windowName, 
                    m_windowSize.width, 
                    m_windowSize.height, 
                    nullptr);

                //handler for destruction
                m_messageHandler.add_message_handler(WM_DESTROY,[&](win_params msg)->int{
                    PostQuitMessage(0);
                    return 1;
                });

                m_messageHandler.add_message_handler(WM_SIZE,[&](win_params msg)->int{
                    dimensions size(LOWORD(msg.lparam), HIWORD (msg.lparam));
                    send(size_buffer, size);
                    m_renderTarget.target->Resize(D2D1::SizeU(size.width, size.height));
                    return 0;
                });

                m_renderTarget.init(m_hwnd);
                //remove_window_frame(m_hwnd);

                show_and_update_window(m_hwnd);

            }
            int process_messages(UINT msg, WPARAM wParam, LPARAM lParam)
            {
                return m_messageHandler.process_message(msg,win_params(wParam,lParam));
            }
        };
    }//end details namespace
    using namespace details;
    enum message_handler_status {ok = 0 , exit = 1};
    typedef class window_class
    {
    public:
        const string name;
        WNDCLASSEX wcex;
        template <class WindProc>
        window_class(string& class_name, WindProc& proc):name(class_name),wcex(create_registered_window_class(name, proc)){}
    } window_class;

    class message_handler
    {
        std::unordered_map<UINT,function<int(win_params)>> m;
    public:
        template <class Func>
        bool add_message_handler(UINT msg, Func&& f)
        {
            return m.insert(make_pair(msg,f)).second;
        }
        int process_message(UINT msg, win_params wp)
        {
            if(m.find(msg) != m.end())
            {
                return m[msg](wp);
                //return 0;
            }
            return 1;
        }
    };

    class event_handler
    {
    public:
        event_handler():on_click([](){;}){}
        typedef function<void(void)> nullfunc;
        nullfunc on_click;
    };
    class control
    {
    public:
        float left;
        float right;
        float top;
        float bottom;
        control(d2d::point2d point):left(point.x),right(point.y){}
        control(float x0 = 0.f, float y0 = 0.f):left(x0),top(y0){}
        event_handler events;
        virtual void on_draw(d2d::render_target& t){}
        virtual void on_click(d2d::render_target& t, int xPos, int yPos){}
        virtual bool intersects(int x, int y){return false;}
    };
    class control_list : public control
    {
    public:
        typedef control* control_ptr;
        void add_control(control_ptr c)
        {
            items.push_back(c);
        };
        void add_control(control& c)
        {
            items.push_back(&c);
        };
        void on_draw(d2d::render_target& t)
        {
            for_each(items.begin(),items.end(),[&](control_ptr& c)
            {
                c->on_draw(t);
            });
        }
        void on_click(d2d::render_target& t, int xPos, int yPos)
        {
            for_each(items.begin(),items.end(),[&](control_ptr& c)
            {
                if (c->intersects(xPos,yPos))
                    c->on_click(t, xPos, yPos);
            });
        }

    private:
        vector<control_ptr> items;
    };


    class label : public control
    {

        D2D1_RECT_F text_rectangle;
        D2D1_RECT_F button_rectangle;
        D2D1_ROUNDED_RECT roundRect;
    public:
        volatile bool clicked;
        bool intersects (int x, int y)
        {
            if ((left < x) && (x < right) && (top < y) && (y < bottom))
                return true;
            return false;
        }
        template<class stringType>
        label(stringType&& text, d2d::point2d point):control(point.x,point.y),m_text(wstring(text))
        {
            init();
        }

        template<class stringType>
        label(stringType&& text, float x = 0.f, float y = 0.f):control(x,y),m_text(wstring(text))
        {
            init();
        }
        void init()
        {
            const float padding = 5.f;
            const float charlength = (float) m_text.size();
            const float charwidth = 9.f;
            const float charheight = 28.f;
            const float buttonwidth = charlength * charwidth + 2*padding;

            bottom = top + charheight + padding;
            right = left + buttonwidth + padding;

            button_rectangle = D2D1::RectF(left + padding, top + padding, right, bottom);
            text_rectangle = D2D1::RectF(left + padding*2,top + padding*2, right, bottom+padding );
            roundRect = D2D1::RoundedRect(button_rectangle,5,5);
        }
        void on_draw(d2d::render_target& t)
        {
            ID2D1SolidColorBrush* brush;

            //create a brush on the target
            t.target->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::WhiteSmoke),
                &brush
                );

            //draw the caption
            t.target->DrawTextA(m_text.c_str(),(UINT)m_text.size(),t.text_format,&text_rectangle,brush);

            brush->Release();
        }
        void on_click(d2d::render_target& t, int xPos, int yPos)
        {
            clicked = true;
            events.on_click();
        }
        overwrite_buffer<function<void(void)>> click_func;
        wstring m_text;
    };

    class button : public control
    {
        D2D1_RECT_F text_rectangle;
        D2D1_RECT_F button_rectangle;
        D2D1_ROUNDED_RECT roundRect;
    public:
        volatile bool clicked;
        bool intersects (int x, int y)
        {
            if ((left < x) && (x < right) && (top < y) && (y < bottom))
                return true;
            return false;
        }
        template<class stringType>
        button(stringType&& text, d2d::point2d point):control(point.x,point.y),m_text(wstring(text))
        {
            init();
        }

        template<class stringType>
        button(stringType&& text, float x = 0.f, float y = 0.f):control(x,y),m_text(wstring(text))
        {
            init();
        }
        void init()
        {
            const float padding = 5.f;
            const float charlength = (float) m_text.size();
            const float charwidth = 9.f;
            const float charheight = 28.f;
            const float buttonwidth = charlength * charwidth + 2*padding;

            bottom = top + charheight + padding;
            right = left + buttonwidth + padding;

            button_rectangle = D2D1::RectF(left + padding, top + padding, right, bottom);
            text_rectangle = D2D1::RectF(left + padding*2,top + padding*2, right, bottom+padding );
            roundRect = D2D1::RoundedRect(button_rectangle,5,5);
        }
        void on_draw(d2d::render_target& t)
        {            
            ID2D1SolidColorBrush* brush;
            ID2D1SolidColorBrush* fillbrush;
            ID2D1SolidColorBrush* fillbrush_default;
            ID2D1SolidColorBrush* fillbrush_click;

            //create a brush on the target
            t.target->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::WhiteSmoke),
                &brush
                );

            t.target->CreateSolidColorBrush(
                D2D1::ColorF(.90f,.90f,.90f,.90f),
                &fillbrush_default
                );
            t.target->CreateSolidColorBrush(
                D2D1::ColorF(.99f,.99f,.99f,.9f),
                &fillbrush_click
                );
            if(clicked == true)
            {
                clicked = false;
                fillbrush = fillbrush_click;
            }
            else
            {
                fillbrush = fillbrush_default;
            }

            //draw the outline
            t.target->DrawRoundedRectangle(roundRect,brush);

            //draw the caption
            t.target->DrawTextA(m_text.c_str(),(UINT)m_text.size(),t.text_format,&text_rectangle,brush);

            brush->Release();
            fillbrush_default->Release();
            fillbrush_click->Release();
            
        }
        void on_click(d2d::render_target& t, int xPos, int yPos)
        {
            clicked = true;
            events.on_click();
        }
        overwrite_buffer<function<void(void)>> click_func;
        wstring m_text;
    };

    class nbody_points 
    {
        ID2D1SolidColorBrush* brush;

        typedef float3 point;
    public:
        //body color
        volatile D2D1::ColorF::Enum body_color;
        nbody_points():body_color(D2D1::ColorF::DarkRed)
        {
        }

        ~nbody_points()
        {

        }

        void init(d2d::render_target& t)
        {
        }

        void add_point(float x, float y, float z)
        {
            float size = (-0.01f*z + 3.7f) * 4.f;
            point p(x, y, size);
            input.push_back(p);
        }

        void render_points()
        {
            swap(input, current);
            input.clear();
        }

        void on_draw(d2d::render_target& t)
        {
            ID2D1GradientStopCollection *m_gradientStops;
            D2D1_GRADIENT_STOP gradientStops[2];
            gradientStops[0].color = D2D1::ColorF(body_color,0.3f);
            gradientStops[0].position = 0.0f;
            gradientStops[1].color = D2D1::ColorF(body_color, 0.f);
            gradientStops[1].position = 1.f;

            t.target->CreateGradientStopCollection(
                gradientStops,
                2,
                D2D1_GAMMA_2_2,
                D2D1_EXTEND_MODE_CLAMP,
                &m_gradientStops
                );

            vector<float3>& to_draw = current;
            for_each(to_draw.begin(),to_draw.end(),[&](float3& p)
            {
                ID2D1RadialGradientBrush* brush;

                t.target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                    D2D1::Point2F(p.x, p.y),
                    D2D1::Point2F(0.f,0.f),
                    p.z,
                    p.z),
                    m_gradientStops,
                    &brush
                    );

                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(p.x, p.y),max(.8f,p.z),max(.8f,p.z));

                t.target->FillEllipse(ellipse, brush);
                brush->Release();
            });
            m_gradientStops->Release();
        }

        void clear()
        {
            current.clear();
        }
    private:
        vector<float3> input;
        vector<float3> current;
    };

    class window_agent : public Concurrency::agent, public window<window_agent>
    {
        control_list controls;

    public:
        nbody_points nbodies;
        window_agent(const string& className,size_t width = 300, size_t height= 300):window(className,width,height){}
        void run()
        {
            control* c;
            while(try_receive(controls_buf,c))
                controls.add_control(c);
            init();
            nbodies.init(m_renderTarget);
            
            //handler for redraw
            m_messageHandler.add_message_handler(WM_PAINT,[&](win_params msg)->int
            {
                m_renderTarget.target->BeginDraw();
                d2d::clear_window(m_renderTarget);
                nbodies.on_draw(m_renderTarget);
                controls.on_draw(m_renderTarget);
                m_renderTarget.target->EndDraw();
                return 0;
            });

            //handler for resize
            m_messageHandler.add_message_handler(WM_SIZE,[&](win_params msg)->int
            {
                m_renderTarget.target->Resize(D2D1::SizeU(LOWORD(msg.lparam), HIWORD(msg.lparam)));
                return 0;
            });

            //handler for left click                          
            m_messageHandler.add_message_handler(WM_LBUTTONDOWN,[&](win_params msg)->int
            {
                auto xPos = GET_X_LPARAM(msg.lparam); 
                auto yPos = GET_Y_LPARAM(msg.lparam); 
                controls.on_click(m_renderTarget, xPos, yPos);
                return 0;
            });
            
            set_window_ptr(m_hwnd,this);

            run_message_loop_inline();

            agent::done();
        }

        void add_point(float x, float y, float z)
        {
            nbodies.add_point(x, y, z);
        }

        void render_points()
        {
            nbodies.render_points();
        }

        void wait()
        {
            agent::wait(this);
        }
        unbounded_buffer<control_list::control_ptr> controls_buf;
        void close()
        {
            PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
        };
    };

    template<class control_holder_type, class control_type>
    void add_control(control_holder_type* w,control_type& c)
    {
        send(w->controls_buf,(control_list::control_ptr)&c);
    }
    template<class control_holder_type, class control_type>
    void add_control(control_holder_type& w,control_type& c)
    {
        send(w.controls_buf,(control_list::control_ptr)&c);
    }
    template<class control_holder_type, class control_type>
    void add_control(control_holder_type* w,control_type* c)
    {
        send(w->controls_buf,(control_list::control_ptr)c);
    }
    template<class control_holder_type, class control_type>
    void add_control(control_holder_type& w,control_type* c)
    {
        send(w.controls_buf,(control_list::control_ptr)c);
    }
    template<class control_holder_type, class control_type>
    void add_control(control_holder_type* w,shared_ptr<control_type> c)
    {
        send(w->controls_buf,(control_list::control_ptr)c.get());
    }
    template<class control_holder_type, class control_type>
    void add_control(control_holder_type& w,shared_ptr<control_type> c)
    {
        send(w.controls_buf,(control_list::control_ptr)c.get());
    }
    template<class control_type>
    d2d::point2d right_of(control_type& c)
    {
        return d2d::point2d(c.right,c.top);
    }
    template<class control_type>
    d2d::point2d directly_below(control_type& c)
    {
        return d2d::point2d(c.left,c.bottom);
    }
    template<class counter_type>
    class spinner_class
    {
    public:
        typedef volatile counter_type cntr_type;
        wstring 					m_text;
        cntr_type					m_delta;
        cntr_type 					m_value;
        shared_ptr<label> 	 		m_label;
        shared_ptr<label> 			m_valueLabel;

        shared_ptr<button> 			m_buttonIncrease;
        shared_ptr<button> 			m_buttonDecrease;

        template<class text, class location_type>
        spinner_class(text& name, location_type&& l, cntr_type value = 0, cntr_type delta = 1):
        m_label(new label(name, l)), m_text(name), m_value(value), m_delta(delta)
        {

            m_buttonDecrease = make_shared<button>(L"-", right_of(*m_label));
            m_buttonIncrease = make_shared<button>(L"+", right_of(*m_buttonDecrease));

            m_buttonDecrease->events.on_click = [this](){this->decrease();};
            m_buttonIncrease->events.on_click = [this](){this->increase();};

            update_label();

            m_label->init();

        }
        template<class window_type>
        void add_to(window_type& w)
        {
            add_control(w , *m_label);
            add_control(w, *m_buttonDecrease);
            add_control(w, *m_buttonIncrease);
        }
        void increase()
        {
            m_value += m_delta;
            update_label();
        }
        void decrease()
        {
            m_value -= m_delta;
            update_label();
        }

        void update_label() 
        {
            wstringstream labelString;
            labelString << m_text  << (cntr_type) m_value;

            m_label->m_text = labelString.str();
        }
    };
    template <class number_type>
    wstring number_to_wstring (wstring str, number_type in, size_t precision = 4)
    {
        wstringstream result;
        result << str << setprecision(precision) << in;
        return  result.str();
    };
}

