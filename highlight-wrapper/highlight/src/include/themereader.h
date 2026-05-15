/***************************************************************************
                          themereader.h  -  description
                             -------------------
    begin                : Son Nov 10 2002
    copyright            : (C) 2002-2026 by Andre Simon
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


#ifndef THEMEREADER_H
#define THEMEREADER_H

#include <map>
#include <string>
#include <string_view>

#include <Diluculum/LuaState.hpp>
#include <Diluculum/LuaFunction.hpp>
#include <Diluculum/LuaVariable.hpp>

#include "elementstyle.h"
#include "stylecolour.h"


namespace highlight
{

/** maps keyword class names and the corresponding formatting information*/
typedef std::map <std::string, ElementStyle, std::less<>> KeywordStyles;

/** \brief Contains information about document formatting properties.
 * @author Andre Simon
 */

class ThemeReader
{
private:
    ElementStyle comment, slcomment, str, dstr,
                 escapeChar, number, directive, line, operators,
                 interpolation, hover, errorMessages, errors;
    ElementStyle defaultElem;
    ElementStyle canvas;

    std::string errorMsg;
    std::string desc, categories;
    std::string themeInjections;

    std::vector<Diluculum::LuaFunction*> pluginChunks;

    std::map<std::string, int, std::less<>> semanticStyleMap;

    bool fileOK{ false }, restoreStyles{false}, dirtyAttributes{false};

    int keywordStyleCnt{0};

    OutputType outputType{HTML};

    KeywordStyles keywordStyles, originalStyles;

    void initStyle(ElementStyle& style, const Diluculum::LuaVariable& var);

    float getsRGB(int rgbValue) const;

    float getBrightness(const Colour& colour) const;

    OutputType getOutputType(std::string_view typeDesc);

public:
    /** Constructor */
    ThemeReader();
    ~ThemeReader();

    /** load style definition
          \param styleDefinitionFile Style definition path
          \param outputType
          \param loadSemanticStyles
          \return True if successful */
    bool load ( const std::string & styleDefinitionFile, OutputType outputType=HTML, bool loadSemanticStyles = false );

    void addUserChunk(const Diluculum::LuaFunction& chunk)
    {
        pluginChunks.push_back(new Diluculum::LuaFunction(chunk));
    }

    /** \return class names defined in the theme file */
    std::vector <std::string> getClassNames() const;

    /** \return keyword styles */
    KeywordStyles getKeywordStyles() const;

    /** \return Font size */
    std::string getErrorMessage() const;

    const std::string &getDescription() const
    {
        return desc;
    }

    const std::string &getCategoryDescription() const
    {
        return categories;
    }

    std::string getInjections() const;

    /** \return Background colour*/
    Colour getBgColour() const;

    /** \return Style of default (unrecognized) strings */
    ElementStyle getDefaultStyle() const;

    /** \return Comment style*/
    ElementStyle getCommentStyle() const;

    /** \return Single line comment style*/
    ElementStyle getSingleLineCommentStyle() const;

    /** \return String style*/
    ElementStyle getStringStyle() const;

    /** \return Directive line string style*/
    ElementStyle getPreProcStringStyle() const;

    /** \return Escape character style*/
    ElementStyle getEscapeCharStyle() const;

    /** \return String interpolation style*/
    ElementStyle getInterpolationStyle() const;

    /** \return Number style*/
    ElementStyle getNumberStyle() const;

    /** \return Directive style*/
    ElementStyle getPreProcessorStyle() const;

    /** \return Type style*/
    ElementStyle getTypeStyle() const;

    /** \return Line number style*/
    ElementStyle getLineStyle() const;

    /** \return Operator style*/
    ElementStyle getOperatorStyle() const;

    ElementStyle getHoverStyle() const;

    ElementStyle getErrorStyle() const;

    ElementStyle getErrorMessageStyle() const;

    /** \param className Name of keyword class (eg kwa, kwb, .., kwd)
        \return keyword style of the given className
    */
    ElementStyle getKeywordStyle ( std::string_view className ) ;

    /** \return True if language definition was found */
    bool found() const ;

    int getSemanticStyle(std::string_view type);

    int getSemanticTokenStyleCount() const;

    int getKeywordStyleCount() const;

    void overrideAttributes( std::vector<int>& attributes);

    float getContrast() const;
};

}

#endif
