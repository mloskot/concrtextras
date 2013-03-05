// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: view_agent.h
//
//
//--------------------------------------------------------------------------
#pragma once
#include <memory>

#include "..\..\concrtextras\connect.h"
#include "winwrap.h"
using namespace ::std;
using namespace ::winwrap;
using namespace Concurrency::samples;

struct signal
{

};
template <class buffer_type>
class signal_to_buffer
{
    buffer_type& buffer;
public:
    signal_to_buffer(buffer_type& buf): buffer(buf){}
    void operator()()
    {
        send(&buffer,signal());
    }
};
template <class buffer_type>
signal_to_buffer<buffer_type> make_signal_buffer(buffer_type& buf)
{
    return signal_to_buffer<buffer_type>(buf);
};
class view_agent : public agent
{
private:
#pragma region private gui controls
    //mode label
    shared_ptr<label> 			m_statusHeaderLabel;
    shared_ptr<label> 			m_statusLabel;

    //frames per second
    shared_ptr<label> 			m_fpsLabel;

    //average distance
    shared_ptr<label> 			m_distanceLabel;

    //gravity
    shared_ptr<label> 			m_gravityLabel;
    shared_ptr<button> 			m_gravityDecreaseButton;
    shared_ptr<button> 			m_gravityIncreaseButton;

    //bodies
    shared_ptr<label> 			m_numBodiesLabel;
    shared_ptr<button>			m_numBodiesDecreaseButton;
    shared_ptr<button> 			m_numBodiesIncreaseButton;

    //mode toggles
    shared_ptr<button> 			m_serialModeButton;
    shared_ptr<button> 			m_parallelModeButton;
    shared_ptr<button> 			m_parallelMode2Button;
    shared_ptr<button> 			m_pauseButton;

    //async update functions
    shared_ptr<call<double>> 	m_updateFps;
    shared_ptr<call<double>> 	m_updateDistance;
    shared_ptr<call<size_t>> 	m_updateGravity;
    shared_ptr<call<size_t>> 	m_updateBodies;
    shared_ptr<call<wstring>>   m_updateStatus;

    overwrite_buffer<signal> 	m_parallel_button_clicked_buffer;
    overwrite_buffer<signal> 	m_serial_button_clicked_buffer;
    overwrite_buffer<signal>    m_parallel2_button_clicked_buffer;
    overwrite_buffer<signal> 	m_pause_button_clicked_buffer;

    overwrite_buffer<signal>    m_gravity_increase_clicked_buffer;
    overwrite_buffer<signal>    m_gravity_decrease_clicked_buffer;

    overwrite_buffer<signal>    m_bodies_increase_clicked_buffer;
    overwrite_buffer<signal>	m_bodies_decrease_clicked_buffer;
    
#pragma endregion
public:
    //the window
    window_agent window;

    //constructor
    template <typename type>
    view_agent(type name, size_t width, size_t height):
        window(name, width, height),
        parallel_button_clicked_buffer (&m_parallel_button_clicked_buffer),
        parallel2_button_clicked_buffer(&m_parallel2_button_clicked_buffer),
        pause_button_clicked_buffer    (&m_pause_button_clicked_buffer),
        serial_button_clicked_buffer   (&m_serial_button_clicked_buffer),
        gravity_increase_clicked_buffer(&m_gravity_increase_clicked_buffer),
        gravity_decrease_clicked_buffer(&m_gravity_decrease_clicked_buffer),
        bodies_increase_clicked_buffer (&m_bodies_increase_clicked_buffer),
        bodies_decrease_clicked_buffer (&m_bodies_decrease_clicked_buffer)
    {}

    //public message blocks for communication
    ISource<signal>*                const parallel2_button_clicked_buffer;
    ISource<signal>*                const pause_button_clicked_buffer;
    ISource<signal>*                const parallel_button_clicked_buffer;
    ISource<signal>*                const serial_button_clicked_buffer;
    
    ISource<signal>*                const gravity_increase_clicked_buffer;
    ISource<signal>*                const gravity_decrease_clicked_buffer;

    ISource<signal>*                const bodies_increase_clicked_buffer;
    ISource<signal>*                const bodies_decrease_clicked_buffer;


