/***************************************************************************
                          SVGGenerator.h  -  description
                             -------------------
    begin                : Mo 23.06.2008
    copyright            : (C) 2008-2026 by Andre Simon
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


#ifndef SVGGenerator_H
#define SVGGenerator_H

#include "codegenerator.h"

namespace highlight
{

/**
   \brief This class generates SVG.

   It contains information about the resulting document structure (document
   header and footer), the colour system, white space handling and text
   formatting attributes.

* @author Andre Simon
*/

class SVGGenerator : public highlight::CodeGenerator
{
public:

    SVGGenerator();
    ~SVGGenerator() = default;

    /** Set SVG dimensions
        \param w page width
        \param h page height
    */
    void setSVGSize ( const std::string& w, const std::string& h );

private:

    /** prints document header
     */
    std::string getHeader();

    /** Prints document footer*/
    std::string getFooter();

    /** Prints document body*/
    void printBody();

    /** initialize tags in specific format according to colouring information provided in DocumentStyle */
    void initOutputTags();

    std::string styleDefinitionCache;
    std::string width, height;

    std::string getStyleDefinition();

    std::string getAttributes ( const std::string &, const ElementStyle & );

    /** \return escaped character*/
    virtual std::string maskCharacter ( unsigned char );

    std::string getOpenTag ( const std::string& );

    std::string getKeywordOpenTag ( unsigned int styleID );
    std::string getKeywordCloseTag ( unsigned int styleID );

    /** @return Newline string */
    std::string getNewLine();
};

}

#endif
