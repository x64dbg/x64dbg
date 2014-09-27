/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of EdgeLabelPositioner classes used in EdgeLabel placement.
 *
 * \author Karsten Klein
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

#ifndef OGDF_EDGE_LABEL_H
#define OGDF_EDGE_LABEL_H


#include <ogdf/basic/geometry.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/BinaryHeap2.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/planarity/PlanRepUML.h>
#include <ogdf/basic/GridLayoutMapped.h>
#include <ogdf/orthogonal/OrthoRep.h>

#include <ogdf/labeling/ELabelInterface.h>


//debug
//#define foutput

namespace ogdf
{

//*******************************************************
//all label types are declared in ELabelInterface

//every edge has a number of labels associated with it
//current status:

//two end labels
//two end multiplicities
//a name

//each label has two coordinates and its input size
//*******************************************************


//*******************************************************
//there are some discrete phases:
// *compute(UML)Candidates:
//  assign candidate positions to all input labels
// *test(UML)FeatureIntersect:
//  test intersection of graph features by position candidates
//  currently, only nodes are considered
// *test(UML)AllIntersect:
//  test label position intersection against each other
//  and save the resulting information
// *Assignment of "good" candidates
//  different heuristics can be applied
//*******************************************************


//parameter values for write(UML)GML - output function
enum OutputParameter {opStandard, opOmitIntersect, opOmitFIntersect, opResult};

//the candidate status values have the following meaning
//csActive: candidate can be assigned
//csAssigned: another candidate was assigned for that label
//csFIntersect: label would intersect Graph node
//csUsed: this candidate was used for the label

//csActive and csUsed are both counted as active candidates
//in PosInfo structure, cause they can have influence on the number
//of necessary intersections
enum candStatus {csAssigned, csFIntersect, csActive, csUsed};



//class ELabelPos is responsible for computation of edge label positions

//this should NOT be a template class
//UG call is for double only, PRU call for int, PRU call maybe be considered obsolete
//and is no longer maintained (but works)
template <class coordType>
class ELabelPos
{

public:
    //construction and destruction
    ELabelPos();

    ~ELabelPos() {}

    //we can call the positioner on a given PlanRepUML (grid)drawing or
    //just with given (double)positions for all graph features and label sizes
    virtual void call(PlanRepUML & pru, GridLayoutMapped & L, ELabelInterface<coordType> & eli); //int

    virtual void call(GraphAttributes & ug, ELabelInterface<coordType> & eli); //double

    //set the edge-label distance
    void setDefaultDistance(coordType dist)
    {
        m_defaultDistance = dist;
    }

    void setDistance(edge e, eLabelType elt, coordType dist)
    {
        (m_distance[elt])[e] = dist;
    }

    //switch if all label types should be distributed evenly on the edge length
    void setEndLabelPlacement(bool b)
    {
        m_endLabelPlacement = b;
    }

    void writeGML(const char* filename = "labelgraph.gml", OutputParameter sectOmit = opStandard);
    void writeUMLGML(const char* filename = "labelgraphUML.gml", OutputParameter sectOmit = opStandard);

protected:

    //**********************************************************
    //needed structures

    struct SegmentInfo
    {
        //SegmentInfo() {length = 0; number = 0; direction = odNorth;}

        coordType length;
        int number;
        coordType min_x, max_x, min_y, max_y;
        OrthoDir direction;
    };//Segmentinfo


    struct FeatureInfo
    {
        coordType min_x, max_x, min_y, max_y, size_x, size_y;
    };//FeatureInfo

    //Candidate positions have the following properties:
    //*active/passive (if intersection)
    //*pointer list, pointers to all intersecting candidates
    //*Number of intersections with status active (=> if number == 0, this
    //candidate can be assigned,fuer andere Kandidaten deren Ueberlappungspartner --Anzahl
    //sowie die aktiven Ueberlappungen vorlaeufig sperren
    struct PosInfo
    {
        GenericPoint<coordType> m_coord; //position of middle
        List< PosInfo* >  m_intersect;
        int m_numActive; //active label intersections
        int m_numFeatures; //number of intersected features
        int m_posIndex; //holds the relative position of candidates

        edge m_edge;
        eLabelType m_typ;
        candStatus m_active; //csAssigned assigned, csFIntersect  node, csActive

        double m_cost; //costs for intersections, placement

