/***************************************************************************
                         ansicode.h  -  description
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


#ifndef ANSIGENERATOR_H
#define ANSIGENERATOR_H

#include <string>
#include <string_view>

#include "codegenerator.h"
#include "charcodes.h"
#include "version.h"

namespace highlight
{

/**
   \brief This class generates ANSI escape sequences.

   It contains information about the resulting document structure (document
   header and footer), the colour system, white space handling and text
   formatting attributes.

* @author Andre Simon
*/

class AnsiGenerator : public highlight::CodeGenerator
{
public:
    AnsiGenerator();
    ~AnsiGenerator() = default;

    /** prints document header
     */
    std::string getHeader() override;

    /** Prints document footer*/
    std::string getFooter() override;

    /** Prints document body*/
    void printBody() override;

private:

    /** \return escaped character*/
    std::string maskCharacter ( unsigned char ) override;


    /** \return ANSI formatting sequences */
    std::string getOpenTag ( std::string_view font,
                             std::string_view fgCol, std::string_view bgCol="" );

    /** initialize tags in specific format according to colouring information provided in DocumentStyle */
    void initOutputTags() override;

    /** @param styleID current style ID
        @return matching sequence to begin a new element formatting*/
    std::string getKeywordOpenTag ( unsigned int styleID ) override;

    /** @param styleID current style ID
        @return matching  sequence to stop element formatting*/
    std::string getKeywordCloseTag ( unsigned int styleID ) override;
};

}
#endif
