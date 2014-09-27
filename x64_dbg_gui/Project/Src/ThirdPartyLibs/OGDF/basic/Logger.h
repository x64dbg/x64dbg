/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Contains logging functionality
 *
 * \author Markus Chimani
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


#ifndef OGDF_LOGGER_H
#define OGDF_LOGGER_H

#include <ogdf/basic/basic.h>

namespace ogdf
{

//! Centralized global and local logging facility working on streams like cout.
/**
 * The Logger class is a centralized logging environment with 2x2 different use-cases working together.
 * All generated output is sent into the \a world-stream, i.e., \a cout, if not set otherwise.
 *
 * \b Logging \b vs. \b Statistic:
 * The Logger differentiates between \a logging and \a statistic mode.
 * When in logging mode, only the output written via the lout()/slout() commands is
 * written to the world stream (according to loglevels). When in statistic mode,
 * only the output of the sout()/ssout() commands is written.
 * (Sidenote: there is also a \a forced output fout()/sfout() which is written independent on the current mode).
 *
 * The idea of these two modi is that one can augment the code with output which is
 * interesting in the normal computation mode via lout, but the same algorithm can also be run given tabular
 * statistic-lines when e.g. batch-testing the algorithm on a set of benchmark instances.
 *
 * \b Global \b vs. \b Local:
 * You can choose to use the Logging facilities globally via the static outputs (slout(), ssout(), sfout()).
 * Thereby the global log-level and statistic-mode settings are applied.
 * Alternatively you can create your own Logging object with its own parameters only for your algorithm,
 * and use the object methods lout(), sout(), and fout(). This allows you to turn output on for your own
 * (new) algorithm, but keep the rest of the library silent.
 *
 * \b Global \b Settings:
 * The slout command takes an (optional) parameter given the importance of the output (aka. loglevel).
 * The output is written only if the globalLogLevel is not higher. The method globalStatisticMode
 * turn the statistic-mode on and off (thereby disabling or enabling the logging mode).
 *
 * Furthermore, we have a globalMinimumLogLevel. This is used to globally forbid any output
 * with too low importance written by any Logger-objects.
 *
 * \b Local \b Settings:
 * A Logger-object has its own set of settings, i.e., its own localLogLevel and an own localLogMode,
 * which can be any of the following:
 *   - \a LM_LOG: the object is in logging mode, using its own localLogLevel
 *   - \a LM_STATISTIC: the object is in statistic mode
 *   - \a LM_GLOBAL: the object is in the same mode as the static Logger-class (i.e., global settings)
 *   - \a LM_GLOBALLOG: the object is in logging mode, but uses the globalLogLevel
 *
 * \b Typical \b Usage:<br>
 * The simplest but restricted and verbose usage is to write <br>
 * <code><br>
 *    Logger::slout() << "1+2=" << (1+2) << endl;
 * </code>
 *
 * The conceptually easiest and cleanest approach is to augment your algorithm class with a Logger.
 * Multiple inheritance allows this pretty straightforwardly:<br>
 * <code><br>
 *    class MyAlgorithm : public MyBaseClass, protected Logger {<br>
 *    &nbsp;&nbsp;int myMethod();<br>
 *    }<br>
 *    <br>
 *    MyAlgorithm::myMethod() {<br>
 *    &nbsp;&nbsp;lout() << "1+2=" << (1+2) << endl;<br>
 *    }<br>
 * </code>
 *
 */

class OGDF_EXPORT Logger
{

public:
    //! supported log-levels from lowest to highest importance
    enum Level { LL_MINOR, LL_MEDIUM, LL_DEFAULT, LL_HIGH, LL_ALARM, LL_FORCE };
    //! (local) log-modes (see class description)
    enum LogMode { LM_GLOBAL, LM_GLOBALLOG, LM_LOG, LM_STATISTIC };

