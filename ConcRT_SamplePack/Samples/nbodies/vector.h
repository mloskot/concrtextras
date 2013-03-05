// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: vector.h
//
//
//--------------------------------------------------------------------------
#pragma once
namespace ray_lib
{
    template <class vec>
    inline float dot_product(vec& v1, vec& v2)
    {
        return (v1.first() * v2.first()) + (v1.second() * v2.second()) + (v1.third() * v2.third());
    }
    template <class vec>
    inline float dot_product(vec& v1)
    {
        return (float)((v1.first() * v1.first()) + (v1.second() * v1.second()) + (v1.third() * v1.third()));
    }
    template <class vec>
    inline vec add(vec& v1, vec& v2)
    {
        return vec(v1.first() + v2.first(), v1.second() + v2.second(), v1.third() + v2.third());
    }
    template <class vec>
    inline vec subtract(vec& v1, vec& v2)
    {
        return vec(v1.first() - v2.first(), v1.second() - v2.second(), v1.third() - v2.third());
    }
    template <class vec>
    inline vec multiply1(vec& v1, vec& v2)
    {
        return vec(v1.first() * v2.first(), v1.second() * v2.second(), v1.third() * v2.third());
    }
    template <class vec, class scalar>
    inline vec divide(vec& v, scalar n)
    {
        return vec(v.first() / n, v.second() / n, v.third() / n);
    }
    template <class vec, class scalar>
    inline vec multiply(scalar n, vec& v)
    {
        return vec(v.first() * n, v.second() * n, v.third() * n);
    }
    template <class vec>
    inline float magnify(vec& v)
    {
        return sqrt(dot_product(v, v)); 
    }
    template <class vec>
    inline vec normal(vec& v)
    {
        float mag = magnify(v);
        float div = mag == 0 ? FLT_MAX : 1 / mag;
        return multiply(div, v);
    }
    template <class vec>
    inline vec cross_product(vec& v1, vec& v2)
    {
        return vec(((v1.second() * v2.third()) - (v1.third() * v2.second())),
            ((v1.third() * v2.first()) - (v1.first() * v2.third())),
            ((v1.first() * v2.second()) - (v1.second() * v2.first())));
    }
    template <class intersection>
    static bool is_null(const intersection& sect) 
    {
        return sect.is_null; 
    }

    template <class type>
    struct vec3
    {
        typedef type vec_type;
        vec_type x;
        vec_type y;
        vec_type z;

        vec3(){}
        inline vec3(vec_type x_, vec_type y_, vec_type z_):x(x_),y(y_),z(z_)
        { 
        }
        const vec3 operator=(const vec3& other){
            if(this!=&other){
                x = other.x;
                y = other.y;
                z = other.z;
            }
            return *this;
        }
        inline vec_type first()
        {
            return x;
        };
        inline vec_type second()
        {
            return y;
        }
        inline vec_type third()
        {
            return z;
        }
        inline double& operator[](int index)
        {
            switch(index)
            {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                return x;
            }
        }
    };
    template<typename type>
    vec3<type> operator-( vec3<type>& left,  vec3<type>& right)
    {
        return subtract(left,right);
    }
    template<typename type>
    vec3<type> operator+( vec3<type>& left,  vec3<type>& right)
    {
        return add(left,right);
    }
    template<typename type>
    vec3<type> operator+=( vec3<type>& left,  vec3<type>& right)
    {
        return left = add(left,right);
    }
    template<typename type, typename scalar>
    vec3<type> operator*( scalar& left,  vec3<type>& right)
    {
        return multiply(left, right);
    }
    template<typename type>
    vec3<type> operator*( vec3<type>& left,  vec3<type>& right)
    {
        return multiply(left, right);
    }
    typedef vec3<float> float3;
    typedef vec3<double> double3;
}