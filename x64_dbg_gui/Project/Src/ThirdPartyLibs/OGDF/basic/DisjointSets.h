/*
 * $Revision: 2584 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 02:38:07 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of disjoint sets data structures (union-find functionality).
 *
 * \author Andrej Dudenhefner
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

#ifndef DISJOINTSETS_H
#define DISJOINTSETS_H

namespace ogdf
{

#define INTERMEDIATE_PARENT_CHECK
struct AnyOption {};

/**
 * Defines options for linking two sets.
 * NL = Naive Link, LI = Link by Index (default), LS = Link by Size, LR = Link by Rank
 */
enum LinkOptions { NL = 0, LI = 1, LS = 2, LR = 3 };
template<LinkOptions linkOption> struct LinkOption : AnyOption {};
extern const char* linkOptionNames[];

/**
 * Defines options for compression search paths.
 * PC = Path Compression, PS = Path Splitting (default), PH = Path Halving, R1 = Reversal of type 1, CO = Collapsing, NF = No Compression
 */
enum CompressionOptions { PC = 0, PS = 1, PH = 2, R1 = 4, CO = 5, NF = 6 };
template<CompressionOptions compressionOption> struct CompressionOption : AnyOption {};
extern const char* compressionOptionNames[];

/*
 * Defines options for interleaving find/link operations in quickUnion.
 * NI = No Interleaving
 * Rem = Rem's Algorithm (only compatible with linkOption = LI) (default)
 * TvL = Tarjan's and van Leeuwen's Algorithm (only compatible with linkOption = LR)
 * IR0 = Interleaved Reversal of Type 0 (only compatible with linkOption = NF)
 * IPSPC = Interleaved Path Splitting Path Compression (only compatible with linkOption = LI)
 */
enum InterleavingOptions { NI = 0, Rem = 1, TvL = 2, IR0 = 3, IPSPC = 4 };
template<InterleavingOptions interleavingOption> struct InterleavingOption : AnyOption {};
extern const char* interleavingOptionNames[];


//! A Union/Find data structure for maintaining disjoint sets.
template <class Element, LinkOptions linkOption = LI, CompressionOptions compressionOption = PS, InterleavingOptions interleavingOption = NI>
class DisjointSets
{
private:
    int numberOfSets; //!< Current number of disjoint sets.
    int numberOfElements; //!< Current number of elements.
    int maxNumberOfElements; //!< Maximum number of elements (array size) adjusted dynamically.

    // Arrays parents, elements, parameters, siblings map a set id to its properties.

    int* parents; //!< Maps set id to parent set id.
    Element* elements; //!< Maps set id to element.
    int* parameters; //!< Maps set id to rank/size.
    int* siblings; //!< Maps set id to sibling set id.

    //find
    int find(CompressionOption<PC>, int set);
    int find(CompressionOption<PS>, int set);
    int find(CompressionOption<PH>, int set);
    int find(CompressionOption<R1>, int set);
    int find(CompressionOption<CO>, int set);
    int find(CompressionOption<NF>, int set);

    //link
    int link(LinkOption<NL>, int set1, int set2);
    int link(LinkOption<LI>, int set1, int set2);
    int link(LinkOption<LS>, int set1, int set2);
    int link(LinkOption<LR>, int set1, int set2);

    //quickUnion
    bool quickUnion(LinkOption<LI>, InterleavingOption<Rem>, int set1, int set2);
    bool quickUnion(LinkOption<LI>, InterleavingOption<IPSPC>, int set1, int set2);
    bool quickUnion(LinkOption<LR>, InterleavingOption<TvL>, int set1, int set2);
    bool quickUnion(AnyOption, InterleavingOption<NI>, int set1, int set2);
    bool quickUnion(LinkOption<NL>, InterleavingOption<IR0>, int set1, int set2);

public:
    //! Cretes an empty DisjointSets structure.
    /**
    * \param maxNumberOfElements Expected number of Elements.
    */
    DisjointSets(int maxNumberOfElements = (1 << 15))
    {
        this->numberOfSets = 0;
        this->numberOfElements = 0;
        this->maxNumberOfElements = maxNumberOfElements;
        this->parents = new int[this->maxNumberOfElements];
        this->elements = new Element[this->maxNumberOfElements];
        this->parameters = (linkOption == LR || linkOption == LS) ? new int[this->maxNumberOfElements] : 0;
        this->siblings = (compressionOption == CO) ? new int[this->maxNumberOfElements] : 0;
    }

    ~DisjointSets()
    {
        delete [] this->parents;
        delete [] this->elements;
        if(this->parameters != 0) delete [] this->parameters;
        if(this->siblings != 0) delete [] this->siblings;
    }

    //! Returns the id of the largest superset of \a set and compresses the path according to \a compressionOption.
    /**
    * \param set Set.
    * \return Superset id
    * \pre \a set is a non negative properly initialized id.
    */
    int find(int set)
    {
        return find(CompressionOption<compressionOption>(), set);
    }

