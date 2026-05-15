/***************************************************************************
                          syntaxreader.h  -  description
                             -------------------
    begin                : Wed Nov 28 2001
    copyright            : (C) 2001-2026 by Andre Simon
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


#ifndef SYNTAXREADER_H
#define SYNTAXREADER_H

#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>

#include <Diluculum/LuaState.hpp>
#include <Diluculum/LuaVariable.hpp>
#include <Diluculum/LuaFunction.hpp>

#include "regexelement.h"
#include "platform_fs.h"
#include "enums.h"

#define GLOBAL_SR_INSTANCE_NAME "HL_SRInstance"

namespace highlight
{
class RegexElement;

/** maps keywords and the corresponding class IDs*/
typedef std::map <std::string, int, std::less<>> KeywordMap;

/** maps embedded language names to exit delimiter regexes*/
typedef std::map <std::string, std::string, std::less<>> DelimiterMap;

typedef std::map <std::string, bool, std::less<>> AllowInnerSectionsMap;


/**\brief Contains specific data of the programming language being processed.

* @author Andre  Simon
*/
class SyntaxReader
{

public:

    SyntaxReader();

    ~SyntaxReader();


    /** Load new language definition
        Will only read a new language definition if the given
          file path is not equal to the path of the current language definition.
        \param langDefPath Path of language definition
        \param pluginReadFilePath path to file which is read by plugin
        \param outputType output format
        \return LoadResult  */
    LoadResult load( const std::string& langDefPath, const std::string& pluginReadFilePath,  OutputType outputType );

    /** \return True if the next load() call would load a new language definition
        \param  langDefPath Path to language definition  */
    bool needsReload ( std::string_view langDefPath ) const
    {
        return currentPath!=langDefPath;
    }

    /** \return Failed regular expression */
    std::string getFailedRegex() const
    {
        return regexErrorMsg;
    }

    /** \return Failed Lua exception description */
    std::string getLuaErrorText() const
    {
        return luaErrorMsg;
    }

    /** \return Prefix of raw strings */
    unsigned char getRawStringPrefix() const
    {
        return rawStringPrefix;
    }

    /** \return Continuation Character */
    unsigned char getContinuationChar() const
    {
        return continuationChar;
    }

    /** \return true if syntax highlighting is enabled*/
    bool highlightingEnabled() const
    {
        return !disableHighlighting;
    }

    /** \return True if language is case sensitive */
    bool isIgnoreCase() const
    {
        return ignoreCase;
    }

    /** \param s String
         \return true if s is not a known keyword */
    bool isKeyword ( std::string_view s ) ;

    /** \param s String
         \return keyword list group id */
    int getKeywordListGroup ( std::string_view s );

    /** \return True if multi line comments may be nested */
    bool allowNestedMLComments() const
    {
        return allowNestedComments;
    }

    /** \return True if highlighting is disabled
                            TODO remove method */
    bool highlightingDisabled() const
    {
        return disableHighlighting;
    }

    /** \return True if current language may be reformatted (c, c++, c#, java) */
    bool enableReformatting() const
    {
        return reformatCode;
    }

    bool assertDelimEqualLength() const
    {
      return assertEqualLength;
    }

    /** \return keywords*/
    const KeywordMap& getKeywords() const
    {
        return keywords;
    }

    /** \return keyword classes*/
    const std::vector<std::string>& getKeywordClasses() const
    {
        return keywordClasses;
    }

    /** \return regular expressions */
    const std::vector<RegexElement*>& getRegexElements() const
    {
        return regex;
    }

    /** \return list of Lua code snippets to be stored on disk */
    const std::vector<std::string>& getPersistentSnippets() const
    {
        return persistentSnippets;
    }

    /** \return number of Lua code snippets to be stored on disk */
    int getPersistentSnippetsNum() const
    {
        return persistentSnippets.size();
    }

    /** \return list of format override flags defined in syntax definitions */
    std::vector<int>& getOverrideStyleAttributes()
    {
        return overrideStyles;
    }

    /** \return description of the programming language */
    const std::string & getDescription () const
    {
        return langDesc;
    }

    const std::string & getCategoryDescription() const
    {
        return categories;
    }


    /** \return header string defined by a plug-in */
    const std::string & getHeaderInjection () const
    {
        return headerInjection;
    }

    /** \return footer string defined by a plug-in */
    const std::string & getFooterInjection () const
    {
        return footerInjection;
    }

    /**  \param delimID delimiter id
         \return true,  if no closing delimiter exists (open and close delimiters are equal)
     */
    bool delimiterIsDistinct ( int delimID )
    {
        return delimiterDistinct[delimID];
    }

    /**  \param delimID delimiter id
         \return true,  if delimiter indicates a raw string
     */
    bool delimiterIsRawString ( int delimID )
    {
        return rawStringOpenDelims[delimID];
    }

    /**  Pairs of open/close delimiters have a unique ID to test if two tokens act as delimiters
         \param token delimiter token
                     \param s State of delimiter
         \return delimiter ID
     */
    int getOpenDelimiterID ( const std::string& token, State s);

    /**  Pairs of open/close delimiters have a unique ID to test if two tokens act as delimiters
         \param token delimiter token
                     \param s State of delimiter
                     \param openDelimId opening delimiter retrieved with getOpenDelimiterID
         \return true if delimiter id of token matches openDelimID
     */
    bool matchesOpenDelimiter ( const std::string& token, State s, int openDelimId);

    /** initializes end delimiter regex to switch back to host language
    	\param langPath path of embedded language definition
    */
    void restoreLangEndDelim(std::string_view langPath);

    bool allowsInnerSection(std::string_view langPath);

    bool requiresTwoPassRun();

