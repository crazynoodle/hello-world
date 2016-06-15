#include "timer.h"

Timer::Timer() : m_start(),
    m_finish(),m_duration(-1.0),
    m_is_start(false), m_is_finish(false)
{
    
}


bool Timer::startTick()
{
    assert(!m_is_start);
    if (m_is_start == false) {
        m_is_start = true;
        m_is_finish = false;
        gettimeofday(&m_start, NULL);
        return true;
    }
    
    return false;
}

bool Timer::finishTick()
{
    assert((m_is_start) && (!m_is_finish));
    if (m_is_start && (!m_is_finish)) {
        m_is_finish = true;
        gettimeofday(&m_finish, NULL);
        m_duration = m_finish.tv_sec-m_start.tv_sec+ (double)(m_finish.tv_usec - m_start.tv_usec)/1000.0;
        return true;
    }
    
    return false;
}


double Timer::getDuration()
{
    return m_duration;
}