        PosInfo()
        {
            m_edge = 0;
            m_active = csActive;
            m_posIndex = 0;
            m_numActive = 0;
            m_typ = elName;
            m_cost = 0.0;
        }
        PosInfo(edge e, eLabelType elt, GenericPoint<coordType> gp, int posIndex = -1)
        {
            m_edge = e;
            m_typ = elt;
            m_coord = gp;
            m_numActive = 0;
            m_numFeatures = 0;
            m_posIndex = posIndex;
            m_active = csActive;
            m_cost = 0.0;
        }
        PosInfo(edge e, eLabelType elt)
        {
            m_edge = e;
            m_typ = elt;
            m_numActive = 0;
            m_numFeatures = 0;
            m_posIndex = 0;
            m_active = csActive;
            m_cost = 0.0;
        }

        bool active()
        {
            return (m_active == csActive) || (m_active == csUsed);
        }
        //deactivate candidate
        void deactivate() {}

    };//PosInfo


    //used for sorting and intersection graph buildup
    struct FeatureLink
    {
        FeatureInfo m_fi;
        edge m_edge;
        eLabelType m_elt;
        int m_index; //index of label position entry in list
        node m_node; //representant in intersection graph
        PosInfo* m_posInfo;

        FeatureLink()
        {
            m_edge = 0;
            m_elt = (eLabelType)0;
            m_index = 0;
            m_node = 0;
            m_posInfo = 0;
        }
        FeatureLink(edge e, eLabelType elt, node v, FeatureInfo & fi, int index)
        {
            m_edge = e;
            m_elt = elt;
            m_node = v;
            m_fi = fi;
            m_index = index;
            m_posInfo = 0;
        }
        FeatureLink(edge e, eLabelType elt, node v, FeatureInfo & fi, int index, PosInfo & pi)
        {
            m_edge = e;
            m_elt = elt;
            m_node = v;
            m_fi = fi;
            m_index = index;
            m_posInfo = &pi;
        }
    };//FeatureLink

    struct LabelInfo
    {
        edge m_e;
        int m_labelTyp;

        int m_index; //index in der PosListe

        LabelInfo()
        {
            m_e = 0;
            m_labelTyp = m_index = 0;
        }
        LabelInfo(edge e, int l, int i)
        {
            m_e = e;
            m_labelTyp = l;
            m_index = i;
        }
    };//LabelInfo


    //**********************************************************
    //modification

    //********
    //settings
    //cost for feature (node?) intersection
    double costFI()
    {
        return m_posNum * 5.0;
    }
    //cost for label intersection
    double costLI()
    {
        return 0.9;
    }//2.0;}
    //cost for edge intersection
    double costEI()
    {
        return 0.7;
    }
    //cost for distance from standard position,
    double costPos()
    {
        return 0.6;   //e.g. edge start for start label
    }
    //cost for non-symmetry at start/end label pairs
    double costSym()
    {
        return 1.3;
    }

    coordType segmentMargin()
    {
        return m_segMargin;   //defining the size of the rectangle
    }
    //around a segment for intersection testing

    bool usePosCost()
    {
        return m_posCost;
    }
    bool useSymCost()
    {
        return m_symCost;
    }

    //**********************************************************
    //main parts of the algorithm
    //check edges for segment structure
    void initSegments();
    void initUMLSegments();

    //build rectangle structures for all graph features
    void initFeatureRectangles();
    void initUMLFeatureRectangles();

    //computes the candidate positions, fills the lists
    void computeCandidates();
    void computeUMLCandidates();

    //build up data structure to decide feasible solutions
    //only needed if labeltree not already build
    void initStructure();

    //check label candidates for feature intersection, delete from list
    void testFeatureIntersect();
    void testUMLFeatureIntersect();

    //assign special candidate to avoid empty list after featuretest
    void saveRecovery(EdgeArray< GenericPoint<coordType> > (&saveCandidate)[elNumLabels]);
    void saveUMLRecovery(EdgeArray< GenericPoint<coordType> > (&saveCandidate)[elNumLabels]);

    //check label candidates for label intersection
    void testAllIntersect();
    void testUMLAllIntersect();


    //return the list of all label position candidates (PlanRepUML)
    List< GenericPoint<coordType> > & posList(edge e, int lnum)
    {
        return (m_candPosList[lnum])[e];
    }
    //return the list of all label position candidates
    List< PosInfo > & candList(edge e, int lnum)
    {
        return (m_candList[lnum])[e];
    }