    //! Returns the id of the largest superset of \a set.
    /**
    * \param set Set.
    * \return Superset id
    * \pre \a set is a non negative properly initialized id.
    */
    int getRepresentative(int set)
    {
        while(set != parents[set]) set = parents[set];
        return set;
    }

    //! Initializes a singleton set containing \a element.
    /**
    * \param element Element.
    * \return Set id of the initialized singleton set.
    */
    int makeSet(Element element)
    {
        if(this->numberOfElements == this->maxNumberOfElements)
        {
            int* parents = this->parents;
            this->parents = new int[this->maxNumberOfElements * 2];
            memcpy(this->parents, parents, sizeof(int)*this->maxNumberOfElements);
            delete [] parents;

            Element* elements = this->elements;
            this->elements = new Element[this->maxNumberOfElements * 2];
            memcpy(this->elements, elements, sizeof(Element)*this->maxNumberOfElements);
            delete [] elements;

            if(this->parameters != 0)
            {
                int* parameters = this->parameters;
                this->parameters = new int[this->maxNumberOfElements * 2];
                memcpy(this->parameters, parameters, sizeof(int)*this->maxNumberOfElements);
                delete [] parameters;
            }

            if(this->siblings != 0)
            {
                int* siblings = this->siblings;
                this->siblings = new int[this->maxNumberOfElements * 2];
                memcpy(this->siblings, siblings, sizeof(int)*this->maxNumberOfElements);
                delete [] siblings;
            }
            this->maxNumberOfElements *= 2;
        }
        this->numberOfSets++;
        int id = this->numberOfElements++;
        this->parents[id] = id;
        this->elements[id] = element;
        //Initialize size/ rank/ sibling.
        if(linkOption == LS) this->parameters[id] = 1;
        else if(linkOption == LR) this->parameters[id] = 0;
        if(compressionOption == CO) this->siblings[id] = -1;
        return id;
    }

    //! Unions \a set1 and \a set2.
    /**
    * \pre \a set1 and \a set2 are maximal disjoint sets.
    * \return Set id of the union.
    */
    int link(int set1, int set2)
    {
        if(set1 == set2) return -1;
        this->numberOfSets--;
        int superset = link(LinkOption<linkOption>(), set1, set2);
        //Collapse subset tree.
        if(compressionOption == CO)
        {
            int subset = set1 == superset ? set2 : set1;
            int id = subset;
            while(this->siblings[id] != -1)
            {
                id = this->siblings[id];
                this->parents[id] = superset;
            }
            this->siblings[id] = this->siblings[superset];
            this->siblings[superset] = subset;
        }
        return superset;
    }

    //! Unions the maximal disjoint sets containing \a set1 and \a set2.
    /**
    * \return True, if the maximal sets containing \a set1 and \a set2 were disjoint und joined correctly. False otherwise.
    */
    bool quickUnion(int set1, int set2)
    {
        if(set1 == set2) return false;
        this->numberOfSets--;
        return quickUnion(LinkOption<linkOption>(), InterleavingOption<interleavingOption>(), set1, set2);
    }

    //! Returns the current number of disjoint sets.
    int getNumberOfSets()
    {
        return numberOfSets;
    }

    //! Returns the current number of elements.
    int getNumberOfElements()
    {
        return numberOfElements;
    }
};


