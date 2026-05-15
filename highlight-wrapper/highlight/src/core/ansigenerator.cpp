/***************************************************************************
                    ansigenerator.cpp  -  description
                             -------------------
    begin                : Jul 5 2004
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


#include <string_view>

#include "ansigenerator.h"

using std::string;
using std::string_view;

namespace highlight
{

string  AnsiGenerator::getOpenTag ( string_view font,
                                    string_view fgCol, string_view bgCol )
{
    string s = "\033[";
    s += font;
    if ( !fgCol.empty() ) {
        s += ";";
        s += fgCol;
    }
    if ( !bgCol.empty() ) {
        s += ";";
        s += bgCol;
    }
    s += "m";
    return s;
}


AnsiGenerator::AnsiGenerator() : CodeGenerator ( ESC_ANSI )
{
    newLineTag = "\n";
    spacer = initialSpacer = " ";
}

void AnsiGenerator::initOutputTags()
{
    openTags = {
        getOpenTag ( "00", "39" ),
        getOpenTag ( "00", "31" ), //str
        getOpenTag ( "00", "34" ), //number
        getOpenTag ( "00", "34" ), //sl comment
        getOpenTag ( "00", "34" ), //ml comment
        getOpenTag ( "00", "35" ), //escapeChar
        getOpenTag ( "00", "35" ), //directive
        getOpenTag ( "00", "31" ), //directive string
        getOpenTag ( "00", "39" ), //linenum
        getOpenTag ( "00", "39" ), //symbol
        getOpenTag ( "00", "35" ), //interpolation
        getOpenTag ( "01", "31" ), //error
        getOpenTag ( "01", "31" )  //warning
    };

    closeTags.assign ( NUMBER_BUILTIN_STATES, "\033[m" );
    closeTags[0] = "";
}

string AnsiGenerator::getHeader()
{
    return string();
}

void AnsiGenerator::printBody()
{
    processRootState();
}

string AnsiGenerator::getFooter()
{
    return string();
}

string AnsiGenerator::maskCharacter ( unsigned char c )
{
    return string ( 1, c );
}

string AnsiGenerator::getKeywordOpenTag ( unsigned int styleID )
{
    return ( styleID ) ?getOpenTag ( "00", "32", "" ) :getOpenTag ( "00", "33" );
}

string AnsiGenerator::getKeywordCloseTag ( unsigned int styleID )
{
    return "\033[m";
}

}
