//
//  Timer.hpp
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#ifndef GLAppNative_Timer_hpp
#define GLAppNative_Timer_hpp

#include <sys/time.h>
#include <unistd.h>


/**
 *  A very basic timer class, suitable for FPS counters etc.
 */
class Timer {
    
public:
	Timer() : startTime_(getCurrentTime()) {};
    
	/**
	 * Report the elapsed time in seconds (it will return a double,
	 * so the fractional part is subsecond part).
	 */
	inline double elapsed() const {
		return getCurrentTime() - startTime_;
	};
    
	/**
	 * Report the elapsed time in seconds, and reset the timer.
	 */
	inline double elapsedAndRestart() {
		double now = getCurrentTime();
		double elapsed = now - startTime_;
		startTime_ = now;
		return elapsed;
	};
    
	/**
	 * Restart the timer.
	 */
	inline void restart() {
		startTime_ = getCurrentTime();
	};
    
	/**
	 * Return the current time as number of elapsed seconds of this day.
	 */
	double static getCurrentTime() {
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);
        return tv.tv_sec+tv.tv_usec*1e-6;
    };
    
    
private:
	double startTime_;
};

#endif