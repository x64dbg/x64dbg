/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Contains diverse enumerations and string constants.
 *        See comments for further information.
 *
 * \author Christian Wolf
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

#ifndef OGDF_OGML_H
#define OGDF_OGML_H

#include <ogdf/basic/String.h>


namespace ogdf
{

class Ogml
{
public:

    //! Max count of ogml tags (used for setting maxOccurs).
    static const int MAX_TAG_COUNT = 4000;


    //! This enumeration is used for identifying OGML tags.
    enum TagId
    {
        t_none = -1,
        t_bool,
        t_composed,
        t_constraint,
        t_constraints,
        t_content,
        t_data,
        t_default,
        t_edge,
        t_edgeRef,
        t_edgeStyle,
        t_edgeStyleTemplate,
        t_edgeStyleTemplateRef,     //!< tag template within tag edgeStyle/-Template
        t_endpoint,
        t_fill,
        t_font,
        t_graph,
        t_graphStyle,
        t_int,
        t_label,
        t_labelRef,
        t_labelStyle,
        t_labelStyleTemplate,
        t_labelStyleTemplateRef,    //!< tag template within tag labelStyle/-Template
        t_layout,
        t_line,
        t_location,
        t_node,
        t_nodeRef,
        t_nodeStyle,
        t_nodeStyleTemplate,
        t_nodeStyleTemplateRef,     //!< tag template within tag nodeStyle/-Template
        t_num,
        t_ogml,
        t_point,
        t_port,
        t_segment,
        t_shape,
        t_source,
        t_sourceStyle,
        t_string,
        t_structure,
        t_styles,
        t_styleTemplates,
        t_target,
        t_targetStyle,
        t_text,
        t_image,

        TAG_NUM                     //!< number of tags
    };


    //! Stores the names of all OGML tags.
    static const String s_tagNames[TAG_NUM];


    //! This enumeration is used for identifying OGML attributes.
    enum AttributeId
    {
        a_none = -1,
        a_alignment,
        a_angle,
        a_color,
        a_decoration,
        a_defaultEdgeTemplate,
        a_defaultLabelTemplate,
        a_defaultNodeTemplate,
        a_family,
        a_height,
        a_id,                       //!< id attribute
        a_nodeIdRef,                //!< attribute idRef of elements source, target, nodeRef, nodeStyle
        a_edgeIdRef,                //!< attribute idRef of elements edgeRef, edgeStyle
        a_labelIdRef,               //!< attribute idRef of elements edgeRef, edgeStyle
        a_sourceIdRef,              //!< attribute idRef of element endpoint
        a_targetIdRef,              //!< attribute idRef of element endpoint
        a_nodeStyleTemplateIdRef,   //!< attribute idRef of subelement template of element nodeStyle
        a_edgeStyleTemplateIdRef,   //!< attribute idRef of subelement template of element edgeStyle
        a_labelStyleTemplateIdRef,  //!< attribute idRef of subelement template of element labelStyle
        a_endpointIdRef,            //!< attribute idRef of subelement endpoint of element segment
        a_name,
        a_nLineType,                //!< attribute type of subelement line of tag nodeStyleTemplate
        a_nShapeType,               //!< attribute type of subelement shape of tag nodeStyleTemplate
        a_pattern,
        a_patternColor,
        a_rotation,
        a_size,
        a_stretch,
        a_style,
        a_transform,
        a_type,                     //!< attribute type of subelements source-/targetStyle of tag edgeStyleTemplate
        a_uri,
        a_intValue,
        a_boolValue,
        a_numValue,
        a_variant,
        a_weight,
        a_width,
        a_x,
        a_y,
        a_z,
        a_imageUri,
        a_imageStyle,
        a_imageAlignment,
        a_imageDrawLine,
        a_imageWidth,
        a_imageHeight,
        a_constraintType,
        a_disabled,

        ATT_NUM                 //!< number of attributes
    };


    //! Stores the names of all OGML attributes.
    static const String s_attributeNames[ATT_NUM];