    //****************************
    //information about the segments
    //return number of segments for original edge e
    int segNumber(edge e)
    {
        return m_poly[e].size() - 1;
    }
    //return direction (?ver/hor?) of edge segments, dynamic version
    //of SegInfo.dir
    OrthoDir segDir(edge e, int segNum)
    {
        OrthoDir od;
        if(segNum > segNumber(e)) OGDF_THROW(Exception);

        IPoint ip1 = (*m_poly[e].get(segNum - 1));
        IPoint ip2 = (*m_poly[e].get(segNum));
        bool isHor = (ip1.m_y == ip2.m_y); //may still be same place
        if(isHor)
        {
            if(ip1.m_x > ip2.m_x) od = odWest;
            else od = odEast;
        }//if isHor
        else
        {
            //check m_x == m_x
            if(ip1.m_y < ip2.m_y) od = odNorth;
            else od = odSouth;
        }//else

        return od;
    }//segDir


private:

    //settings
    int m_numAssignment; //number of necessary assignments

    int m_candStyle; //defines the style how pos cands are computed
    int m_placeHeuristic; //defines how candidates are chosen
    bool m_endInsertion; //should candidates nearest to endnode be chosen

    bool m_endLabelPlacement; //are endlabels candidates computed near the nodes

    bool m_posCost; //should end label distance to end give a cost for candidates
    bool m_symCost; //should non-symmetric assignment for end label pairs -"-

    //number of candidates for every label
    //end pos. candidates get double the number , cause a position
    //on both sides of the edge is possible for these values
    int m_posNum; //number of pos. cand. for candstyle 1, like ppinch

    //only internally used option: dont stop after first feature intersection,
    //allows weighting over number of feature intersections
    bool m_countFeatureIntersect;

    coordType m_segMargin;

    //pointers to the PlanRepUML input instances
    PlanRepUML* m_prup;
    GridLayoutMapped* m_gl; //the existing drawing

    //pointers to the AttributedGraph input instances
    GraphAttributes* m_ug;

    //pointers to the generic input instances
    ELabelInterface<coordType>* m_eli;//the input/output interface

    //maybe this should be a parameter, reference
    //forall label types the cand. pos.
    EdgeArray< List<GenericPoint<coordType> > > m_candPosList[elNumLabels];
    //wird ersetzt durch: Liste von PosInfos
    EdgeArray< List < PosInfo > > m_candList[elNumLabels];
    //structure holding all intersection free labels
    List< PosInfo* > m_freeLabels;
    //structure holding all intersecting labels (should be PQ)
    List< PosInfo* > m_sectLabels;
    //structure holding candidates sorted by associated costs
    BinaryHeap2<double, PosInfo*> m_candidateHeap;

    //the bends and crossings, in the UML call use AttributedGraph::bends
    EdgeArray< List<GenericPoint<coordType> > > m_poly;

    //list of segment info
    EdgeArray< List<SegmentInfo> > m_segInfo;
    EdgeArray<coordType> m_edgeLength;

    //list of graph feature(nodes,...) info
    NodeArray<FeatureInfo> m_featureInfo; //on prup-original

    //save the intersections
    EdgeArray< List< List<LabelInfo> > > m_intersect[elNumLabels];

    EdgeArray<bool> m_assigned[elNumLabels]; //edges with already assigned labels?

    //allow specific edge to label distance, there is a default value
    EdgeArray<coordType> m_distance[elNumLabels];
    coordType m_defaultDistance;

    Graph m_intersectGraph;

    void init(PlanRepUML & pru, GridLayoutMapped & L, ELabelInterface<coordType> & eli); //int

    void initUML(GraphAttributes & ug, ELabelInterface<coordType> & eli); //double

    //intersection test section
    //we have to sort the features and therefore define a special method
    class FeatureComparer;
    friend class FeatureComparer;
    class FeatureComparer
    {
    public:
        //we sort from lower (bottom side) to upper and from left to right side
        static int compare(const FeatureLink & f1, const FeatureLink & f2)
        {
            coordType d1 = f1.m_fi.min_y;
            coordType d2 = f2.m_fi.min_y;

            if(DIsLess(d1, d2)) return -1;
            else if(DIsGreater(d1, d2)) return 1;
            else
            {
                if(DIsLess(f1.m_fi.min_x, f2.m_fi.min_x)) return -1;
                else if(DIsGreater(f1.m_fi.min_x, f2.m_fi.min_x)) return 1;

                return 0;
            }
        }
        OGDF_AUGMENT_STATICCOMPARER(FeatureLink)
    };


};//ELabelPos

}//namespace

//#if defined(_MSC_VER) || defined(__BORLANDC__)
#include <src/orthogonal/EdgeLabel-impl.h>
//#endif

#endif
