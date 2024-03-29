//******************************************************************************
//
//                 Low Cost Vision
//
//******************************************************************************
// Project:        PixelCord_RealifeCord_Transformation
// File:           point2f.cpp
// Description:    A object used for storing a coordinated as 2 floats
// Author:         Kasper van Nieuwland & Zep Mouris
// Notes:          ...
//
// License: newBSD 
//  
// Copyright © 2012, HU University of Applied Sciences Utrecht. 
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// - Neither the name of the HU University of Applied Sciences Utrecht nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE HU UNIVERSITY OF APPLIED SCIENCES UTRECHT
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//******************************************************************************
#pragma once

#include <cmath>
#include <vector>

namespace pcrctransformation {
	/**
	* class that represents a 2 dimensional point.
	* Has utilities to transform the point.
	*/
	class point2f {
		public:
			/**
			 * contains the x value of the point
			 */
			double x;
			/**
			 * contains the y value of the point
			 */
			double y;

			/**
			 * constructor to make a point to 0,0
			 */
			point2f() : x(0), y(0) { }
			/**
			 * constructor to make a point at a specific location
			 * @param x the x value of the point
			 * @param y the y value of the point
			 */
			point2f(double x, double y) : x(x), y(y) { }
			/**
			 * destructor
			 */
			~point2f() { }

			inline bool operator==(const point2f& rhs) const
			{
				return this->x == rhs.x && this->y == rhs.y;
			}

            inline point2f& operator+=(const point2f& rhs)
            {
                x += rhs.x;
                y += rhs.y;
                return *this;
            }

            inline point2f& operator-=(const point2f& rhs)
            {
                x -= rhs.x;
                y -= rhs.y;
                return *this;
            }

            inline point2f& operator*=(const point2f& rhs)
            {
                x *= rhs.x;
                y *= rhs.y;
                return *this;
            }

            inline point2f operator+(const point2f& rhs) const
            {
                point2f res = *this;
                res += rhs;
                return res;
            }

            inline point2f operator-(const point2f& rhs) const
            {
                point2f res = *this;
                res -= rhs;
                return res;
            }

            inline point2f operator*(const point2f& rhs) const
            {
                point2f res = *this;
                res *= rhs;
                return res;
            }
            /**
             * calculates the distance between two points
             * @param p the second point
             * @return the distance between the points
             */
            inline double distance(const point2f& p) const
            {
                double dx = x - p.x;
                double dy = y - p.y;
                return sqrt(dx*dx + dy*dy);
            }
            /**
             * Calculate the center of two points
             * @param p the second point
             * @return returns a point2f with the location of the center
             */
            inline point2f mean(const point2f& p) const
			{
            	point2f result;
            	result = *this+p;
            	result.x /= 2;
            	result.y /= 2;
				return result;
			}
            /**
             * rotates the point around 0,0
             * @param angle the angle to rotate (radians)
             * @return returns a point2f with the new location
             */
            inline point2f rotate(double angle) const
            {
            	return point2f(x*cos(angle) - y*sin(angle), x*sin(angle) + y*cos(angle));
            }
            /**
             * a vector of point2f's
             */
            typedef std::vector<point2f> point2fvector;
	};
}
