//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: model_agent.h
//
//
//--------------------------------------------------------------------------
#pragma once
#include <stdlib.h>
#include <agents.h>
#include <ppl.h>
#include <vector>
#include "vector.h"
using namespace Concurrency;
using namespace ray_lib;
#define grainsize 32
#define MAX_NUM_BODIES (20048)
#define GRAINSIZE 32
#define TIME_TICK 15
#define EPSILON (0.00001)

struct bodytype 
{
    double3 pos;
    double3 vel;
    double3 acc;
    double mass;
    unsigned char padding[128-(3*sizeof(double3) + sizeof(double))];
};
inline double generate_random(double low, double high)
{
    return (double)rand()*(high - low)/(double)RAND_MAX + low;
}

class model_agent : public agent
{
    inline void run()
    {
        initialize(MAX_NUM_BODIES);
        agent::done();
    }
public:
    model_agent():GRAVITY(100), bodies(MAX_NUM_BODIES)
    {
    }
    ~model_agent()
    {
    }
    volatile int GRAVITY;
    overwrite_buffer<size_t> particle_count_buffer;
    std::vector<bodytype> bodies;

    void initialize(int end, int begin = 0, float3 min = float3(-50000,-50000,-50000), float3 max = float3(50000,50000,50000))
    {
        double deltay = 0.100;
        double deltax = 0.100;
        int half = begin + (end - begin)/2;
        for (int i = begin; i < end; i+=2)
        {
            bodies[i].pos = double3(generate_random(min.x, max.x),generate_random(min.y, max.y), generate_random(min.z, max.z));
            bodies[i].vel = double3(generate_random(-0.1 - deltax, 0.1 - deltax),generate_random(-0.1 - deltay, 0.1 - deltay), generate_random(-0.1, 0.1));
            bodies[i].acc = double3(0,0,0);
            bodies[i].mass = generate_random(15000., 200000);

            bodies[i + 1].pos = double3(generate_random(min.x + 100000, max.x + 100000),generate_random(min.y, max.y), generate_random(min.z, max.z));
            bodies[i + 1].vel = double3(generate_random(-0.1 - deltax, 0.1 - deltax),generate_random(-0.1 - deltay, 0.1 - deltay), generate_random(-0.1, 0.1));
            bodies[i + 1].acc = double3(0,0,0);
            bodies[i + 1].mass = generate_random(15000., 200000);
        }
    }
    inline void update_body(int i)
    {
        bodies[i].vel = add(bodies[i].vel, multiply(TIME_TICK, bodies[i].acc));
        bodies[i].pos = add(bodies[i].pos, multiply(TIME_TICK, bodies[i].vel));
        bodies[i].acc = double3(0,0,0);
    }
    inline void add_accelerations(int i, int j)
    {

        double3 delta = subtract(bodies[i].pos, bodies[j].pos);

        double distsq = dot_product(delta);

        if (distsq < EPSILON) 
            distsq = EPSILON;

        double dist = sqrt(distsq);

        // compute the unit vector from j to i
        double3 unit_dist = divide(delta, dist);

        double gravity_by_distance = GRAVITY / distsq;

        double ai = gravity_by_distance*bodies[j].mass;
        double aj = gravity_by_distance*bodies[i].mass;

        bodies[i].acc = subtract(bodies[i].acc, multiply(ai,unit_dist));
        bodies[j].acc = add(bodies[j].acc, multiply(aj,unit_dist));

        //add gravity effects
        if(dist < 900)
        {
            bodies[i].vel = divide(bodies[i].vel,3);
            bodies[i].acc = divide(bodies[i].acc,3);
        }
    }
    void nbodies_serial(int n)
    {
        for (int i = 0; i < n - 1; ++i) 
            for (int j = i + 1; j < n; ++j) 
                add_accelerations(i, j);

        for (int i = 0; i < n; ++i) {
            update_body(i);
        }
    }
    void nbodies_parallel(int n)
    {
        parallel_for(0, n-1, [&](int i)
        {
            for (int j = i; j < n; ++j)
                if (i != j) 
                    add_accelerations(i, j);
        });

        parallel_for( 0, n, [&] (int i) {
            update_body(i);
        });
    }
    void interact_rectangle(int i_begin, int i_end, int j_begin, int j_end)
    {
        int delta_i = i_end - i_begin; 
        int dj = j_end - j_begin;

        if (delta_i > GRAINSIZE && dj > GRAINSIZE) 
        {
            int im = i_begin + delta_i/2;
            int jm = j_begin + dj/2;

            parallel_invoke([&]() {interact_rectangle(i_begin, im, j_begin, jm);}, 
                [&]() {interact_rectangle(im, i_end, jm, j_end);});

            parallel_invoke([&]() {interact_rectangle(i_begin, im, jm, j_end);}, 
                [&]() {interact_rectangle(im, i_end, j_begin, jm);});
        }

        else {
            for (int i = i_begin; i < i_end; ++i)
                for (int j = j_begin; j < j_end; ++j)
                    add_accelerations(i, j);
        }
    }
    void body_interact(int i, int j)
    {
        int count = j - i;
        if (count > 1) {
            int mid = count/2 + i;
            parallel_invoke([&]() {body_interact(i,mid);}, 
                [&]() {body_interact(mid,j);});

            interact_rectangle(i, mid, mid, j);
        }

    }

    void nbodies_cache_friendly(int n)
    {
        body_interact(0, n);

        parallel_for( 0, n, [&] (int i) {
            update_body(i);
        });
    }
};