    //! This enumeration is used for identifying OGML attributes.
    enum AttributeValueId
    {
        av_any = 0,                 //!< for any attributeValue
        av_blink,
        av_bold,
        av_bolder,
        av_bool,
        av_box,
        av_capitalize,
        av_center,
        av_checked,
        av_circle,
        av_condensed,
        av_cursive,
        av_dashed,
        av_esNoPen,                 //!< values for line style
        av_esSolid,
        av_esDash,
        av_esDot,
        av_esDashdot,
        av_esDashdotdot,
        av_diamond,
        av_dotted,
        av_double,
        av_doubleSlash,
        av_ellipse,
        av_expanded,
        av_extraCondensed,
        av_extraExpanded,
        av_fantasy,
        av_filledBox,
        av_filledCircle,
        av_filledDiamond,
        av_filledHalfBox,
        av_filledHalfCircle,
        av_filledHalfDiamond,
        av_filledHalfRhomb,
        av_filledRhomb,
        av_smurf,
        av_arrow,
        av_groove,
        av_halfBox,
        av_halfCircle,
        av_halfDiamond,
        av_halfRhomb,
        av_hexagon,
        av_hex,                     //!< hexadecimal value
        av_id,
        av_nodeIdRef,               //!< attribute idRef of elements source, target, nodeRef, nodeStyle
        av_edgeIdRef,               //!< attribute idRef of elements edgeRef, edgeStyle
        av_labelIdRef,              //!< attribute idRef of elements edgeRef, edgeStyle
        av_sourceIdRef,             //!< attribute idRef of element endpoint
        av_targetIdRef,             //!< attribute idRef of element endpoint
        av_nodeStyleTemplateIdRef,  //!< attribute idRef of subelement template of element nodeStyle
        av_edgeStyleTemplateIdRef,  //!< attribute idRef of subelement template of element edgeStyle
        av_labelStyleTemplateIdRef, //!< attribute idRef of subelement template of element labelStyle
        av_pointIdRef,              //!< attribute idRef of subelement endpoint of element segment
        av_image,
        av_inset,
        av_int,                     //!< integer value
        av_italic,
        av_justify,
        av_left,
        av_lighter,
        av_line,
        av_lineThrough,
        av_lowercase,
        av_lParallelogram,
        av_monospace,
        av_narrower,
        av_none,
        av_normal,
        av_num,                     //!< real value
        av_oblique,
        av_oct,
        av_octagon,
        av_outset,
        av_overline,
        av_pentagon,
        av_rect,
        av_rectSimple,
        av_rhomb,
        av_ridge,
        av_right,
        av_rParallelogram,
        av_sansSerif,
        av_semiCondensed,
        av_semiExpanded,
        av_serif,
        av_slash,
        av_smallCaps,
        av_solid,
        av_bpNone,                  //!< values for node patterns
        av_bpSolid,
        av_bpDense1,
        av_bpDense2,
        av_bpDense3,
        av_bpDense4,
        av_bpDense5,
        av_bpDense6,
        av_bpDense7,
        av_bpHorizontal,
        av_bpVertical,
        av_bpCross,
        av_bpBackwardDiagonal,
        av_bpForwardDiagonal,
        av_bpDiagonalCross,
        av_string,
        av_striped,
        av_trapeze,
        av_triangle,
        av_triple,
        av_ultraCondensed,
        av_ultraExpanded,
        av_umlClass,
        av_underline,
        av_uppercase,
        av_upTrapeze,
        av_uri,
        av_wider,
        av_freeScale,               //!< image-style
        av_fixScale,                //!< image-style
        av_topLeft,                 //!< image-alignemnt
        av_topCenter,               //!< image-alignemnt
        av_topRight,                //!< image-alignemnt
        av_centerLeft,              //!< image-alignemnt
        //  av_center,              // just defined // image-alignemnt
        av_centerRight,             //!< image-alignemnt
        av_bottomLeft,              //!< image-alignemnt
        av_bottomCenter,            //!< image-alignemnt
        av_bottomRight,             //!< image-alignemnt
        // Constraint-Types:
        av_constraintAlignment,
        av_constraintAnchor,
        av_constraintSequence,

        ATT_VAL_NUM                 //!< number of attribute values
    };


    //! Stores the names of all OGML values of attributes.
    static const String s_attributeValueNames[ATT_VAL_NUM];


    //! This enumeration is used for encoding diverse validity stati of tags and attributes after parsing and validating a Xml file.
    enum ValidityState
    {
        vs_tagEmptIncl = -10,   //!< empty tag inclusion
        vs_idNotUnique = -9,    //!< id already exhausted
        vs_idRefErr = -8,       //!< referenced id wasn't found or wrong type of referenced tag
        vs_unexpTag = -7,       //!< tag unexpected
        vs_unexpAtt = -6,       //!< attribute unexpected
        vs_expTagNotFound = -5, //!< expected tag not found
        vs_expAttNotFound = -4, //!< expected attribute not found
        vs_attValueErr = -3,    //!< attribute-value error
        vs_cardErr = -2,        //!< tag/attribute cardinality error
        vs_invalid = -1,        //!< tag/attribute is invalid (no detailled information)
        vs_valid = 1            //!< tag/attribute is valid
    };


    //! This enumeration is used for indentifying graph types.
    enum GraphType
    {
        graph,
        clusterGraph,
        compoundGraph,
        corruptCompoundGraph
    };


    //! Stores the names of graph types.
    static const String s_graphTypeS[];
};

}; //namspace ogdf

#endif
