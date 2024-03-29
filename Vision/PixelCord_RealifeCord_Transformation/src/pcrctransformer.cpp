//******************************************************************************
//
//                 Low Cost Vision
//
//******************************************************************************
// Project:        PixelCord_RealifeCord_Transformation
// File:           pcrctransformer.cpp
// Description:    library for convert reallife coordinates to pixel coordinates and vice versa
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
#include <pcrctransformation/pcrctransformer.hpp>
#include <pcrctransformation/point2f.hpp>
#include <cmath>
#include <vector>
#include <stdexcept>
#include <float.h>
#include <list>
#include <iostream>

namespace pcrctransformation {
	pc_rc_transformer::pc_rc_transformer(const point2f::point2fvector & fiducials_real_cordinates,
			const point2f::point2fvector& fiducials_pixel_cordinates )
		: fiducials_real_coordinates(fiducials_real_cordinates), fiducials_pixel_coordinates(fiducials_pixel_cordinates), scale(0)
	{
		update_transformation_parameters();
	}

	pc_rc_transformer::~pc_rc_transformer()
	{
	}

	void pc_rc_transformer::set_fiducials_pixel_coordinates(const point2f::point2fvector & fiducials_real_cordinates)
	{
		this->fiducials_pixel_coordinates = fiducials_real_cordinates;
		update_transformation_parameters();
	}

	point2f pc_rc_transformer::to_rc(const point2f & pixel_cordinate) const
	{
		for(unsigned int n = 0; n < fiducials_pixel_coordinates.size(); n++)
		{
			if(fiducials_pixel_coordinates[n] == pixel_cordinate){
				return fiducials_real_coordinates[n];
			}
		}
		point2f intersection0, intersection1;
		std::list<point2f> intersections;

		for(unsigned int n = 0; n < fiducials_pixel_coordinates.size(); n++)
		{
			for(unsigned int m = n+1; m < fiducials_pixel_coordinates.size();m++)
			{
				if(!circle_circle_intersection( fiducials_real_coordinates[n],
														pixel_cordinate.distance(fiducials_pixel_coordinates[n])*scale,
														fiducials_real_coordinates[m],
														pixel_cordinate.distance(fiducials_pixel_coordinates[m])*scale,
														intersection0,
														intersection1))
				{
					throw std::runtime_error("Can not find two intersections between the circles");
				}
				intersections.push_back(intersection0);
				intersections.push_back(intersection1);
			}
		}

		point2f result;
		std::list<point2f>::iterator index_intersection0;
		std::list<point2f>::iterator index_intersection1;

		for(unsigned int n= 0; n < fiducials_real_coordinates.size()-1; n++)
		{
			double shortest_distance = DBL_MAX;

			for(std::list<point2f>::iterator i = intersections.begin(); i != intersections.end(); i++)
			{
				std::list<point2f>::iterator j = i;
				++j;
				for(; j != intersections.end(); j++)
				{
					double temp = (*i).distance(*j);
					if(temp < shortest_distance)
					{
						shortest_distance = temp;
						index_intersection0 = i;
						index_intersection1 = j;
					}
				}
			}
			result = (*index_intersection0).mean(*index_intersection1);
			intersections.erase(index_intersection0);
			intersections.erase(index_intersection1);
			intersections.push_back(result);
		}
		return result;
	}

