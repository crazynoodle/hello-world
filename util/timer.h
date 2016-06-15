/*!
 * \file timer.h
 * \brief 系统计时类
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.11.30
 */
#ifndef __DCR__timer__
#define __DCR__timer__

#include <sys/time.h>
#include <cassert>
#include <cstdlib>

class Timer {
public:
    Timer();
    
    bool startTick();
    
    bool finishTick();
    
    double getDuration();
    
private:
    struct timeval m_start;
    struct timeval m_finish;
    double m_duration;
    bool m_is_start;
    bool m_is_finish;
};

#endif 