    //status buffers
    overwrite_buffer<double>  fps_buffer;
    overwrite_buffer<double>  distance_buffer;
    overwrite_buffer<size_t>  body_count_buffer;
    overwrite_buffer<size_t>  gravity_buffer;
    overwrite_buffer<wstring> status_buffer;

private:
    void run()
    {
        //create the gui controls
#pragma region create_gui_controls
        //fps
        m_statusHeaderLabel		    = make_shared<label>(L"Status: ");
        m_statusLabel				= make_shared<label>(L"                   ", right_of(*m_statusHeaderLabel));
        m_fpsLabel 					= make_shared<label>(L"FPS               ", directly_below(*m_statusHeaderLabel));
        m_distanceLabel 			= make_shared<label>(L"avg dist:               ", right_of(*m_fpsLabel));

        //gravity
        m_gravityLabel 				= make_shared<label> (L"gravity:        ",directly_below(*m_fpsLabel));
        m_gravityDecreaseButton 	= make_shared<button>(L"-",right_of(*m_gravityLabel));
        m_gravityIncreaseButton  	= make_shared<button>(L"+",right_of(*m_gravityDecreaseButton));

        //bodies
        m_numBodiesLabel			= make_shared<label> (L"body count:     ",directly_below(*m_gravityLabel));
        m_numBodiesDecreaseButton	= make_shared<button>(L"-",directly_below(*m_gravityDecreaseButton));
        m_numBodiesIncreaseButton	= make_shared<button>(L"+",right_of(*m_numBodiesDecreaseButton));

        //mode
        m_serialModeButton          = make_shared<button> (L"Serial", directly_below(*m_numBodiesLabel));
        m_parallelModeButton        = make_shared<button> (L"Parallel", right_of(*m_serialModeButton));
        m_parallelMode2Button       = make_shared<button> (L"Parallel Faster", right_of(*m_parallelModeButton));
        m_pauseButton               = make_shared<button> (L"Pause", right_of(*m_parallelMode2Button));

#pragma endregion
        //add them to the window
#pragma region add_controls_to_window
        add_control(window, m_statusHeaderLabel);
        add_control(window, m_statusLabel);
        add_control(window, m_fpsLabel);
        add_control(window, m_distanceLabel);

        add_control(window, m_gravityLabel);
        add_control(window, m_gravityDecreaseButton);
        add_control(window, m_gravityIncreaseButton);

        add_control(window, m_numBodiesLabel);
        add_control(window, m_numBodiesDecreaseButton);
        add_control(window, m_numBodiesIncreaseButton);

        add_control(window, m_serialModeButton);
        add_control(window, m_parallelModeButton);
        add_control(window, m_parallelMode2Button);
        add_control(window, m_pauseButton);

        //connect the on click events to the output buffers
        m_gravityIncreaseButton->events.on_click 	= make_signal_buffer(m_gravity_increase_clicked_buffer);
        m_gravityDecreaseButton->events.on_click 	= make_signal_buffer(m_gravity_decrease_clicked_buffer);

        m_numBodiesIncreaseButton->events.on_click	= make_signal_buffer(m_bodies_increase_clicked_buffer);
        m_numBodiesDecreaseButton->events.on_click 	= make_signal_buffer(m_bodies_decrease_clicked_buffer);	

        m_serialModeButton->events.on_click 		= make_signal_buffer(m_serial_button_clicked_buffer);
        m_parallelModeButton->events.on_click 		= make_signal_buffer(m_parallel_button_clicked_buffer);
        m_parallelMode2Button->events.on_click 		= make_signal_buffer(m_parallel2_button_clicked_buffer);		
        m_pauseButton->events.on_click              = make_signal_buffer(m_pause_button_clicked_buffer);	
        

        //create the private text update message blocks
        m_updateFps = make_shared<call<double>> ([&](double fps)
        {
            m_fpsLabel->m_text = number_to_wstring(L"FPS: ", fps);
        });

        m_updateDistance = make_shared<call<double>> ([&](double dist)
        {
            m_distanceLabel->m_text = number_to_wstring(L"avg dist: ", dist);
        });
        m_updateBodies = make_shared<call<size_t>> ([&](size_t count)
        {
            m_numBodiesLabel->m_text = number_to_wstring(L"body count: ", count);
        });
        m_updateGravity = make_shared<call<size_t>> ([&](size_t count)
        {
            m_gravityLabel->m_text = number_to_wstring(L"gravity: ", count);
        });
        m_updateStatus = make_shared<call<wstring>> ([&](wstring in)
        {
            m_statusLabel->m_text = in;
        });
        
    #pragma endregion
        //connect these to the status update buffers
        connect(fps_buffer, m_updateFps);
        connect(body_count_buffer, m_updateBodies);
        connect(gravity_buffer, m_updateGravity);
        connect(status_buffer, m_updateStatus);
        connect(distance_buffer, m_updateDistance);

        //start the view
        window.start();
    }
};