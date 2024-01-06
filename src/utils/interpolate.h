//-----------------------------------------------------------------------------
// (Bi)Linear interpolation in 2D and 3D 
//-----------------------------------------------------------------------------
//
// Copyright (C) 2018 Arend Lammertink
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------
// 
// Based on:
//
// Interpolation in Two or More Dimensions
// http://www.aip.de/groups/soe/local/numres/bookcpdf/c3-6.pdf
//
// How to build a lookup table in C (SDCC compiler) with linear interpolation
// http://bit.ly/LUT_c_linear_interpolation
//
// Linear interpolation: calculate correction based on 2D table
// http://bit.ly/Interpolate2D
//
// 2D Array Interpolation
// http://bit.ly/biliniar_barycentric_interpolation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Multi-include protection
//-----------------------------------------------------------------------------

#ifndef _INTERPOL_H
#define _INTERPOL_H

//-----------------------------------------------------------------------------
// TODO :
//
//   fix16.h includes a number of lerp (linear interpolation) routines. These
//   could perhaps be used to speed things up.
//
//   There are no routines for casting uint16_t integers to/from Fix16. For
//   now, we cast to float before casting to Fix16. 
//
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// 1D linear interpolation
//-----------------------------------------------------------------------------

template <typename X, typename Y>
inline Y interpolate(X x, X x_1, X x_2, Y y_1, Y y_2) 
{
    float one = 1.0f; // avoid ambiguous operator overload

    // cast indexes to Y
    Y _x = static_cast<Y>(x);
    Y _x_1 = static_cast<Y>(x_1);
    Y _x_2 = static_cast<Y>(x_2);

    float dx = (static_cast<float>(_x) - static_cast<float>(_x_1)) / (static_cast<float>(_x_2) - static_cast<float>(_x_1));
     
    // Cast the return value to type Y
    return static_cast<Y>((one - dx) * y_1 + dx * y_2);
}



//-----------------------------------------------------------------------------
// 2D bilinear interpolation
//-----------------------------------------------------------------------------

template <typename X, typename Y>
inline Y interpolate(X x1, X x2, X x_1, X x_2, X x_3, X x_4,
                     Y y_1, Y y_2, Y y_3, Y y_4)
{
    float one = 1.0f; // avoid ambiguous operator overload

    // cast indexes to Y
    Y _x1 = static_cast<Y>(x1);
    Y _x2 = static_cast<Y>(x2);
    Y _x_1 = static_cast<Y>(x_1);
    Y _x_2 = static_cast<Y>(x_2);
    Y _x_3 = static_cast<Y>(x_3);
    Y _x_4 = static_cast<Y>(x_4);

    float dx1 = (static_cast<float>(_x1) - static_cast<float>(_x_1)) / (static_cast<float>(_x_2) - static_cast<float>(_x_1));
    float dx2 = (static_cast<float>(_x2) - static_cast<float>(_x_3)) / (static_cast<float>(_x_4) - static_cast<float>(_x_3));

    // Cast the return value to type Y
    return static_cast<Y>((one - dx1) * (one - dx2) * y_1 +
                          dx1 * (one - dx2) * y_2 +
                          dx1 * dx2 * y_3 +
                          (one - dx1) * dx2 * y_4);
}




#endif // End multi-include protection