//find
template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::find(CompressionOption<PC>, int set)
{
    int parent = parents[set];
    if(set == parent)
    {
        return set;
    }
    else
    {
        parent = find(CompressionOption<PC>(), parent);
        parents[set] = parent;
        return parent;
    }
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::find(CompressionOption<PH>, int set)
{
    while(set != parents[set])
    {
        int parent = parents[set];
        int grandParent = parents[parent];
        parents[set] = grandParent;
        set = grandParent;
    }
    return set;
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::find(CompressionOption<PS>, int set)
{
    int parent = parents[set];
    int grandParent = parents[parent];
    while(parent != grandParent)
    {
        parents[set] = grandParent;
        set = parent;
        parent = grandParent;
        grandParent = parents[grandParent];
    }
    return parent;
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::find(CompressionOption<R1>, int set)
{
    int root = set;
    set = parents[root];

    while(set != parents[set])
    {
        int parent = parents[set];
        parents[set] = root;
        set = parent;
    }
    parents[root] = set;
    return set;
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::find(CompressionOption<NF>, int set)
{
    while(set != parents[set]) set = parents[set];
    return set;
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::find(CompressionOption<CO>, int set)
{
    return parents[set];
}


//quickUnion
template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<Element, linkOption, compressionOption, interleavingOption>::quickUnion(AnyOption, InterleavingOption<NI>, int set1, int set2)
{
#ifdef INTERMEDIATE_PARENT_CHECK
    if(parents[set1] == parents[set2]) return false;
#endif
    set1 = find(set1);
    set2 = find(set2);
    if(set1 != set2)
    {
        link(set1, set2);
        return true;
    }
    return false;
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<Element, linkOption, compressionOption, interleavingOption>::quickUnion(LinkOption<NL>, InterleavingOption<IR0>, int set1, int set2)
{
#ifdef INTERMEDIATE_PARENT_CHECK
    if(parents[set1] == parents[set2]) return false;
#endif
    int root = set2;
    int set = set2;
    int parent = parents[set];
    parents[set] = root;
    while(set != parent)
    {
        if(parent == set1)
        {
            parents[root] = set1;
            return false;
        }
        set = parent;
        parent = parents[set];
        parents[set] = root;
    }

    set = set1;
    parent = parents[set];
    while(true)
    {
        if(parent == root) return false;
        parents[set] = root;
        if(parent == set) return true;
        set = parent;
        parent = parents[set];
    }
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<Element, linkOption, compressionOption, interleavingOption>::quickUnion(LinkOption<LI>, InterleavingOption<Rem>, int set1, int set2)
{
    int r_x = set1;
    int r_y = set2;
    int p_r_x = parents[r_x];
    int p_r_y = parents[r_y];
    while(p_r_x != p_r_y)
    {
        if(p_r_x < p_r_y)
        {
            if(r_x == p_r_x)
            {
                parents[r_x] = p_r_y;
                return true;
            }
            parents[r_x] = p_r_y;
            r_x = p_r_x;
            p_r_x = parents[r_x];
        }
        else
        {
            if(r_y == p_r_y)
            {
                parents[r_y] = p_r_x;
                return true;
            }
            parents[r_y] = p_r_x;
            r_y = p_r_y;
            p_r_y = parents[r_y];
        }
    }
    return false;
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<Element, linkOption, compressionOption, interleavingOption>::quickUnion(LinkOption<LI>, InterleavingOption<IPSPC>, int set1, int set2)
{
#ifdef INTERMEDIATE_PARENT_CHECK
    if(parents[set1] == parents[set2]) return false;
#endif
    int set = set1;

    if(set1 < set2)
    {
        set = set2;
        set2 = set1;
        set1 = set;
    }

    //!Use path splitting to compress the path of set1 and get the root
    set = parents[set];
    int parent = parents[set];
    int grandParent = parents[parent];
    while(parent != grandParent)
    {
        parents[set] = grandParent;
        set = parent;
        parent = grandParent;
        grandParent = parents[grandParent];
    }
    parents[set1] = parent;
    int root = parent;

    //!Redirect all nodes with smaller indices on the path of set2 to the root
    set = set2;
    parent = parents[set];
    while(true)
    {
        if(parent < root)
        {
            parents[set] = root;
            if(set == parent) return true;
            set = parent;
            parent = parents[set];
        }
        else if(parent > root)
        {
            parents[root] = parent;
            parents[set1] = parent;
            parents[set2] = parent;
            return true;
        }
        else return false;
    }
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
bool DisjointSets<Element, linkOption, compressionOption, interleavingOption>::quickUnion(LinkOption<LR>, InterleavingOption<TvL>, int set1, int set2)
{
    int r_x = set1;
    int r_y = set2;
    int p_r_x = parents[r_x];
    int p_r_y = parents[r_y];
    while(p_r_x != p_r_y)
    {
        if(parameters[p_r_x] <= parameters[p_r_y])
        {
            if(r_x == p_r_x)
            {
                if(parameters[p_r_x] == parameters[p_r_y])
                {
                    if(p_r_y == parents[p_r_y])
                    {
                        parameters[p_r_y]++;
                    }
                }
                parents[r_x] = parents[p_r_y];
                return true;
            }
            parents[r_x] = p_r_y;
            r_x = p_r_x;
            p_r_x = parents[r_x];
        }
        else
        {
            if(r_y == p_r_y)
            {
                parents[r_y] = parents[p_r_x];
                return true;
            }
            parents[r_y] = p_r_x;
            r_y = p_r_y;
            p_r_y = parents[r_y];
        }
    }
    return false;
}


//link
template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::link(LinkOption<LI>, int set1, int set2)
{
    if(set1 < set2)
    {
        parents[set1] = set2;
        return set2;
    }
    else
    {
        parents[set2] = set1;
        return set1;
    }
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::link(LinkOption<LR>, int set1, int set2)
{
    int parameter1 = parameters[set1];
    int parameter2 = parameters[set2];

    if(parameter1 < parameter2)
    {
        parents[set1] = set2;
        return set2;
    }
    else if(parameter1 > parameter2)
    {
        parents[set2] = set1;
        return set1;
    }
    else
    {
        parents[set1] = set2;
        parameters[set2]++;
        return set2;
    }
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::link(LinkOption<LS>, int set1, int set2)
{
    int parameter1 = parameters[set1];
    int parameter2 = parameters[set2];

    if(parameter1 < parameter2)
    {
        parents[set1] = set2;
        parameters[set2] += parameter1;
        return set2;
    }
    else
    {
        parents[set2] = set1;
        parameters[set1] += parameter2;
        return set1;
    }
}

template <class Element, LinkOptions linkOption, CompressionOptions compressionOption, InterleavingOptions interleavingOption>
int DisjointSets<Element, linkOption, compressionOption, interleavingOption>::link(LinkOption<NL>, int set1, int set2)
{
    parents[set1] = set2;
    return set2;
}

}  // end namespace ogdf

#endif
