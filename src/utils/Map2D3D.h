//-----------------------------------------------------------------------------
// 2D and 3D Maps c.q. lookup tables with (bi)linear interpolation
//-----------------------------------------------------------------------------
//
// Copyright (C) 2018 Arend Lammertink
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, version 3.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library. If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------
//
// Description:
//
// Maps or lookup tables (LUTs) are used to approximate a multidimensional
// function y = f(X), with y a single dimensional value and X an n dimensional
// vector X = (x1, x2, ... xn).
//
// For the one dimensional case, two arrays are used to store a number of
// known values for y = f(x), as follows::
//
//   ys[i] = f( xs[i] )
//
// For the two dimensional case, three arrays are used to store a number of
// known values for y = f(X) = f(x1,x2), as follows::
//
//   ys[i][j] = f( x1s[i], x2s[j] )
//
// In order to approximate f(X) at any point within the range of stored
// values, we search the xs array(s) to find the nearest known X-es and
// lineary interpolate the known result stored in the ys array.
//
//-----------------------------------------------------------------------------
//
// Based on:
//
// Set array size at compile time thru constructor
// http://www.cplusplus.com/forum/general/114406/
//
// How to Search an Ordered Table:
// http://www.aip.de/groups/soe/local/numres/bookcpdf/c3-4.pdf
//
// Putting constant data into program memory (PROGMEM)
// http://www.gammon.com.au/progmem
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Multi-include protection
//-----------------------------------------------------------------------------

#ifndef _LOOKUP_TABLE_2D3D
#define _LOOKUP_TABLE_2D3D

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "utils/interpolate.h"

//-----------------------------------------------------------------------------
// Abstract base class
//-----------------------------------------------------------------------------

class Map
{
public:
    virtual int memSize() const = 0;

protected:
    uint16_t curOffset;
};

//-----------------------------------------------------------------------------
// 2D lookup table / fuel map. X axis (xs) must be sorted in ascending order.
//-----------------------------------------------------------------------------

template <int S, typename X, typename Y> // S: size, X,Y: data types
class Map2D : public Map
{
public:
    Map2D()
    {
        for (int i = 0; i < S; i++)
        {
            xs[i] = 0;
            ys[i] = 0;
        }
    }

    int xSize() const { return S; }
    int ySize() const { return S; }
    int memSize() const { return S * (sizeof(X) + sizeof(Y)); }

    void setXs(const X *xss) { memcpy(xs, xss, S * sizeof(X)); }

    int getXInt(int i) { return (i >= 0 && i < S) ? static_cast<int>(xs[i]) : 0; }

    float getXFloat(int i) { return (i >= 0 && i < S) ? static_cast<float>(xs[i]) : 0; }

    void setYs(const Y *yss) { memcpy(ys, yss, S * sizeof(Y)); }

    int getYInt(int i) { return (i >= 0 && i < S) ? static_cast<int>(ys[i]) : 0; }

    float getYFloat(int i) { return (i >= 0 && i < S) ? static_cast<float>(ys[i]) : 0; }

#ifdef ARDUINO // Initialization from array in PROGMEM

    void setXs_P(const X *xss) { memcpy_P(xs, xss, S * sizeof(X)); }

    void setYs_P(const Y *yss) { memcpy_P(ys, yss, S * sizeof(Y)); }

#endif

    Y f(X x) // approximate f(x)
    {
        if (x < xs[0])
        {
            return ys[0];
        } // minimum
        if (x > xs[S - 1])
        {
            return ys[S - 1];
        } // maximum

        int i = 0, j = S - 1, k;

        // find i, such that xs[i] <= x < xs[j] using bisection
        while (j - i > 1)
        {
            k = (i + j) >> 1; // k = (i+j)/2

            if (x >= xs[k])
                i = k;
            else
                j = k;
        }


        return interpolate(x, xs[i], xs[i + 1], ys[i], ys[i + 1]);
    }

protected:
    X xs[S];
    Y ys[S];
};

//-----------------------------------------------------------------------------
// 3D lookup table / fuel map. X axis must be sorted in ascending order.
//-----------------------------------------------------------------------------

template <int R, int C, typename X, typename Y> // R,C: size, X,Y: data type
class Map3D : public Map
{
public:
    Map3D()
    {
        for (int i = 0; i < R; i++)
        {
            x1s[i] = 0;
        }
        for (int i = 0; i < C; i++)
        {
            x2s[i] = 0;
        }
        for (int i = 0; i < R * C; i++)
        {
            ys[i] = 0;
        }
    }

    int x1Size() const { return R; }
    int x2Size() const { return C; }
    int ySize() const { return R * C; }
    int memSize() const { return (R + C) * sizeof(X) + (R * C) * sizeof(Y); }

    void setX1s(const X *x1ss)
    {
        memcpy(x1s, x1ss, R * sizeof(X));
    }

    void setX2s(const X *x2ss)
    {
        memcpy(x2s, x2ss, C * sizeof(X));
    }

    int getX1Int(int i) { return (i >= 0 && i < R) ? static_cast<int>(x1s[i]) : 0; }
    int getX2Int(int i) { return (i >= 0 && i < C) ? static_cast<int>(x2s[i]) : 0; }

    float getX1Float(int i) { return (i >= 0 && i < R) ? static_cast<float>(x1s[i]) : 0; }
    float getX2Float(int i) { return (i >= 0 && i < C) ? static_cast<float>(x2s[i]) : 0; }

    void setYs(const Y *yss)
    {
        memcpy(ys, yss, R * C * sizeof(Y));
    }

    int getYInt(int i, int j)
    {
        return (i >= 0 && i < R && j >= 0 && j < C) ? static_cast<int>(ys[i][j]) : 0;
    }

    float getYFloat(int i, int j)
    {
        return (i >= 0 && i < R && j >= 0 && j < C) ? static_cast<float>(ys[i][j]) : 0;
    }

#ifdef ARDUINO // Initialize from array in PROGMEM

    void setX1s_P(const X *x1ss) { memcpy_P(x1s, x1ss, R * sizeof(X)); }
    void setX2s_P(const X *x2ss) { memcpy_P(x2s, x2ss, C * sizeof(X)); }

    void setYs_P(const Y *yss) { memcpy_P(ys, yss, R * C * sizeof(Y)); }

#endif

    Y f(X x1, X x2)
    {
        if (x1 < x1s[0])
        {
            x1 = x1s[0];
        } // minimum
        if (x1 > x1s[R - 1])
        {
            x1 = x1s[R - 1];
        } // maximum
        if (x2 < x2s[0])
        {
            x2 = x2s[0];
        } // minimum
        if (x2 > x2s[C - 1])
        {
            x2 = x2s[C - 1];
        } // maximum

        int i = 0, j = R - 1, m;

        // find i, such that x1s[i] <= x1 < x1s[j] using bisection
        while (j - i > 1)
        {
            m = (i + j) >> 1; // m = (i+j)/2

            if (x1 >= x1s[m])
                i = m;
            else
                j = m;
        }

        int k = C - 1;
        j = 0;

        // find j, such that x2s[j] <= x2 < x2s[k] using bisection
        while (k - j > 1)
        {
            m = (j + k) >> 1; // m = (j+k)/2

            if (x2 >= x2s[m])
                j = m;
            else
                k = m;
        }

        return interpolate(x1, x2,
                           x1s[i], x1s[i + 1], x2s[j], x2s[j + 1],
                           ys[i][j], ys[i + 1][j], ys[i + 1][j + 1], ys[i][j + 1]);
    }

protected:
    X x1s[R];
    X x2s[C];
    Y ys[R][C];
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#endif // End multi-include protection