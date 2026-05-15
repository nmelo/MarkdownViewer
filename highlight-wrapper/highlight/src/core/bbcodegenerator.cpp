/***************************************************************************
                    bbcodegenerator.cpp  -  description
                             -------------------
    begin                : Jul 21 2009
    copyright            : (C) 2004-2026 by Andre Simon
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



#include "bbcodegenerator.h"

using std::string;

namespace highlight
{

BBCodeGenerator::BBCodeGenerator() : CodeGenerator ( BBCODE )
{
    newLineTag = "\n";
    spacer = initialSpacer = " ";
}

string BBCodeGenerator::getHeader()
{
    return "[size=" + getBaseFontSize() + "]";
}

void BBCodeGenerator::printBody()
{
    processRootState();
}

string BBCodeGenerator::getFooter()
{
    return "[/size]";
}

string  BBCodeGenerator::getOpenTag (const ElementStyle & elem )
{
    string s = "[color=#";
    s += elem.getColour().getRed ( HTML );
    s += elem.getColour().getGreen ( HTML );
    s += elem.getColour().getBlue ( HTML );
    s += "]";

    if ( elem.isBold() ) s += "[b]";
    if ( elem.isItalic() ) s += "[i]";
    if ( elem.isUnderline() ) s += "[u]";
    return s;
}

string  BBCodeGenerator::getCloseTag ( const ElementStyle &elem )
{
    string s;
    if ( elem.isUnderline() ) s += "[/u]";
    if ( elem.isItalic() ) s += "[/i]";
    if ( elem.isBold() ) s += "[/b]";
    s += "[/color]";
    return s;
}

void BBCodeGenerator::initOutputTags ()
{
    openTags = {
        getOpenTag ( docStyle.getDefaultStyle() ),
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

    closeTags = {
        getCloseTag ( docStyle.getDefaultStyle() ),
        getCloseTag ( docStyle.getStringStyle() ),
        getCloseTag ( docStyle.getNumberStyle() ),
        getCloseTag ( docStyle.getSingleLineCommentStyle() ),
        getCloseTag ( docStyle.getCommentStyle() ),
        getCloseTag ( docStyle.getEscapeCharStyle() ),
        getCloseTag ( docStyle.getPreProcessorStyle() ),
        getCloseTag ( docStyle.getPreProcStringStyle() ),
        getCloseTag ( docStyle.getLineStyle() ),
        getCloseTag ( docStyle.getOperatorStyle() ),
        getCloseTag ( docStyle.getInterpolationStyle() ),
        getCloseTag ( docStyle.getErrorStyle() ),
        getCloseTag ( docStyle.getErrorMessageStyle() )
    };
}

string BBCodeGenerator::maskCharacter ( unsigned char c )
{
    return string ( 1, c );
}

string BBCodeGenerator::getKeywordOpenTag ( unsigned int styleID )
{
    return getOpenTag (docStyle.getKeywordStyle ( currentSyntax->getKeywordClasses() [styleID] ) );
}

string BBCodeGenerator::getKeywordCloseTag ( unsigned int styleID )
{
    return getCloseTag ( docStyle.getKeywordStyle ( currentSyntax->getKeywordClasses() [styleID] ) );
}

}