    bool requiresParamUpdate();


    std::string getPersistentHookConditions();

    void clearPersistentSnippets();

    /**
    	\param lang language definition name  (no path, no ".lang" extension)
    	\return absolute path based on the previously loaded definition
    */
    std::string getNewPath(const std::string& lang);

    /**
    	\return absolute path of currently loaded definition
    */
    std::string getCurrentPath() const
    {
        return currentPath;
    }

    /**
    	\return encoding which is normally used for input files of this syntax
    */
    std::string getEncodingHint() const
    {
        return encodingHint;
    }

     /**
    	\return test function
    */
    std::string getOverrideConfigVal(std::string_view name) const
    {
        if (auto it = pluginConfigOverride.find(name); it != pluginConfigOverride.end()) {
            return it->second;
        }
        return "";
    }

    /**
    	\return pointer to state validation function
    */
    Diluculum::LuaFunction* getValidateStateChangeFct() const
    {
        return validateStateChangeFct;
    }
    /**
    	\return pointer to state decorate function
    */
    Diluculum::LuaFunction* getDecorateFct() const
    {
        return decorateFct;
    }

    /**
    	\return pointer to line begin decorate function
    */
    Diluculum::LuaFunction* getDecorateLineBeginFct() const
    {
        return decorateLineBeginFct;
    }

    /**
    	\return pointer to line end decorate function
    */
    Diluculum::LuaFunction* getDecorateLineEndFct() const
    {
        return decorateLineEndFct;
    }

    /**
    	\return pointer to Lua state
    */
    Diluculum::LuaState* getLuaState() const
    {
        return luaState;
    }

    /**
    	\param chunk Lua function to be added to the function list
    */
    void addUserChunk(const Diluculum::LuaFunction& chunk)
    {
        pluginChunks.push_back(new Diluculum::LuaFunction(chunk));
    }

    /**
    	\param fn name of the processed input file
    */
    void setInputFileName(const std::string& fn) { currentInputFile=fn; }

    /**
    	\return name of the processed input file
    */

    std::string getInputFileName() const { return currentInputFile; }

    /**
    	\param groupID keyword group to be stored on disk
    	\param kw keyword token to be stored on disk
    */

    void addPersistentKeyword(unsigned int groupID, std::string_view kw);

    /**
    	\param groupID keyword group to be stored on disk
    	\param column start of range within line
    	\param length length of range
    	\param lineNumber line number
    	\param fileName file name of processed file containing the line
    	*/
    void addPersistentStateRange(unsigned int groupID, unsigned int column,unsigned int length, unsigned int lineNumber, std::string_view fileName);

    /**
        \param ls Lua state to be initialized with constants
        \param langDefPath absolute path of language definition
        \param pluginReadFilePath absolute path of plugin input file
    */
    static void initLuaState(Diluculum::LuaState& ls, const std::string& langDefPath, const std::string& pluginReadFilePath, OutputType outputType=HTML );

    // generate a keyword class
    unsigned int generateNewKWClass ( int classID, const char *prefix="kw" );

    int getKeywordCount() const;

private:

    static const std::string REGEX_IDENTIFIER;
    static const std::string REGEX_NUMBER;
    static const std::string REGEX_ESCSEQ;

    // path to loaded language definition
    std::string currentPath;

    // name of file being processed
    std::string currentInputFile;

    // Language description
    std::string langDesc, categories, encodingHint;

    std::string headerInjection, footerInjection;

    std::string regexErrorMsg, luaErrorMsg;

    KeywordMap keywords;

    std::vector <std::string> keywordClasses;
    static std::vector <std::string> persistentSnippets;
    static std::set <std::string> persistentSyntaxDescriptions;

    std::vector <RegexElement*> regex;

    std::vector <int>overrideStyles;

    // collect delimiters or get current delimiter in CodeGenerator::loadEmbeddedLang
    static DelimiterMap nestedStateEndDelimiters;

    static DelimiterMap pluginConfigOverride;

    static AllowInnerSectionsMap allowInnerSections;

    // saves if delimiter pair consists of the same delimiter symbol
    std::map <int, bool> delimiterDistinct;

    std::map <int, bool> rawStringOpenDelims;

    std::map <int, int> matchingDelimiters;

    // Caution: SWIG does not support member default initializers

    // keywords are not case sensitive if set
    bool ignoreCase,

         // highlighting is disabled
         disableHighlighting,

         // allow nested multi line comment blocks
         allowNestedComments,

         // code formatting is enabled if set
         reformatCode,

         // string open and close delimiters must have the same length
         assertEqualLength,

         paramsNeedUpdate;

    // character which is prefix of raw string (c#)
    unsigned char rawStringPrefix;

    //character which continues current style on next line
    unsigned char continuationChar;

    int keywordCount;

    bool readFlag(const Diluculum::LuaVariable& var) ;

    // interface for plug-ins: add keywords dynamically
    static int luaAddKeyword (lua_State *L);

    // interface for plug-ins: remove keywords dynamically
    static int luaRemoveKeyword (lua_State *L);

    static int luaAddPersistentState (lua_State *L);

    static int luaOverrideParam (lua_State *L);



    void addKeyword(unsigned int groupID, std::string_view kw);

    void removeKeyword(std::string_view kw);

    void overrideParam(std::string_view name, std::string_view val);


    // Functions accessible in Lua State
    Diluculum::LuaFunction* validateStateChangeFct;
    Diluculum::LuaFunction* decorateFct, *decorateLineBeginFct, *decorateLineEndFct;

    Diluculum::LuaState* luaState; // make member to allow interaction with codeparser instance

    static std::vector<Diluculum::LuaFunction*> pluginChunks;
};

}
#endif
