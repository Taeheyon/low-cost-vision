//******************************************************************************
//
//                 Low Cost Vision
//
//******************************************************************************
// Project:        huniplacer
// File:           utils.h
// Description:    miscellaneous utilities
// Author:         Lukas Vermond & Kasper van Nieuwland
// Notes:          -
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

#include <boost/thread.hpp>
#include <cstdio>
#include <algorithm>
#include <vector>

namespace huniplacer
{
	/**
	 * @brief various utilities
	 **/
    namespace utils
    {
    	/**
    	 * @brief get the current time in milliseconds
    	 * @return time in milliseconds
    	 **/
        long time_now(void);

        /**
         * @brief sleep for X milliseconds
         * @param ms time in milliseconds
         **/
        void sleep(long ms);

        /**
         * @brief convert radians to degrees
         * @param rad radians
         * @return degrees
         **/
        double deg(double rad);

        /**
         * @brief convert degrees to radians
         * @param deg degrees
         * @return radians
         **/
        double rad(double deg);

        /**
         * @brief utility class to time stuff
         * @note TEMPORARY
         **/
        class stopwatch
        {
			private:
        		const char* name;
				long t0, t1;

			public:
				stopwatch(const char* name, bool s = false) : name(name)
				{
					if(s) { start(); }
				}

				~stopwatch(void) { }
				void start(void) { t0 = time_now(); }
				void stop(void) { t1 = time_now(); }
				void print(FILE* stream)
				{
					fprintf(stream, "%s: %ld ms\n", name, t1 - t0);
				}

				void stop_and_print(FILE* stream)
				{
					stop();
					print(stream);
				}
        };

        template<typename T>
        bool vector_contains(const std::vector<T>& vec, const T& elem)
        {
        	return std::find(vec.begin(), vec.end(), elem) != vec.end();
        }
    }
}
