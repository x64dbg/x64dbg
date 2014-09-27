/*
 * $Revision: 2528 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 23:05:08 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Offers colorizer for SimDraw.
 *
 * \author Michael Schulz and Tobias Dehling
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


#ifndef OGDF_SIMDRAW_COLORIZER_H
#define OGDF_SIMDRAW_COLORIZER_H

#include <ogdf/simultaneous/SimDrawManipulatorModule.h>

namespace ogdf
{

//! Adds color to a graph
/** SimDrawColorizer adds colors to the edges (and nodes) of a simdraw
*  instance. The underlying color scheme sets the used colors.
*  Some color schemes are only usable for a small number of basic graphs.
*
*  A typical use of SimDrawColorizer involves a predefined SimDraw
*  instance on which SimDrawColorizer works.
*  \code
*  SimDraw SD;
*  SimDrawColorizer SDC(SD);
*  [...]
*  SDC.addColor();
*  [...]
*  \endcode
*/
class OGDF_EXPORT SimDrawColorizer : public SimDrawManipulatorModule
{

public:
    //! types for colorschemes
    enum colorScheme
    {
        none, //!< <= 32 different colors
        bluYel, //!< blue and yellow <= 2 colors
        redGre, //!< red and green <= 2 colors
        bluOra, //!< blue and orange <= 2 colors
        teaLil, //!< teal and purple <= 2 colors
        redBluYel, //!< red, blue and yellow <= 3 colors
        greLilOra //!< green, purple and orange <= 3 colors
    };

private:
    //! stores the current colorscheme
    colorScheme m_colorScheme;

public:
    //! constructor assigns default color scheme
    SimDrawColorizer(SimDraw & SD) : SimDrawManipulatorModule(SD)
    {
        m_colorScheme = none;
    }

    //! returns current color scheme
    const colorScheme & ColorScheme() const
    {
        return m_colorScheme;
    }

    //! assigns a new color scheme
    colorScheme & ColorScheme()
    {
        return m_colorScheme;
    }

    //! adds color to a graph including nodes
    void addColorNodeVersion();

    //! adds some color to a graph
    void addColor();

public:
    //! Manages the various color schemes
    /**
    *  Color schemes are used within SimDrawColorizer to chose
    *  different colors for the basic graph visualizations.
    *  It is used directly within SimDrawColorizer.
    *
    *  \code
    *  SimDraw SD;
    *  SimDrawColorizer SDC(SD);
    *  [...]
    *  SDC.ColorScheme() = SimDrawColorizer::redGre;
    *  SDC.addColor();
    *  [...]
    *  \endcode
    *
    *  CAUTION: Some color schemes are only valid for a small
    *  number (e.g. two or three) of basic graphs. The default color
    *  scheme can be used for up to 32 basic graphs.
    */
    class SimDrawColorScheme
    {
    private:
        //! stores the current colorscheme (set by constructor)
        colorScheme m_intScheme;

        //! red color component
        /** stores the values of the red color component for every graph
        *  according to colorscheme
        */
        int* red;

        //! green color component
        /** stores the values of the green color component for every graph
        *  according to colorscheme
        */
        int* green;

        //! blue color component
        /** stores the values of the blue color component for every graph
        *  according to colorscheme
        */
        int* blue;

    public:
        //! constructor
        SimDrawColorScheme(enum colorScheme colorScm, int numberOfGraphs);

        //! destructor
        ~SimDrawColorScheme();

        //! joins the different color components together
        String getColor(int subGraphBits, int numberOfGraphs);

        //! sets the color component arrays according to colorschemeXS
        void assignColScm(int numberOfGraphs);
    }; // SimDrawColorScheme

}; // SimDrawColorizer

}

#endif
