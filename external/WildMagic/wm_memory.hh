
// Geometric Tools, LLC
// Copyright (c) 1998-2014
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 5.0.1 (2010/10/01)


#pragma once

namespace WM
{

// For 1D arrays:  data[bound0]
template <typename T>
T* new1 (const size_t bound0);

// For 2D arrays:  data[bound1][bound0]
template <typename T>
T** new2 (const size_t bound0, const size_t bound1);

// For 3D arrays:  data[bound2][bound1][bound0]
template <typename T>
T*** new3 (const size_t bound0, const size_t bound1, const size_t bound2);

// For 4D arrays:  data[bound3][bound2][bound1][bound0]
template <typename T>
T**** new4 (const size_t bound0, const size_t bound1, const size_t bound2,
    const size_t bound3);

// For singletons.
template <typename T>
void delete0 (T*& data);

// For 1D arrays:  data[bound0]
template <typename T>
void delete1 (T*& data);

// For 2D arrays:  data[bound1][bound0]
template <typename T>
void delete2 (T**& data);

// For 3D arrays:  data[bound2][bound1][bound0]
template <typename T>
void delete3 (T***& data);

// For 4D arrays:  data[bound3][bound2][bound1][bound0]
template <typename T>
void delete4 (T****& data);

template <typename T>
T* new1 (const size_t bound0)
{
    return new T[bound0];
}
//----------------------------------------------------------------------------
template <typename T>
T** new2 (const size_t bound0, const size_t bound1)
{
    const size_t bound01 = bound0*bound1;
    T** data = new T*[bound1];
    data[0] = new T[bound01];

    for (size_t i1 = 1; i1 < bound1; ++i1)
    {
        size_t j0 = bound0*i1;  // = bound0*(i1 + j1) where j1 = 0
        data[i1] = &data[0][j0];
    }
    return data;
}
//----------------------------------------------------------------------------
template <typename T>
T*** new3 (const size_t bound0, const size_t bound1, const size_t bound2)
{
    const size_t bound12 = bound1*bound2;
    const size_t bound012 = bound0*bound12;
    T*** data = new T**[bound2];
    data[0] = new T*[bound12];
    data[0][0] = new T[bound012];

    for (size_t i2 = 0; i2 < bound2; ++i2)
    {
        size_t j1 = bound1*i2;  // = bound1*(i2 + j2) where j2 = 0
        data[i2] = &data[0][j1];
        for (size_t i1 = 0; i1 < bound1; ++i1)
        {
            size_t j0 = bound0*(i1 + j1);
            data[i2][i1] = &data[0][0][j0];
        }
    }
    return data;
}
//----------------------------------------------------------------------------
template <typename T>
T**** new4 (const size_t bound0, const size_t bound1, const size_t bound2,
    const size_t bound3)
{
    const size_t bound23 = bound2*bound3;
    const size_t bound123 = bound1*bound23;
    const size_t bound0123 = bound0*bound123;
    T**** data = new T***[bound3];
    data[0] = new T**[bound23];
    data[0][0] = new T*[bound123];
    data[0][0][0] = new T[bound0123];

    for (size_t i3 = 0; i3 < bound3; ++i3)
    {
        size_t j2 = bound2*i3;  // = bound2*(i3 + j3) where j3 = 0
        data[i3] = &data[0][j2];
        for (size_t i2 = 0; i2 < bound2; ++i2)
        {
            size_t j1 = bound1*(i2 + j2);
            data[i3][i2] = &data[0][0][j1];
            for (size_t i1 = 0; i1 < bound1; ++i1)
            {
                size_t j0 = bound0*(i1 + j1);
                data[i3][i2][i1] = &data[0][0][0][j0];
            }
        }
    }
    return data;
}
//----------------------------------------------------------------------------
template <typename T>
void delete0 (T*& data)
{
    delete data;
    data = 0;
}
//----------------------------------------------------------------------------
template <typename T>
void delete1 (T*& data)
{
    delete[] data;
    data = 0;
}
//----------------------------------------------------------------------------
template <typename T>
void delete2 (T**& data)
{
    if (data)
    {
        delete[] data[0];
        delete[] data;
        data = 0;
    }
}
//----------------------------------------------------------------------------
template <typename T>
void delete3 (T***& data)
{
    if (data)
    {
        delete[] data[0][0];
        delete[] data[0];
        delete[] data;
        data = 0;
    }
}
//----------------------------------------------------------------------------
template <typename T>
void delete4 (T****& data)
{
    if (data)
    {
        delete[] data[0][0][0];
        delete[] data[0][0];
        delete[] data[0];
        delete[] data;
        data = 0;
    }
}

}