	point2f pc_rc_transformer::to_pc(const point2f & real_cordinate) const
	{
		for(unsigned int n = 0; n < fiducials_real_coordinates.size(); n++)
		{
			if(fiducials_real_coordinates[n] == real_cordinate)
			{
				return fiducials_pixel_coordinates[n];
			}
		}
		point2f intersection0, intersection1;
		std::list<point2f> intersections;

		for(unsigned int n = 0; n < fiducials_real_coordinates.size(); n++)
		{
			for(unsigned int m = n+1; m < fiducials_real_coordinates.size(); m++)
			{
				if(!circle_circle_intersection( fiducials_pixel_coordinates[n],
														real_cordinate.distance(fiducials_real_coordinates[n])/scale,
														fiducials_pixel_coordinates[m],
														real_cordinate.distance(fiducials_real_coordinates[m])/scale,
														intersection0,
														intersection1))
				{
					throw std::runtime_error("Can not find two intersections between the circles");
				}
				intersections.push_back(intersection0);
				intersections.push_back(intersection1);
			}
		}

		point2f result;
		std::list<point2f>::iterator index_intersection0;
		std::list<point2f>::iterator index_intersection1;

		for(unsigned int n= 0; n < fiducials_pixel_coordinates.size()-1; n++)
		{
			double shortest_distance = DBL_MAX;

			for(std::list<point2f>::iterator i = intersections.begin(); i != intersections.end(); i++)
			{
				std::list<point2f>::iterator j = i;
				++j;
				for(; j != intersections.end(); j++)
				{
					double temp = (*i).distance(*j);
					if(temp < shortest_distance)
					{
						shortest_distance = temp;
						index_intersection0 = i;
						index_intersection1 = j;
					}
				}
			}
			result = (*index_intersection0).mean(*index_intersection1);
			intersections.erase(index_intersection0);
			intersections.erase(index_intersection1);
			intersections.push_back(result);
		}
		return result;
	}

	void pc_rc_transformer::update_transformation_parameters()
	{
		if(fiducials_real_coordinates.size() != fiducials_pixel_coordinates.size())
			throw std::runtime_error("Number of real fiducial coordinates does not match number of pixel fiducials coordinates");
		scale = 0;
		int distances_count = 0;
		for(unsigned int n = 0 ; n < fiducials_real_coordinates.size();n++)
		{
			for(unsigned int m = n+1 ; m < fiducials_real_coordinates.size();m++)
			{
				scale += fiducials_real_coordinates[n].distance(fiducials_real_coordinates[m]) / fiducials_pixel_coordinates[n].distance(fiducials_pixel_coordinates[m]);
				distances_count++;
			}
		}
		scale /= distances_count;
	}

	bool pc_rc_transformer::circle_circle_intersection(point2f point0, double r0,
	                               point2f point1, double r1,
	                               point2f &intersection0,
	                               point2f &intersection1 ) const
	{
		double x0 = point0.x;
		double x1 = point1.x;
		double y0 = point0.y;
		double y1 = point1.y;
		double a, dx, dy, d, h, rx, ry;
		double x2, y2;

		/* dx and dy are the vertical and horizontal distances between
		* the circle centers.
		*/
		dx = x1 - x0;
		dy = y1 - y0;

		/* Determine the straight-line distance between the centers. */
		//d = sqrt((dy*dy) + (dx*dx));
		d = hypot(dx,dy); // Suggested by Keith Briggs

		/* Check for solvability. */
		if ((d > (r0 + r1)) || (d < fabs(r0 - r1)))
		{
			double ratio  = d/((d - r0 - r1) / 2 + r0) ;
			intersection0.x = x0+(dx/ratio);
			intersection1.x = x0+(dx/ratio);
			intersection0.y = y0+(dy/ratio);
			intersection1.y = y0+(dy/ratio);

			//std::cout<< "circles do not intersect, calculating the point that has the same distance to both circles" << std::endl;
			return true;
		}

		/* 'point 2' is the point where the line through the circle
		* intersection points crosses the line between the circle
		* centers.
		*/

		/* Determine the distance from point 0 to point 2. */
		a = ((r0*r0) - (r1*r1) + (d*d)) / (2.0 * d) ;

		/* Determine the coordinates of point 2. */
		x2 = x0 + (dx * a/d);
		y2 = y0 + (dy * a/d);

		/* Determine the distance from point 2 to either of the
		* intersection points.
		*/
		h = sqrt((r0*r0) - (a*a));

		/* Now determine the offsets of the intersection points from
		* point 2.
		*/
		rx = -dy * (h/d);
		ry = dx * (h/d);

		/* Determine the absolute intersection points. */
		intersection0.x = x2 + rx;
		intersection1.x = x2 - rx;
		intersection0.y = y2 + ry;
		intersection1.y = y2 - ry;

		return true;
	}
}
