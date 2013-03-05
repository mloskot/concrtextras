//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: controller_agent.cpp
//
//
//--------------------------------------------------------------------------
#pragma once
#include "model_agent.h"
#include "view_agent.h"
#include "..\..\concrtextras\concrt_extras.h"
#include "..\..\concrtextras\connect.h"
using namespace Concurrency::samples;

class view_agent;
class controller_agent : public agent
{
    
    shared_ptr<transformer<signal, function<void(void)>>> m_updateWorkSerial;
    shared_ptr<transformer<signal, function<void(void)>>> m_updateWorkParallel;
    shared_ptr<transformer<signal, function<void(void)>>> m_updateWorkParallel2;
    shared_ptr<transformer<signal, function<void(void)>>> m_updateWorkPaused;
    shared_ptr<transformer<signal, size_t>>               m_increaseBodyCount;
    shared_ptr<transformer<signal, size_t>>               m_decreaseBodyCount;

    shared_ptr<transformer<signal, size_t>>               m_increaseGravity;
    shared_ptr<transformer<signal, size_t>>               m_decreaseGravity;

    shared_ptr<transformer<signal, signal>>               m_compute;
    shared_ptr<transformer<signal, signal>>               m_sort;
    shared_ptr<transformer<signal, signal>>               m_explode;
    shared_ptr<transformer<signal, signal>>               m_render;

    //buffer for completion status
    shared_ptr<call<agent_status>> m_agent_complete_call;
    task_group                     m_tasks;
    overwrite_buffer<function<void(void)>> 	m_workBuffer;
public:
    unbounded_buffer<signal> m_workCompletedBuffer;
private:
    int prevAvgDistance;
    double curAvgDist;
    bool isExpanding;
    bool wasCollapsing;



    void connect_render_loop(view_agent& view, model_agent& model)
    {
        prevAvgDistance = INT_MAX;
        curAvgDist = INT_MAX;

        isExpanding = true;
        wasCollapsing = true;
        
        send(&m_workBuffer,function<void(void)>([](){;}));
        auto task = [&](){receive(m_workBuffer)();};
        auto compute_nbodies = [&,task]()
        {
            auto begin = GetTickCount();

            //get the current work item and execute it.
            receive(m_workBuffer)();

            auto end = GetTickCount();
            send(&fps_buffer, 1000.0/(end-begin));
        };
        auto check_for_explosion = [&]()
        {
            //set the previous state
            wasCollapsing = !isExpanding;	

            //update isExpanding
            isExpanding = (curAvgDist > prevAvgDistance);

            if(isExpanding && wasCollapsing)
            {
                view.window.nbodies.body_color = D2D1::ColorF::YellowGreen;
            }
            //set previous = current
            prevAvgDistance = (int) curAvgDist;

        };
        auto calculate_distance = [&](size_t numParticles)->int
        {
            m_tasks.run_and_wait([&](){
            double dist = 0.;
            vector<double> distance(numParticles,0);

            for(size_t i = 0; i < numParticles; ++i)
            {
                // distance [i] = dot_product(body[i].pos);
                distance [i] = model.bodies[i].pos[0]*model.bodies[i].pos[0] + model.bodies[i].pos[1]*model.bodies[i].pos[1] + model.bodies[i].pos[2]*model.bodies[i].pos[2];
                dist += distance[i];
            }

            sort(distance.begin(),distance.end());

            curAvgDist = distance[numParticles/4];
            send(distance_buffer, (double) curAvgDist);
            });
            return (int) curAvgDist;
        };
        
        auto render = [&](size_t numParticles)
        {

            dimensions curSize = receive(view.window.size_buffer);

            for (size_t i = 0; i < numParticles; i++)
            {
                //very naive culling
                auto is_visible = [](size_t width, size_t height, float x, float y)
                {
                    return (x > 0) && (y > 0 )&& (x < width) && (y < height);
                };
                
                
                float x = (float) model.bodies[i].pos[0]/200 + curSize.width/2;
                float y = (float) model.bodies[i].pos[1]/200 + curSize.height/2;
                float z = (float) model.bodies[i].pos[2]/2000 + 200;
                
                if(is_visible(curSize.width, curSize.height,x,y))
                    view.window.add_point(x,y,z);
            }

            view.window.render_points(); 
        };
        
        m_compute = make_shared<transformer<signal, signal>>(
            [=](const signal& in)->signal{
                compute_nbodies();
                return signal();
        });
        
        m_sort = make_shared<transformer<signal, signal>>([=](const signal& in)->signal{
            calculate_distance(receive(particles_buffer));
            return signal();
        });
        
        m_explode = make_shared<transformer<signal, signal>>([=](const signal& in)->signal{
            check_for_explosion();
            return signal();
        });
        
        m_render = make_shared<transformer<signal, signal>>([=](const signal& in)->signal{
            render(receive(particles_buffer));
            return signal();
        });

        connect(m_compute, m_sort);
        connect(m_sort, m_explode);
        connect(m_explode, m_render);

        send(m_render.get(), signal());
    }
    
