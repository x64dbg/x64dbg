/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of base class for drawing constraints.
 *
 * \author PG478
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

#ifndef OGDF_CONSTRAINTS_H
#define OGDF_CONSTRAINTS_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/String.h>


namespace ogdf
{


class GraphConstraints;
struct XmlTagObject;


/* -------------------------- Constraint ----------------------------------------- */
class Constraint
{
    friend class GraphConstraints;

private:
    ListIterator<Constraint*> listIt;

protected:
    String m_Name;
    /* */
    bool m_UserDisabled;

    int m_Status; // OK, Suspended, notOk
    /* The Graph this Constraint belongs to. No Constraint without a Graph! */
    const Graph* m_pGraph;

    /* Will be called by the GraphConstraints when a Node in mGraph is removed. */
    virtual void nodeDeleted(node v) { }

public:
    Constraint(const Graph & g) : m_pGraph(&g)
    {
        m_Status = 0;
        m_UserDisabled = false;
    }

    virtual ~Constraint() {}

    const Graph & constGraph() const
    {
        return *m_pGraph;
    }

    virtual bool isValid()
    {
        return (!m_UserDisabled && (m_Status == 0));
    }

    virtual int getInternalStatus()
    {
        return m_Status;
    }

    virtual void setInternalStatus(int s)
    {
        m_Status = s;
    }

    //virtual bool load(int stuff) = 0;
    //virtual bool save(int stuff) = 0;
    //virtual bool checkConstraints(List<Constraint*> csts) {return true; }

    void userEnable()
    {
        m_UserDisabled = false;
    }

    void userDisable()
    {
        m_UserDisabled = true;
    }

    bool isUserDisabled()
    {
        return m_UserDisabled;
    }

    virtual int getType()
    {
        return 0;
    }

    static int getStaticType()
    {
        return 0;
    }

    void setName(String text)
    {
        m_Name = text;
    }

    String getName()
    {
        return m_Name;
    }

    //DEBUG
    virtual bool buildFromOgml(XmlTagObject* constraintTag, Hashing <String, node>* nodes);
    virtual bool storeToOgml(int id, ostream & os, int indentStep);
    //static void generateIndent(char ** indent, const int & indentSize);

    OGDF_NEW_DELETE
};


/* -------------------------- GraphConstraints ----------------------------------------- */
/* For each Graph a Set of Constraints*/
class GraphConstraints //: public GraphStructure
{
    friend class ConstraintManager;

protected:
    /* The Graph this Constraint belongs to. No Constraint without a Graph! */
    const Graph* m_pGraph;

    /* All Constraints for mGraph */
    List<Constraint*> m_List;

public:
    /* Constructor, GraphStructure */
    GraphConstraints(const Graph & g) : m_pGraph(&g), m_List() { }

    /* Returns the Graph */
    const Graph* constGraph()
    {
        return m_pGraph;
    }

    /* adds a Constraint to this Set */
    void addConstraint(Constraint* c)
    {
        c->listIt = m_List.pushBack(c);
    }

    /* removes a Constraint from this set */
    void removeConstraint(Constraint* c)
    {
        m_List.del(c->listIt);
    }

    /* returns all Constraints */
    List<Constraint*>* getConstraints()
    {
        return &m_List;
    }

    /* Return Constraint Count */
    int numberOfConstraints()
    {
        return m_List.size();
    }

    /* returns all Constraints of Type type */
    List<Constraint*> getConstraintsOfType(int type);

    template<class TYP> void getConstraintsOfType(List<TYP*>* res)
    {
        //List<TYP *> res;
        ListConstIterator<Constraint*> it;
        for(it = m_List.begin(); it.valid(); ++it)
        {
            Constraint* c = *it;

            if(TYP::getStaticType() == c->getType())
            {
                if(c->isValid()) res->pushBack(static_cast<TYP*>(c));
            }
        }
        //return res;
    }

    void clear()
    {
        m_List.clear();
    }

    virtual ~GraphConstraints()
    {
        ListConstIterator<Constraint*> it;
        for(it = m_List.begin(); it.valid(); ++it)
        {
            Constraint* c = *it;
            delete c;
        }
    }

    virtual void nodeDeleted(node v);
    virtual void nodeAdded(node v)    { }
    virtual void edgeDeleted(edge e)  { }
    virtual void edgeAdded(edge e)    { }
    virtual void reInit()             { }
    virtual void cleared()            { }

    OGDF_NEW_DELETE
};


/* -------------------------- ConstraintManager ----------------------------------------- */
class ConstraintManager
{
public:

    static Constraint* createConstraintByName(const Graph & G, String* name);

    static String getClassnameOfConstraint(Constraint* c);

    OGDF_NEW_DELETE
};

} // end namespace

#endif
