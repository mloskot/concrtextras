//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: d2wrap.h
//
//
//--------------------------------------------------------------------------
#pragma once
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#pragma comment( lib, "d2d1.lib" )
#pragma comment( lib, "dwrite.lib" )
namespace winwrap
{
    namespace d2d
    {
        namespace details
        {
            template <class InterfaceType>
            class safe_released_interface
            {
            public:
                InterfaceType** m_interface;
                safe_released_interface(InterfaceType** iface):m_interface(iface){};
                ~safe_released_interface()
                {
                };
            };
            template <class InterfaceType>
            class has_safe_released
            {
                safe_released_interface<InterfaceType> iface;
            public:
                has_safe_released(InterfaceType* i): iface(&i){}
            };
        }
        using namespace details;

        //raw types
        class point2d
        {
        public:
            FLOAT x;
            FLOAT y;
            point2d(FLOAT X = 0, FLOAT Y= 0):x(X),y(Y){};
        };
        template<class Interface>
        void
            SafeRelease(
            Interface **ppInterfaceToRelease
            )
        {
            if (*ppInterfaceToRelease != NULL)
            {
                printf("releasing\n");
                (*ppInterfaceToRelease)->Release();

                (*ppInterfaceToRelease) = NULL;
            }
        }
        //wrappers around the D2D interfaces
#define single_threaded_factory D2D1_FACTORY_TYPE_SINGLE_THREADED 
#define multi_threaded_factory D2D1_FACTORY_TYPE_MULTI_THREADED
        template <D2D1_FACTORY_TYPE policy_class>
        class d2dfactory
        {
        public:
            ID2D1Factory* m_pd2d_factory;
            HRESULT hr;
            d2dfactory():hr(D2D1CreateFactory(policy_class,&m_pd2d_factory)){};
            ~d2dfactory()
            {
                SafeRelease(&m_pd2d_factory);
            }
        };

        class render_target
        {
            d2dfactory<multi_threaded_factory> m_factory;
            render_target(const render_target& );
            render_target& operator=(const render_target& );

        public:
            render_target():target(nullptr){}
            ~render_target()
            {

                SafeRelease(&target);
                SafeRelease(&dwrite_factory);
                SafeRelease(&text_format);

            }
            ID2D1HwndRenderTarget* target;
            IDWriteFactory* dwrite_factory;
            IDWriteTextFormat* text_format;
            inline void init(HWND hwnd)
            {
                if(target != nullptr)
                    return;
                RECT rc;
                GetClientRect(hwnd, &rc);

                D2D1_SIZE_U size = D2D1::SizeU(
                    rc.right - rc.left,
                    rc.bottom - rc.top
                    );

                // Create a Direct2D render target.
                const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, 
                    D2D1_ALPHA_MODE_PREMULTIPLIED);

                const D2D1_RENDER_TARGET_PROPERTIES targetProperties = D2D1::RenderTargetProperties(
                    D2D1_RENDER_TARGET_TYPE_DEFAULT, format);

                m_factory.m_pd2d_factory->CreateHwndRenderTarget(
                    targetProperties,
                    D2D1::HwndRenderTargetProperties(hwnd, size),
                    &target
                    );

                DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&dwrite_factory)
                    );


                dwrite_factory->CreateTextFormat(
                    L"Consolas",                // Font family name.
                    NULL,                       // Font collection (NULL sets it to use the system font collection).
                    DWRITE_FONT_WEIGHT_REGULAR,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    16.0f,
                    L"en-us",
                    &text_format
                    );
                text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

            }
        };
        //helper functions
        template <class d2dfactory>
        inline point2d get_desktop_dpi(d2dfactory& f)
        {
            FLOAT X, Y;
            f.m_pd2d_factory->GetDesktopDpi(&X,&Y);
            return point2d(X,Y);
        }
        inline void clear_window(render_target& r)
        {
            r.target->SetTransform(D2D1::Matrix3x2F::Identity());
            r.target->Clear(D2D1::ColorF(255.0f, 255.0f, 255.0f,0.f));
        }
    }
}

