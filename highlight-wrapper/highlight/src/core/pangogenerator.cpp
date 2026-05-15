/***************************************************************************
                   PangoGenerator.cpp  -  description
                             -------------------
    begin                : Sept 5 2014
    copyright            : (C) 2014 by Dominik Schmidt
    email                : a.simon@mailbox.org
 ***************************************************************************/


/*
This file is part of Highlight.

Highlight is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Highlight is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Highlight.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <sstream>

#include "pangogenerator.h"

using std::ostringstream;
using std::string;

namespace highlight
{

string PangoGenerator::getOpenTag ( const ElementStyle & elem )
{
    return "<span "+getAttributes ( elem ) + ">";
}

string PangoGenerator::getAttributes ( const ElementStyle & elem )
{
    ostringstream s;

    if (!elem.getCustomOverride()) {
        s << "foreground=\"#"
        << ( elem.getColour().getRed ( HTML ) )
        << ( elem.getColour().getGreen ( HTML ) )
        << ( elem.getColour().getBlue ( HTML ) )
        << "\""
        << ( elem.isBold() ? " weight=\"bold\"" :"" )
        << ( elem.isItalic() ? " style=\"italic\"" :"" )
        << ( elem.isUnderline() ? " underline=\"single\"" :"" );
    }

    if (string customStyle = elem.getCustomAttribute(); !customStyle.empty()) {
        if (!elem.getCustomOverride()) {
            s << " ";
        }
        s << customStyle;
    }

    return s.str();
}

PangoGenerator::PangoGenerator() : CodeGenerator ( PANGO )
{

    newLineTag = "\n";
    spacer = initialSpacer = " ";
}

void PangoGenerator::initOutputTags()
{
    openTags = {
        "",
        getOpenTag ( docStyle.getStringStyle() ),
        getOpenTag ( docStyle.getNumberStyle() ),
        getOpenTag ( docStyle.getSingleLineCommentStyle() ),
        getOpenTag ( docStyle.getCommentStyle() ),
        getOpenTag ( docStyle.getEscapeCharStyle() ),
        getOpenTag ( docStyle.getPreProcessorStyle() ),
        getOpenTag ( docStyle.getPreProcStringStyle() ),
        getOpenTag ( docStyle.getLineStyle() ),
        getOpenTag ( docStyle.getOperatorStyle() ),
        getOpenTag ( docStyle.getInterpolationStyle() ),
        getOpenTag ( docStyle.getErrorStyle() ),
        getOpenTag ( docStyle.getErrorMessageStyle() )
    };

    closeTags.assign ( NUMBER_BUILTIN_STATES, "</span>" );
    closeTags[0] = "";
}

string PangoGenerator::getHeader()
{
    return string();
}

void PangoGenerator::printBody()
{
    int fontSize=0;
    StringTools::str2num<int> ( fontSize, this->getBaseFontSize(), std::dec );

    *out << "<span size=\""<<( ( fontSize ) ? fontSize*1024: 10*1024 ) << "\" "
         << "font_family=\"" << getBaseFont() << "\""
         <<">";
    processRootState();
    *out << "</span>";
}

string PangoGenerator::getFooter()
{
    return string();
}

string PangoGenerator::maskCharacter ( unsigned char c )
{
    switch ( c ) {
    case '<' :
        return "&lt;";
    case '>' :
        return "&gt;";
    case '&' :
        return "&amp;";
    default :
        return string ( 1, c );
    }
}

string PangoGenerator::getKeywordOpenTag ( unsigned int styleID )
{
    return getOpenTag (docStyle.getKeywordStyle ( currentSyntax->getKeywordClasses() [styleID] ) );
}

string PangoGenerator::getKeywordCloseTag ( unsigned int styleID )
{
    return "</span>";
}

}