    // CONSTRUCTORS //////////////////////////////////////
    //! creates a new Logger-object with LM_GLOBAL and local log-level equal globalLogLevel
    Logger() :
        m_loglevel(m_globalloglevel), m_logmode(LM_GLOBAL) {}
    //! creates a new Logger-object with given log-mode and local log-level equal globalLogLevel
    Logger(LogMode m) :
        m_loglevel(m_globalloglevel), m_logmode(m) {}
    //! creates a new Logger-object with LM_GLOBAL and given local log-level
    Logger(Level l) :
        m_loglevel(l), m_logmode(LM_GLOBAL) {}
    //! creates a new Logger-object with given log-mode and given local log-level
    Logger(LogMode m, Level l) :
        m_loglevel(l), m_logmode(m) {}

    // USAGE //////////////////////////////////////
    //! stream for logging-output (local)
    std::ostream & lout(Level l = LL_DEFAULT) const
    {
        return ((!m_globalstatisticmode && m_logmode == LM_GLOBAL) || m_logmode == LM_GLOBALLOG)
               ? ((l >= m_globalloglevel) ? *world : nirvana)
               : ((m_logmode == LM_LOG && l >= m_loglevel && l >= m_minimumloglevel) ? *world : nirvana);
    }
    //! stream for statistic-output (local)
    std::ostream & sout() const
    {
        return ((m_globalstatisticmode && m_logmode == LM_GLOBAL) || (m_logmode == LM_STATISTIC)) ? *world : nirvana;
    }
    //! stream for forced output (local)
    std::ostream & fout() const
    {
        return sfout();
    }

    // STATIC USAGE ///////////////////////////////
    //! stream for logging-output (global)
    static std::ostream & slout(Level l = LL_DEFAULT)
    {
        return ((!m_globalstatisticmode) && l >= m_globalloglevel) ? *world : nirvana;
    }
    //! stream for statistic-output (global)
    static std::ostream & ssout()
    {
        return (m_globalstatisticmode) ? *world : nirvana;
    }
    //! stream for forced output (global)
    static std::ostream & sfout()
    {
        return *world;
    }

    // LOCAL //////////////////////////////////////
    //! gives the local log-level
    Level localLogLevel() const
    {
        return m_loglevel;
    }
    //! sets the local log-level
    void localLogLevel(Level l)
    {
        m_loglevel = l;
    }
    //! gives the local log-mode
    LogMode localLogMode() const
    {
        return m_logmode;
    }
    //! sets the local log-mode
    void localLogMode(LogMode m)
    {
        m_logmode = m;
    }

    // GLOBAL //////////////////////////////////////
    //! gives the local log-level
    static Level globalLogLevel()
    {
        return m_globalloglevel;
    }
    //! sets the global log-level
    static void globalLogLevel(Level l)
    {
        if(l >= m_minimumloglevel) m_globalloglevel = l;
    }

    //! gives the globally minimally required log-level
    static Level globalMinimumLogLevel()
    {
        return m_minimumloglevel;
    }
    //! sets the globally minimally required log-level
    static void globalMinimumLogLevel(Level l)
    {
        if(l <= m_globalloglevel) m_minimumloglevel = l;
    }

    //! returns true if we are globally in statistic mode
    static bool globalStatisticMode()
    {
        return m_globalstatisticmode;
    }
    //! sets whether we are globally in statistic mode
    static void globalStatisticMode(bool s)
    {
        m_globalstatisticmode = s;
    }

    //! change the stream to which allowed output is written (by default: cout)
    static void setWorldStream(std::ostream & o)
    {
        world = &o;
    }

    // EFFECTIVE //////////////////////////////////////
    //! obtain the effective log-level for the Logger-object (i.e., resolve the depenancies on the global settings)
    Level effectiveLogLevel() const
    {
        if(m_logmode == LM_GLOBAL || m_logmode == LM_GLOBALLOG)
            return m_globalloglevel;
        else
            return (m_loglevel > m_minimumloglevel) ? m_loglevel : m_minimumloglevel;
    }

    //! returns true if the Logger-object is effectively in statistic-mode (as this might be depending on the global settings)
    bool effectiveStatisticMode() const
    {
        return m_logmode == LM_STATISTIC || (m_logmode == LM_GLOBAL && m_globalstatisticmode);
    }


private:
    static std::ostream nirvana;
    static std::ostream* world;

    static Level m_globalloglevel;
    static Level m_minimumloglevel;
    static bool m_globalstatisticmode;

    Level m_loglevel;
    LogMode m_logmode;
};

}

#endif // OGDF_LOGGER_H

