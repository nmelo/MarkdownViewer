/***************************************************************************
                         bbcodegenerator.h  -  description
                             -------------------
    begin                : Jul 20 2009
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


#ifndef BBCODEGENERATOR_H
#define BBCODEGENERATOR_H

#include <string>

#include "codegenerator.h"
#include "charcodes.h"
#include "version.h"

namespace highlight
{

/**
   \brief This class generates BBCode.

   It contains information about the resulting document structure (document
   header and footer), the colour system, white space handling and text
   formatting attributes.

* @author Andre Simon
*/

class BBCodeGenerator : public highlight::CodeGenerator
{
public:
    BBCodeGenerator();
    ~BBCodeGenerator() = default;

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

    /** @return BBcode open tags */
    std::string getOpenTag (const ElementStyle & elem );

    /** @return BBcode close tags */
    std::string  getCloseTag ( const ElementStyle &elem );

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
