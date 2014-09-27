/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of a thread barrier.
 *
 * \author Martin Gronemann
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/


#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_THREAD_BARRIER_H
#define OGDF_THREAD_BARRIER_H

#include <ogdf/basic/basic.h>

#ifdef OGDF_SYSTEM_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace ogdf
{

#ifdef OGDF_SYSTEM_WINDOWS

// if Windows >= Vista, Server 2008
#if (_WIN32_WINNT >= 0x0600)

//---------------------------------------------------------
// Windows Vista (and above) Barrier implementation
//---------------------------------------------------------

class Barrier
{
public:
    inline Barrier(__uint32 numThreads) : m_threadCount(numThreads)
    {
        InitializeConditionVariable(&m_allThreadsReachedSync);
        InitializeCriticalSection(&m_numThreadsReachedSyncLock);
        m_numThreadsReachedSync = 0;
        m_syncNumber = 0;
    }

    ~Barrier() { }

    inline void threadSync()
    {
        EnterCriticalSection(&m_numThreadsReachedSyncLock);
        __uint32 syncNr = m_syncNumber;
        m_numThreadsReachedSync++;
        if(m_numThreadsReachedSync == m_threadCount)
        {
            m_syncNumber++;
            WakeAllConditionVariable(&m_allThreadsReachedSync);
            m_numThreadsReachedSync = 0;
        }
        else
        {
            while(syncNr == m_syncNumber)
            {
                // Sleeping while waiting for the Condition Variable to signal, releases (leaves) the CriticalSection temporarily
                SleepConditionVariableCS(&m_allThreadsReachedSync, &m_numThreadsReachedSyncLock, INFINITE);
            }
            // when awake, whe thread is again in the Critical Section
        }
        LeaveCriticalSection(&m_numThreadsReachedSyncLock);
    }
private:
    __uint32 m_threadCount;
    CRITICAL_SECTION m_numThreadsReachedSyncLock;
    CONDITION_VARIABLE m_allThreadsReachedSync;
    __uint32 m_numThreadsReachedSync;
    __uint32 m_syncNumber;
};

#else //(_WIN32_WINNT >= 0x0600)

//---------------------------------------------------------
// Windows XP (and below) Barrier implementation
//---------------------------------------------------------

//! Representation of a barrier.
/**
 * A barrier is used for synchronizing threads. A barrier for a group of threads means
 * that all threads in the group must have reached the barrier before any of the threads
 * may proceed executing code after the barrier.
 */
class Barrier
{
public:
    inline Barrier(__uint32 numThreads) : m_threadCount(numThreads)
    {
        m_allThreadsReachedSync = CreateEvent(NULL, TRUE, FALSE, NULL);
        InitializeCriticalSection(&m_numThreadsReachedSyncLock);
        m_numThreadsReachedSync = 0;
        m_syncNumber = 0;
    }

    ~Barrier() { }

    inline void threadSync()
    {
        EnterCriticalSection(&m_numThreadsReachedSyncLock);
        __uint32 syncNr = m_syncNumber;
        m_numThreadsReachedSync++;
        if(m_numThreadsReachedSync == m_threadCount)
        {
            SetEvent(m_allThreadsReachedSync);
            m_syncNumber++;
            m_numThreadsReachedSync = 0;
            LeaveCriticalSection(&m_numThreadsReachedSyncLock);
        }
        else
        {
            if((m_syncNumber == syncNr) && (m_numThreadsReachedSync == 1))
            {
                ResetEvent(m_allThreadsReachedSync);
            }
            while(m_syncNumber == syncNr)
            {
                LeaveCriticalSection(&m_numThreadsReachedSyncLock);
                WaitForSingleObject(m_allThreadsReachedSync, 100);
                EnterCriticalSection(&m_numThreadsReachedSyncLock);
            }
            LeaveCriticalSection(&m_numThreadsReachedSyncLock);
        }
    }
private:
    __uint32 m_threadCount;
    CRITICAL_SECTION m_numThreadsReachedSyncLock;
    HANDLE m_allThreadsReachedSync;
    __uint32 m_numThreadsReachedSync;
    __uint32 m_syncNumber;
};

#endif //(_WIN32_WINNT >= 0x0600)
#else //OGDF_SYSTEM_WINDOWS


#ifndef OGDF_PTHREAD_BARRRIER

//---------------------------------------------------------
// pthread implementation without using pthread's barrier
//---------------------------------------------------------

class Barrier
{
public:
    inline Barrier(__uint32 numThreads) : m_threadCount(numThreads)
    {
        pthread_cond_init(&m_allThreadsReachedSync, NULL);
        pthread_mutex_init(&m_numThreadsReachedSyncLock, NULL);
        m_numThreadsReachedSync = 0;
        m_syncNumber = 0;
    }

    ~Barrier()
    {
        pthread_cond_destroy(&m_allThreadsReachedSync);
        pthread_mutex_destroy(&m_numThreadsReachedSyncLock);
    }

    inline void threadSync()
    {
        pthread_mutex_lock(&m_numThreadsReachedSyncLock);
        __uint32 syncNr = m_syncNumber;
        m_numThreadsReachedSync++;
        if(m_numThreadsReachedSync == m_threadCount)
        {
            m_syncNumber++;
            pthread_cond_signal(&m_allThreadsReachedSync);
            m_numThreadsReachedSync = 0;
        }
        else
        {
            while(syncNr == m_syncNumber)
                pthread_cond_wait(&m_allThreadsReachedSync, &m_numThreadsReachedSyncLock);
        }
        pthread_mutex_unlock(&m_numThreadsReachedSyncLock);
    }
private:
    __uint32 m_threadCount;
    pthread_mutex_t m_numThreadsReachedSyncLock;
    pthread_cond_t m_allThreadsReachedSync;
    __uint32 m_numThreadsReachedSync;
    __uint32 m_syncNumber;
};
#else

//---------------------------------------------------------
// pthread barrier implementation
//---------------------------------------------------------

class Barrier
{
public:
    inline Barrier(__uint32 numThreads) : m_threadCount(numThreads)
    {
        pthread_barrier_init(&m_barrier, NULL, m_threadCount);
    }

    ~Barrier()
    {
        pthread_barrier_destroy(&m_barrier);
    }

    inline void threadSync()
    {
        pthread_barrier_wait(&m_barrier);
    }
private:
    pthread_barrier_t m_barrier;
    __uint32 m_threadCount;
};
#endif

#endif //OGDF_SYSTEM_UNIX

} // end of namespace ogdf

#endif //OGDF_THREAD_BARRIER_H
