/***************************************************************************
                          dataDir.cpp  -  description
                             -------------------
    begin                : Sam March 1 2003
    copyright            : (C) 2003-2026 by Andre Simon
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


#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include "platform_fs.h"

#include "datadir.h"

using std::cerr;
using std::cin;
using std::cout;
using std::ifstream;
using std::ios;
using std::string;
using std::string_view;


string DataDir::LSB_DATA_DIR="/usr/share/highlight/";
string DataDir::LSB_CFG_DIR="/etc/highlight/";
string DataDir::LSB_DOC_DIR="/usr/share/doc/highlight/";


void DataDir::initSearchDirectories ( const string &userDefinedDir )
{

#ifndef _WIN32
    possibleDirs.push_back ( Platform::getHomePath() + "/.highlight/" );
#endif

    if ( !userDefinedDir.empty() ) possibleDirs.push_back ( userDefinedDir );

    char* hlEnvPath=getenv("HIGHLIGHT_DATADIR");
    if (hlEnvPath!=nullptr) {
        possibleDirs.emplace_back(hlEnvPath );
    }

#ifndef _WIN32

#ifdef HL_DATA_DIR
    possibleDirs.emplace_back(HL_DATA_DIR );
#else
    possibleDirs.push_back ( LSB_DATA_DIR );
#endif

#ifdef HL_CONFIG_DIR
    possibleDirs.emplace_back(HL_CONFIG_DIR);
#else
    possibleDirs.push_back ( LSB_CFG_DIR);
#endif

#else
    possibleDirs.push_back(Platform::getAppPath()); //not needed because of fallback in searchFile
#endif
}

std::string DataDir::searchFile(std::string_view path)
{
    for (const auto& dir : possibleDirs) {
        std::string fullPath = dir;
        fullPath.append(path);
        // cerr << "Searching " << fullPath << "\n";
        if (Platform::fileExists(fullPath))
            return fullPath;
    }
    return std::string(path);
}


const void DataDir::printConfigPaths()
{
    for ( const auto& dir : possibleDirs ) {
        if ( Platform::fileExists ( dir ) )
            cout << dir <<"\n";
    }
}

const string DataDir::getLangPath ( const string & file )
{
    return searchFile(string("langDefs") + Platform::pathSeparator + file);
}

const string DataDir::getThemePath ( const string & file, bool base16)
{
    string themesPath=string("themes")+Platform::pathSeparator;

    if (base16)
        themesPath=themesPath + "base16" + Platform::pathSeparator;

    return searchFile(themesPath+file);
}

const string DataDir::getPluginPath ( const string & file)
{
    return searchFile(string("plugins")+Platform::pathSeparator+file);
}

const string DataDir::getFiletypesConfPath (const string & file)
{
    return searchFile(file + ".conf");
}

const string DataDir::getThemePath ( )
{
    return getSystemDataPath()+"themes"+Platform::pathSeparator;
}

const string DataDir::getLangPath ( )
{
    return getSystemDataPath()+"langDefs"+Platform::pathSeparator;
}

const string DataDir::getPluginPath ( )
{
    return getSystemDataPath()+"plugins"+Platform::pathSeparator;
}

const string DataDir::getSystemDataPath ( )
{
#ifndef _WIN32
#ifdef HL_DATA_DIR
    return HL_DATA_DIR;
#else
    return LSB_DATA_DIR;
#endif
#else
    return Platform::getAppPath();
#endif
}

const string  DataDir::getI18nDir()
{
    return getSystemDataPath()+"gui_files"+Platform::pathSeparator+"i18n"+Platform::pathSeparator;
}

const string  DataDir::getExtDir()
{
    return getSystemDataPath() +"gui_files"+Platform::pathSeparator+"ext"+Platform::pathSeparator;
}

const string DataDir::getDocDir()
{
    return getSystemDataPath();
}

const string DataDir::getEncodingHint (const string &syntax) {

    return encodingHint[getFileBaseName(syntax)];
}

string DataDir::getFileSuffix(const string& fileName)
{
    size_t ptPos=fileName.rfind(".");
    size_t psPos = fileName.rfind ( Platform::pathSeparator );
    if (ptPos == string::npos) {
        return  (psPos==string::npos) ? fileName : fileName.substr(psPos+1, fileName.length());
    }
    return (psPos!=string::npos && psPos>ptPos) ? "" : fileName.substr(ptPos+1, fileName.length());
}

string DataDir::guessFileType ( const string& suffix, const string &inputFile, bool useUserSuffix, bool forceShebangCheckStdin )
{
    string baseName = getFileBaseName(inputFile);
    if (assocByFilename.count(baseName)) return assocByFilename.find(baseName)->second;

    string lcSuffix = StringTools::change_case(suffix);
    if (assocByExtension.count(lcSuffix)) {
        return assocByExtension[lcSuffix];
    }

    if (!useUserSuffix) {
        string shebang =  analyzeFile(forceShebangCheckStdin ? "" : inputFile);
        if (!shebang.empty()) return shebang;
    }

    return lcSuffix;
}

bool DataDir::loadFileTypeConfig (const string& name )
{
    string confPath=searchFile(name+".conf");
    try {
        Diluculum::LuaState ls;
        Diluculum::LuaValueList ret= ls.doFile (confPath);

        int idx=1;
        string langName;
        Diluculum::LuaValue mapEntry;
        while ((mapEntry = ls["FileMapping"][idx].value()) !=Diluculum::Nil) {
            langName = mapEntry["Lang"].asString();
            if (mapEntry["Extensions"] !=Diluculum::Nil) {
                readLuaList("Extensions", langName, mapEntry,  &assocByExtension);
            } else if (mapEntry["Filenames"] !=Diluculum::Nil) {
                readLuaList("Filenames", langName, mapEntry,  &assocByFilename);
            } else if (mapEntry["Shebang"] !=Diluculum::Nil) {
                assocByShebang.insert ( make_pair ( mapEntry["Shebang"].asString(),  langName ) );
            } else if (mapEntry["EncodingHint"] !=Diluculum::Nil) {
                encodingHint.insert ( make_pair ( langName, mapEntry["EncodingHint"].asString() ) );
            }
            idx++;
        }

    } catch (Diluculum::LuaError &err) {
        cerr <<err.what()<<"\n";
        return false;
    }
    return true;
}

bool DataDir::loadLSPConfig (const string& path )
{
    string confPath=searchFile(path+".conf");
    try {
        Diluculum::LuaState ls;
        Diluculum::LuaValueList ret= ls.doFile (confPath);

        int idx=1;
        std::string serverName;              ///< server name
        std::string executable;              ///< server executable path
        std::string syntax;                  ///< language definition which can be enhanced using the LS
        int delay;                           ///< server delay in milliseconds after initialization request
        bool legacy;                         ///< do not rely on a LS capabilities response
        std::vector<std::string> options;    ///< server executable start options
        Diluculum::LuaValue mapEntry;

        //{ Server="clangd", Exec="clangd", Syntax="c", Options={"--log=error"} },
        while ((mapEntry = ls["Servers"][idx].value()) !=Diluculum::Nil) {
            options.clear();
            serverName = mapEntry["Server"].asString();
            executable = mapEntry["Exec"].asString();
            syntax = mapEntry["Syntax"].asString();
            delay = 0;
            if (mapEntry["Delay"] !=Diluculum::Nil) {
                delay = mapEntry["Delay"].asNumber();
            }

            legacy = false;
            if (mapEntry["Legacy"] !=Diluculum::Nil) {
                legacy = mapEntry["Legacy"].asBoolean();
            }

            if (mapEntry["Options"] !=Diluculum::Nil) {
                int extIdx=1;
                while (mapEntry["Options"][extIdx] !=Diluculum::Nil) {
                    options.push_back(mapEntry["Options"][extIdx].asString());
                    extIdx++;
                }
            }

            highlight::LSPProfile profile;
            profile.executable = executable;
            profile.serverName = serverName;
            profile.syntax = syntax;
            profile.options = options;
            profile.delay = delay;
            profile.legacy = legacy;

            lspProfiles[serverName]=profile;

            idx++;
        }

    } catch (Diluculum::LuaError &err) {
        cerr <<err.what()<<"\n";
        return false;
    }
    return true;
}

bool DataDir::profileExists(const string &profile) {
    return lspProfiles.count(profile);
}


highlight::LSPProfile &DataDir::getProfile(const string &profile) {
    return lspProfiles[profile];
}


void DataDir::readLuaList(const string& paramName, const string& langName,Diluculum::LuaValue &luaVal, StringMap* extMap){
    int extIdx=1;
    string val;
    while (luaVal[paramName][extIdx] !=Diluculum::Nil) {
        val = luaVal[paramName][extIdx].asString();
        extMap->insert ( make_pair ( val,  langName ) );
        extIdx++;
    }
}

string DataDir::analyzeFile ( const string& file )
{
    string firstLine;
    if ( !file.empty() ) {
        ifstream inFile ( file.c_str() );
        getline ( inFile, firstLine );
    } else {

        //  This copies all the data to a new buffer, uses the data to get the
        //  first line, then makes cin use the new buffer that underlies the
        //  stringstream instance
        cin_bufcopy << cin.rdbuf();
        getline ( cin_bufcopy, firstLine );
        cin_bufcopy.seekg ( 0, ios::beg );
        cin.rdbuf ( cin_bufcopy.rdbuf() );
    }
    StringMap::iterator it;
    boost::xpressive::sregex rex;
    boost::xpressive::smatch what;
    for ( it=assocByShebang.begin(); it!=assocByShebang.end(); it++ ) {
        rex = boost::xpressive::sregex::compile( it->first );
        if ( boost::xpressive::regex_search( firstLine, what, rex )  ) return it->second;
    }
    return "";
}

string DataDir::getFileBaseName(const string& fileName){
    size_t psPos = fileName.rfind ( /*Platform::pathSeparator*/ '/' );
    return  (psPos==string::npos) ? fileName : fileName.substr(psPos+1, fileName.length());
}
