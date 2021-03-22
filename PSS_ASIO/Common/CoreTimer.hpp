#pragma once

#include "define.h"
#include "TimerManager.hpp"
#include "singleton.h"

class PSS_Timer_Manager
{
public:
    void Start()
    {
        m_timerMgr = std::make_shared<brynet::TimerMgr>();

        m_ttTimerThread = std::thread([this]()
            {
                m_timerMgr->schedule();
                PSS_LOGGER_DEBUG("[PSS_Timer_Manager::start]End.");
            });
    };

    void Close()
    {
        if (nullptr != m_timerMgr)
        {
            m_timerMgr->Close();
            m_ttTimerThread.join();
        }
    };

    brynet::TimerMgr::Ptr GetTimerPtr() const
    {
        return m_timerMgr;
    };

private:
    brynet::TimerMgr::Ptr m_timerMgr;
    std::thread           m_ttTimerThread;
};

using App_TimerManager = PSS_singleton<PSS_Timer_Manager>;