    void run()
    {
        //get the view and the model
        view_agent& view 	= *receive(view_buffer);
        model_agent& model 	= *receive(model_buffer);

#pragma region controller setup code
        function<void(void)> bodiesSerial = [&]()
        {

            send(view.status_buffer, wstring(L"running serial"));
            size_t numParticles = receive(particles_buffer);
            m_tasks.run_and_wait([&](){model.nbodies_serial(numParticles);});
            send(m_workCompletedBuffer,signal());
            disconnect(m_render);
            connect(m_render, m_compute);
        };

        auto bodiesParallel = [&]()
        {
            send(view.status_buffer, wstring(L"running parallel"));
            size_t numParticles = receive(particles_buffer);
            m_tasks.run_and_wait([&](){model.nbodies_parallel(numParticles);});
            send(m_workCompletedBuffer,signal());
            disconnect(m_render);
            connect(m_render, m_compute);

        };

        auto bodiesParallel2 = [&]()
        {
            send(view.status_buffer, wstring(L"running parallel 2"));
            size_t numParticles = receive(particles_buffer);
            m_tasks.run_and_wait([&](){model.nbodies_cache_friendly(numParticles);});
            send(m_workCompletedBuffer,signal());

        };
        auto bodiesPaused = [&]()
        {
            send(view.status_buffer, wstring(L"paused"));
            send(m_workCompletedBuffer,signal());
        };

        typedef function<void(void)> nullfunc;

        //subscribe to click events
        m_updateWorkSerial = make_shared<transformer<signal, nullfunc>>([=](signal)->function<void(void)>
        { 
            disconnect(m_render);
            connect(m_render, m_compute);
            return bodiesSerial;
        });
        m_updateWorkParallel = make_shared<transformer<signal, nullfunc>>([=](signal)->function<void(void)>
        { 
            disconnect(m_render);
            connect(m_render, m_compute);
            return bodiesParallel;
        });
        m_updateWorkParallel2 = make_shared<transformer<signal,function<void(void)>>>([=](signal)->function<void(void)>
        { 
            disconnect(m_render);
            connect(m_render, m_compute);
            return bodiesParallel2;
        });
        m_updateWorkPaused = make_shared<transformer<signal,function<void(void)>>>([=,&view](signal)->function<void(void)>
        {
           // view.window.nbodies.paused = true;
            disconnect(m_render);
            return bodiesPaused;
        });

        m_increaseBodyCount = make_shared<transformer<signal,size_t>>([&](signal)->size_t
        {
            return receive(particles_buffer) + 100;
        });

        m_decreaseBodyCount = make_shared<transformer<signal,size_t>>([&](signal)->size_t
        {
            return receive(particles_buffer) - 100;
        });

        m_increaseGravity = make_shared<transformer<signal,size_t>>([&](signal)->size_t
        {
            model.GRAVITY += 100;
            return receive(gravity_buffer) + 100;
        });
        m_decreaseGravity = make_shared<transformer<signal,size_t>>([&](signal)->size_t
        {
            model.GRAVITY -=100;
            return receive(gravity_buffer) - 100;
        });

        //notify the model of changes to the number of particles
        connect(particles_buffer, model.particle_count_buffer);
        connect(particles_buffer, view.body_count_buffer);

        //notify the model of changes to the number of particles
        connect(gravity_buffer, view.gravity_buffer);

        //connect status to the view
        connect(fps_buffer, view.fps_buffer);
        connect(distance_buffer, view.distance_buffer);

        //connect to the view
        connect(view.serial_button_clicked_buffer, m_updateWorkSerial);
        connect(view.parallel_button_clicked_buffer, m_updateWorkParallel);
        connect(view.parallel2_button_clicked_buffer, m_updateWorkParallel2);
        connect(view.pause_button_clicked_buffer, m_updateWorkPaused);

        connect(view.bodies_increase_clicked_buffer, m_increaseBodyCount);
        connect(m_increaseBodyCount, particles_buffer);

        connect(view.bodies_decrease_clicked_buffer, m_decreaseBodyCount);
        connect(m_decreaseBodyCount, particles_buffer);

        connect(view.gravity_increase_clicked_buffer, m_increaseGravity);
        connect(m_increaseGravity, gravity_buffer);
        connect(view.gravity_decrease_clicked_buffer, m_decreaseGravity);
        connect(m_decreaseGravity, gravity_buffer);

        //connect these to the work
        connect(m_updateWorkSerial, m_workBuffer);
        connect(m_updateWorkParallel, m_workBuffer);
        connect(m_updateWorkParallel2, m_workBuffer);
        connect(m_updateWorkPaused, m_workBuffer);

        send(gravity_buffer, size_t(model.GRAVITY));


        int prevAvgDistance = INT_MAX;
        int curAvgDist = INT_MAX;

        bool isExpanding = true;
        bool wasCollapsing = true;

#pragma endregion
        connect_render_loop(view, model);

        //agent completion status call
        m_agent_complete_call = make_shared<call<agent_status>>([this](agent_status status){
            
            //disconnect the render loop
            disconnect(m_compute);
            disconnect(m_sort);
            disconnect(m_render);
            disconnect(m_explode);

            //cancel any remaining tasks
            m_tasks.cancel();
            
            //wait for work to complete
            m_tasks.wait();
            
            //set the status to done
            this->done();
        },[](agent_status in){return in == agent_done;});

        //connect the status of the view's window to the status of the controller
        auto statusPort = view.window.status_port();
        connect(statusPort, m_agent_complete_call);
 }
public:
    controller_agent(){}
    overwrite_buffer<double>                fps_buffer;
    overwrite_buffer<double>                distance_buffer;
    overwrite_buffer<size_t>                particles_buffer;
    overwrite_buffer<size_t>                gravity_buffer;

    overwrite_buffer<window_agent*>         window_buffer;
    overwrite_buffer<model_agent*>          model_buffer;
    overwrite_buffer<view_agent*>           view_buffer;
};