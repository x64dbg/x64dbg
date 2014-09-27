/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class Skiplist.
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


#ifndef OGDF_SKIPLIST_H
#define OGDF_SKIPLIST_H

#include <ogdf/basic/basic.h>

namespace ogdf
{

template<class X> class SkiplistIterator;

//! A randomized skiplist
/**
 * The elements height is computed using the traditional coin-flip method, using a
 * 50-50 chance to stop growing. The given running times of the methods below are therefore
 * only expected running times.
 *
 * \warning The code expects the type \a X to be a pointer! If \a X is not a pointer,
 * compiler errors will occur!
 */
template<class X> class Skiplist
{
    friend class SkiplistIterator<X>;
    friend class Element;

    //! Internal structure to hold the items and internal forward pointers of the skiplist
    class Element
    {
        friend class Skiplist<X>;
        friend class SkiplistIterator<X>;

        X entry; // content
        Element** next; // successor elements

        // construction
        Element(const X & item, int height) :
            entry(item)
        {
            next = (Element**)malloc(height * sizeof(Element*));
        }

        ~Element()
        {
            free(next);
        }

        OGDF_NEW_DELETE
    };

public:

    //! Construct an initially empty skiplist
    Skiplist() : lSize(0)
    {
        srand((unsigned int)time(NULL));
        realheight = 5;
        height = 1;
        start = (Element**)malloc(realheight * sizeof(Element*));
        start[0] = NULL;
    }

    ~Skiplist()
    {
        clear();
        free(start);
    }

    //! Returns true if the item \a item is contained in the skiplist [O'(log n)]
    bool isElement(X item) const
    {
        int h = height - 1;
        Element** cur = start; // wheeha!
        while(true)
        {
            if(cur[h] && *(cur[h]->entry) < *item)   //nxt != NULL
                cur = cur[h]->next;
            else if(--h < 0)
                return cur[0] && *(cur[0]->entry) == *item;
        }
    }

    //! Adds the item \a item into the skiplist [O'(log n)]
    void add(X item)
    {
        lSize++;

        int nh = random_height();
        Element* n = OGDF_NEW Element(item, nh);
        if(nh > height)
            grow(nh);

        int h = height - 1;
        Element** cur = start; // wheeha!
        while(true)
        {
            if(cur[h] && *(cur[h]->entry) < *item)   //nxt != NULL
                cur = cur[h]->next;
            else
            {
                if(h < nh)   // add only if new element is high enough
                {
                    n->next[h] = cur[h];
                    cur[h] = n;
                }
                if(--h < 0)
                    return;
            }
        }
    }

    //! Returns the current size of the skiplist, i.e., the number of elements
    int size() const
    {
        return lSize;
    }

    //! Returns true if the skiplist contains no elements
    int empty() const
    {
        return (lSize == 0);
    }

    //! Clears the current skiplist
    /**
    * If \a killData is true, the items of the Skiplist (which are stored as
    * pointers) are automatically deleted.
    */
    void clear(bool killData = false)
    {
        Element* item = start[0];
        Element* old;
        while(item)
        {
            old = item;
            item = item->next[0];
            if(killData)
                delete old->entry;
            delete old;
        }
        lSize = 0;
        height = 1;
        start[0] = 0;
    }

    //! returns an (forward) iterator for the skiplist
    const SkiplistIterator<X> begin() const
    {
        return start[0];
    }

private:
    int lSize;
    Element** start;
    int height;
    int realheight;

    int random_height()
    {
        int h = 1;
        while(rand() > RAND_MAX / 2) h++;
        return h;
    }

    void grow(int newheight)
    {
        if(newheight > realheight)
        {
            realheight = newheight;
            start = (Element**)realloc(start, realheight * sizeof(Element*));
        }
        for(int i = newheight; i-- > height;)
        {
            start[i] = NULL;
        }
        height = newheight;
    }

};

//! Forward-Iterator for Skiplists
template<class X> class SkiplistIterator
{
    friend class Skiplist<X>;

    const typename Skiplist<X>::Element* el;

    SkiplistIterator(const typename Skiplist<X>::Element* e)
    {
        el = e;
    }

public:

    //! Returns the item to which the iterator points
    const X & operator*() const
    {
        return el->entry;
    }

    bool valid() const
    {
        return (el != 0);
    }

    //! Move the iterator one item forward (prefix notation)
    SkiplistIterator<X> & operator++()
    {
        el = el->next[0];
        return *this;
    }
    //! Move the iterator one item forward (prefix notation)
    SkiplistIterator<X> operator++(int)
    {
        SkiplistIterator<X> it = *this;
        el = el->next[0];
        return it;
    }

    //! Assignment operator
    SkiplistIterator<X> & operator=(const SkiplistIterator<X> & it)
    {
        el = it.el;
        return *this;
    }
};

}

#endif /*OGDF_SKIPLIST_H*/
