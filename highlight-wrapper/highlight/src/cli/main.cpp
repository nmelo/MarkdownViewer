/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Die Apr 23 22:16:35 CEST 2002
    copyright            : (C) 2002-2026 by Andre Simon
    email                : a.simon@mailbox.org

   Highlight is a universal source code to formatted text converter.
   Syntax highlighting is formatted by Cascading Style Sheets.
   It's possible to easily enhance highlight's parsing database.

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

#include <memory>
#include <algorithm>
#include <Diluculum/LuaState.hpp>
#include <astyle/astyle.h>

#include "main.h"
#include "datadir.h"
#include "syntaxreader.h"
#include "lspprofile.h"

using std::cerr;
using std::cin;
using std::endl;
using std::flush;
using std::ifstream;
using std::istringstream;
using std::ios;
using std::map;
using std::ostream_iterator;
using std::setiosflags;
using std::setw;
using std::string;
using std::unique_ptr;
using std::vector;

void HLCmdLineApp::printVersionInfo(bool quietMode)
{
    if (quietMode) {
        std::cout << highlight::Info::getVersion() << "\n";
    } else {
        std::cout << "\n highlight version "
            << highlight::Info::getVersion()
            << "\n Copyright (C) 2002-2026 Andre Simon <a dot simon at mailbox.org>"
            << "\n\n Argparser class"
            << "\n Copyright (C) 2006-2008 Antonio Diaz Diaz <ant_diaz at teleline.es>"
            << "\n\n Artistic Style Classes (" << ASTYLE_VERSION << ")"
            << "\n Copyright (C) 1998-2026 The Artistic Style Authors"
            << "\n\n Diluculum Lua wrapper (1.0)"
            << "\n Copyright (C) 2005-2013 by Leandro Motta Barros"
            << "\n\n xterm 256 color matching functions"
            << "\n Copyright (C) 2006 Wolfgang Frisch <wf at frexx.de>"
            << "\n\n PicoJSON library"
            << "\n Copyright (C) 2009-2010 Cybozu Labs, Inc."
            << "\n Copyright (C) 2011-2014 Kazuho Oku"
            << "\n\n This software is released under the terms of the GNU General "
            << "Public License."
            << "\n For more information about these matters, see the file named "
            << "COPYING.\n\n";
    }
}

void HLCmdLineApp::printBadInstallationInfo()
{
    std::cerr << "highlight: Data directory not found ("<<DataDir::LSB_DATA_DIR<<")."
         " Bad installation or wrong "<< OPT_DATADIR << " parameter."
         << "\n\nCopy the highlight files into one of the directories listed "
         << "in INSTALL.\nYou may also set the data directory with "
         << OPT_DATADIR << ".\n";
}

int HLCmdLineApp::printInstalledFiles(const std::string& where, const std::string& wildcard, const std::string& kind, const std::string& option, const std::string& categoryFilterList)
{
    std::vector <string> filePaths;
    std::string searchDir = where + wildcard;

    bool directoryOK = Platform::getDirectoryEntries ( filePaths, searchDir, true );
    if ( !directoryOK ) {
        std::cerr << "highlight: Could not access directory "
             <<  searchDir
             << ", aborted.\n";
        return EXIT_FAILURE;
    }

    std::sort ( filePaths.begin(), filePaths.end() );
    std::string suffix, desc;
    Diluculum::LuaValueMap categoryMap;
    std::cout << "\nInstalled " << kind << "s";

    if (categoryFilterList.size())
        std::cout << " matching \""<<categoryFilterList<<"\"";

    std::cout << " (located in " << where << "):\n\n";
    int matchedFileCnt=0;
    std::set<string> categoryNames;
    std::set<string> categoryFilters;

    std::istringstream valueStream;
    std::string catFilter;
    valueStream.str ( StringTools::change_case ( categoryFilterList, StringTools::CASE_LOWER ) );
    while ( std::getline ( valueStream, catFilter, ';' ) ) {
        categoryFilters.insert ( catFilter );
    }

    for (const auto &path : filePaths) {
        try {
            Diluculum::LuaState ls;
            highlight::SyntaxReader::initLuaState(ls, path,"");
            ls.doFile(path);
            desc = ls["Description"].value().asString();

            suffix = ( path ).substr ( where.length() ) ;
            suffix = suffix.substr ( 0, suffix.length()- wildcard.length()+1 );

            unsigned int filterOKCnt=categoryFilters.size();
            if (ls["Categories"].value() !=Diluculum::Nil){
                filterOKCnt=0;

                categoryMap = ls["Categories"].value().asTable();

                for (const auto &[first, second] : categoryMap)
                {
                    categoryNames.insert(second.asString());
                    if (categoryFilters.size() && categoryFilters.count(second.asString())) {
                        ++filterOKCnt;
                    }
                }
            }

            if (filterOKCnt!=categoryFilters.size() && categoryFilters.size() ) continue;

            matchedFileCnt++;
            if (kind=="langDef") {
                std::cout << setw ( 30 ) <<setiosflags ( ios::left ) <<desc<<": "<<suffix;

                int extCnt=0;
                for (const auto &[first, second] : dataDir.assocByExtension) {
                    if (second==suffix ) {
                        std::cout << ((++extCnt==1)?" ( ":" ") << first;
                    }
                }
                std::cout << ((extCnt)?" )":"");
            } else {
                std::cout << setw ( 30 ) <<setiosflags ( ios::left ) <<suffix<<": "<<desc;

            }
            std::cout << endl;
        } catch (std::runtime_error &error) {
            std::cout << "Failed to read '" << path << "': " << error.what() << endl;
        }
    }

    if (!matchedFileCnt) {
        std::cout <<"No files found." << endl;
    } else {

        if (!categoryFilters.size()){
            std::cout << "\nFound "<<kind<<" categories:\n\n";
            for ( const auto &catName : categoryNames)
                std::cout << catName << ' ';
            std::cout << "\n";
        }

        std::cout <<"\nUse name of the desired "<<kind
            << " with --" << option << ". Filter categories with --list-cat." << endl;

        if (kind=="theme") {
            std::cout <<"\nAdd base16/ prefix to apply a Base16 theme." << endl;
        }

        printConfigInfo();
    }

    return EXIT_SUCCESS;
}

void HLCmdLineApp::printDebugInfo ( const highlight::SyntaxReader *lang,
                                    const string & langDefPath, int level )
{
    if (!lang) return;

    map <int, string> HLStateMap;

    cerr << "\nLoading language definition:\n" << langDefPath;
    cerr << "\n\nDescription: " << lang->getDescription();
    if (level>1) {

        if (auto luaState = lang->getLuaState(); luaState) {
            cerr << "\n\nLUA GLOBALS:\n" ;
            auto glob = luaState->globals();

            for (const auto &[first, second] : glob) {
                auto elemName = first.asString();
                std::cerr << elemName << ": ";
                switch (second.type()) {
                    case  LUA_TSTRING:
                        cerr << "string [ "<<second.asString()<<" ]";
                        break;
                    case  LUA_TNUMBER:
                        cerr << "number [ "<<second.asNumber()<<" ]";
                        if (elemName.find("HL_")==0 && elemName.find("HL_FORMAT")==string::npos)
                            HLStateMap[second.asNumber()] = elemName;
                        break;
                    case  LUA_TBOOLEAN:
                        cerr << "boolean [ "<<second.asBoolean()<<" ]";
                        break;
                    default:
                        cerr << second.typeName();
                }
                cerr << endl;
            }
        }

        if (auto regexElements = lang->getRegexElements(); !regexElements.empty()) {
            std::cerr << "\nREGEX:\n";
            for (const auto *re : regexElements)
                std::cerr << "State " << re->open << " (" << HLStateMap[re->open] << "):\t" << re->pattern << "\n";
        }

        if (auto keys = lang->getKeywords(); !keys.empty()) {
            std::cerr << "\nKEYWORDS:\n";
            for (const auto &[keyword, value] : keys)
                std::cerr << " " << keyword << "(" << value << ")";
        }
    }
    cerr <<"\n\n";
}

void HLCmdLineApp::printConfigInfo ( )
{
    std::cout << "\nConfig file search directories:\n";
    dataDir.printConfigPaths();
    std::cout << "\nFiletype config file:\n"<<dataDir.getFiletypesConfPath ( "filetypes" ) <<"\n";
    std::cout << endl;
#ifdef HL_DATA_DIR
    std::cout << "Compiler directive HL_DATA_DIR = " <<HL_DATA_DIR<< "\n";
#endif
#ifdef HL_CONFIG_DIR
    std::cout << "Compiler directive HL_CONFIG_DIR = " <<HL_CONFIG_DIR<< "\n";
#endif
    std::cout << endl;
}

int HLCmdLineApp::getNumDigits ( int i )
{
    int res=0;
    while ( i ) {
        i/=10;
        ++res;
    }
    return res;
}

void HLCmdLineApp::printProgressBar ( int total, int count )
{
    if ( !total ) return;
    int p=100*count / total;
    int numProgressItems=p/10;
    std::cout << "\r[";
    for ( int i=0; i<10; i++ ) {
        std::cout << ( ( i<numProgressItems ) ?"#":" " );
    }
    std::cout<< "] " <<setw ( 3 ) <<p<<"%, "<<count << " / " << total << "  " <<flush;
    if ( p==100 ) {
        std::cout << endl;
    }
}

void HLCmdLineApp::printCurrentAction ( const string&outfilePath,
                                        int total, int count, int countWidth )
{
    std::cout << "Writing file "
         << setw ( countWidth ) << count
         << " of "
         << total
         << ": "
         << outfilePath
         << "\n";
}

void HLCmdLineApp::printIOErrorReport ( unsigned int numberErrorFiles,
                                        vector<string> & fileList,
                                        const string &action, const string &streamName  )
{
    cerr << "highlight: Could not "
         << action
         << " file"
         << ( ( numberErrorFiles>1 ) ?"s":"" ) <<":\n";

    if (numberErrorFiles==1 && fileList[0].size()==0){
        cerr<<streamName<<"\n";
    }
    else {
        copy ( fileList.begin(), fileList.end(), ostream_iterator<string> ( cerr, "\n" ) );
    }

    if ( fileList.size() < numberErrorFiles ) {
        cerr << "... ["
             << ( numberErrorFiles - fileList.size() )
             << " of "
             << numberErrorFiles
             << " failures not shown, use --"
             << OPT_VERBOSE
             << " switch to print all failures]\n";
    }
}

vector <string> HLCmdLineApp::collectPluginPaths(const vector<string>& plugins)
{
    vector<string> absolutePaths;
    for (const auto& plugin : plugins) {
        if (Platform::fileExists(plugin)) {
            absolutePaths.push_back(plugin);
        } else {
            absolutePaths.push_back(dataDir.getPluginPath(plugin + ".lua"));
        }
    }
    return absolutePaths;
}

bool HLCmdLineApp::serviceModeCheck(CmdLineOptions &options, highlight::CodeGenerator* generator, string &suffix, unsigned int &curFileIndex){
    if (! options.runServiceMode())
        return false;

    cerr << flush;
    if (! service_mode_tag.empty())
        std::cout << service_mode_tag << endl;

    std::cout << flush;

    if (cin.eof())
        return false;

    curFileIndex = 0; //stay on our stdin

    if (cin.peek() == generator->getAdditionalEOFChar()) {
        cin.get(); //eof char
        if (cin.peek() == '\n') //this should always be true if they properly give input but if they separated files with just EOF marker we won't chomp their first char
            cin.get(); //eol after it
    }

    string modeChanges;
    getline(cin, modeChanges);
    if (modeChanges == "exit"){
        cin.setstate(std::ios_base::eofbit);
        return false;
    }

    istringstream modesStream(modeChanges);
    string modeChange;
    string modeRes;
    string modeKey;
    string modeVal;

    while (getline(modesStream, modeChange, ';')) {

        istringstream modeStream(modeChange);

        getline(modeStream, modeKey, '=');
        getline(modeStream, modeVal);

        if (modeKey == "syntax") {
            if (modeVal.find(".") != string::npos)
                suffix = dataDir.guessFileType(dataDir.getFileSuffix(modeVal), modeVal);
            else
                suffix = modeVal;
            modeRes += "Syntax: " + suffix + " ";
        } else if (modeKey == "line-length"){
            int lineLength=0;

            StringTools::str2num<int> ( lineLength, modeVal, std::dec );

            options.setLineLength(lineLength);
            modeRes += "LineLen: " + modeVal + " ";

            generator->setPreformatting(options.getWrappingStyle(),
                (generator->getPrintLineNumbers()) ?
                options.getLineLength() - options.getNumberWidth() : options.getLineLength(),
                options.getNumberSpaces());
        } else if (modeKey == "eof") {
            auto eof = modeVal.size() && (unsigned char)modeVal[0];
            generator->setAdditionalEOFChar(eof);
            modeRes += "eof char val: " + std::to_string(eof) + " ";
        } else if (modeKey == "tag") {
            service_mode_tag = modeVal;
            modeRes += "Tag: " + service_mode_tag + " ";
        } else if (modeKey.length() )
            cerr << "Invalid service mode key change of: " << modeKey << " ignoring." << endl;
    }
    if (options.verbosityLevel())
        cerr << "Updated Items: " << modeRes << endl;

    return true;
}
int HLCmdLineApp::run ( const int argc, const char*argv[] )
{
    CmdLineOptions options ( argc, argv );

    // set data directory path, where /langDefs and /themes reside
    string dataDirPath = ( options.getDataDir().empty() ) ?  Platform::getAppPath() :options.getDataDir();

    if ( options.printVersion() ) {
        printVersionInfo(options.quietMode());
        return EXIT_SUCCESS;
    }

    dataDir.initSearchDirectories ( dataDirPath );

    if ( options.printHelp() ) {
        Help::printHelp(options.getHelpTopic());
        return EXIT_SUCCESS;
    }

    if ( options.printConfigInfo() ) {
        printConfigInfo();
        return EXIT_SUCCESS;
    }

#ifdef _WIN32

// available in recent Windows SDK
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

    //https://docs.microsoft.com/en-us/windows/console/setconsolemode
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode;
    GetConsoleMode(hOutput, &dwMode);
    dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOutput, dwMode);
    if ( options.disableEcho() ) {
        HANDLE hConsole = { GetStdHandle(STD_INPUT_HANDLE) };
        DWORD consoleMode{};
        GetConsoleMode(hConsole, &consoleMode);
        consoleMode &= ~ENABLE_ECHO_INPUT;
        SetConsoleMode(hConsole, consoleMode);
    }
#endif

    //call before printInstalledLanguages!
    dataDir.loadFileTypeConfig ( "filetypes" );

    // set CLI options; if profle is defined read from lsp.conf
    std::string lsProfile(options.getLsProfile());
    std::string lsExecutable(options.getLsExecutable());         ///< server executable path
    std::string lsSyntax(options.getLsSyntax());                   ///< language definition which can be enhanced using the LS
    int lsDelay=options.getLsDelay();
    bool lsLegacy=options.isLsLegacy();
    std::vector<std::string> lsOptions = options.getLSOptions(); ///< server executable start options

    if (lsProfile.size()) {
        dataDir.loadLSPConfig("lsp");
        if (dataDir.profileExists(lsProfile)) {
            highlight::LSPProfile profile = dataDir.getProfile(lsProfile);
            if (lsExecutable.empty())
                lsExecutable = profile.executable;
            if (lsSyntax.empty())
                lsSyntax = profile.syntax;
            if (lsOptions.empty())
                lsOptions = profile.options;
            if (lsDelay==0)
                lsDelay = profile.delay;
            if (lsLegacy==false)
                lsLegacy = profile.legacy;
        } else {
            cerr << "highlight: Unknown LSP profile '"<< lsProfile << "'.\n";
            return EXIT_FAILURE;
        }
    }

    string scriptKind=options.getListScriptKind();
    if (scriptKind.length()) {
        if ( scriptKind.find("theme")==0 ) {
            return printInstalledFiles(dataDir.getThemePath(""), "*.theme", "theme", OPT_STYLE, options.getCategories());
        }
        else if ( scriptKind.find("plug")==0 ) {
            return printInstalledFiles(dataDir.getPluginPath(""), "*.lua", "plug-in", OPT_PLUGIN, options.getCategories());
        }
        else if (  scriptKind.find("lang")==0 ) {
            return printInstalledFiles(dataDir.getLangPath(""), "*.lang", "langDef", OPT_SYNTAX, options.getCategories());
        } else {
            cerr << "highlight: Unknown script type '"<< scriptKind << "'. Apply one of 'themes', 'langs' or 'plug-ins'.\n";
            return EXIT_FAILURE;
        }
    }

    const vector <string> inFileList=options.getInputFileNames();

    if ( options.enableBatchMode() && inFileList[0].empty() ) {
        return EXIT_FAILURE;
    }

    string themePath=options.getAbsThemePath().empty() ? dataDir.getThemePath ( options.getThemeName(), options.useBase16Theme() ): options.getAbsThemePath();

    unique_ptr<highlight::CodeGenerator> generator ( highlight::CodeGenerator::getInstance ( options.getOutputType() ) );

    if (options.checkSyntaxSupport()) {

        if (!options.syntaxGiven() ) {
            cerr << "highlight: Define a syntax to use this option\n";
            return EXIT_FAILURE;
        } else {
            string syntaxByFile=options.getSyntaxByFilename();
            string testSuffix = syntaxByFile.empty() ? options.getSyntax() : dataDir.getFileSuffix(syntaxByFile);
            string resolvedSuffix (dataDir.guessFileType (testSuffix, syntaxByFile, syntaxByFile.empty(),false ));
            string langDefPath (options.getAbsLangPath().empty() ? dataDir.getLangPath ( resolvedSuffix +".lang") : options.getAbsLangPath());

            if (generator->loadLanguage( langDefPath ) == highlight::LOAD_OK) {
                std::cout << "highlight: This syntax is supported\n";
                return EXIT_SUCCESS;
            } else {
                cerr << "highlight: This syntax is not supported\n";
                return EXIT_FAILURE;
            }
        }
    }

    generator->setHTMLAttachAnchors ( options.attachLineAnchors() );
    generator->setHTMLOrderedList ( options.orderedList() );
    generator->setHTMLInlineCSS ( options.inlineCSS() );
    generator->setHTMLEnclosePreTag ( options.enclosePreTag() );
    generator->setHTMLAnchorPrefix ( options.getAnchorPrefix() );
    generator->setHTMLClassName ( options.getClassName() );

    generator->setLATEXReplaceQuotes ( options.replaceQuotes() );
    generator->setLATEXNoShorthands ( options.disableBabelShorthands() );
    generator->setLATEXPrettySymbols ( options.prettySymbols() );
    generator->setLATEXBeamerMode ( options.enableBeamerMode() );

    generator->setRTFPageSize ( options.getPageSize() );
    generator->setRTFCharStyles ( options.includeCharStyles() );
    generator->setRTFPageColor ( options.includePageColor() );

    generator->setSVGSize ( options.getSVGWidth(),  options.getSVGHeight() );

    generator->setESCCanvasPadding ( options.getCanvasPadding() );

    if (options.useCRDelimiter())
        generator->setEOLDelimiter('\r');

    generator->setValidateInput ( options.validateInput() );
    generator->setNumberWrappedLines ( options.numberWrappedLines() );

    generator->setStyleInputPath ( options.getStyleInFilename() );
    generator->setStyleOutputPath ( options.getStyleOutFilename() );
    generator->setIncludeStyle ( options.includeStyleDef() );
    generator->setPrintLineNumbers ( options.printLineNumbers(), options.getNumberStart() );
    generator->setPrintZeroes ( options.fillLineNrZeroes() );
    generator->setFragmentCode ( options.fragmentOutput() );
    generator->setOmitVersionComment ( options.omitVersionInfo() );
    generator->setIsolateTags ( options.isolateTags() );

    generator->setKeepInjections ( options.keepInjections());
    generator->setPreformatting ( options.getWrappingStyle(),
                                  ( generator->getPrintLineNumbers() ) ?
                                  options.getLineLength() - options.getNumberWidth() : options.getLineLength(),
                                  options.getNumberSpaces() );

    generator->setBaseFont ( options.getBaseFont() ) ;
    generator->setBaseFontSize ( options.getBaseFontSize() ) ;
    generator->setLineNumberWidth ( options.getNumberWidth() );
    generator->disableTrailingNL(options.disableTrailingNL());
    generator->setPluginParameter(options.getPluginParameter());

    if (options.getLineRangeStart()>0 && options.getLineRangeEnd()>0){
        generator->setStartingInputLine(options.getLineRangeStart());
        generator->setMaxInputLineCnt(options.getLineRangeEnd());
    }

    bool styleFileWanted = !options.fragmentOutput() || options.styleOutPathDefined();

    const vector <string> pluginFileList=collectPluginPaths( options.getPluginPaths());
    for (const auto& pluginFile : pluginFileList) {
        if ( !generator->initPluginScript(pluginFile) ) {
            cerr << "highlight: "
                 << generator->getPluginScriptError()
                 << " in "
                 << pluginFile
                 <<"\n";
            return EXIT_FAILURE;
        }
    }

    if ( !generator->initTheme ( themePath, options.isLsSemantic() ) ) {
        cerr << "highlight: "
             << generator->getThemeInitError()
             << "\n";
        return EXIT_FAILURE;
    }

    if ( options.printOnlyStyle() ) {
        if (!options.formatSupportsExtStyle()) {
            cerr << "highlight: output format supports no external styles.\n";
            return EXIT_FAILURE;
        }
        bool useStdout =  options.getStyleOutFilename() =="stdout" || options.forceStdout();
        string cssOutFile=options.getOutDirectory()  + options.getStyleOutFilename();
        bool success=generator->printExternalStyle ( useStdout?"":cssOutFile );
        if ( !success ) {
            cerr << "highlight: Could not write " << cssOutFile <<".\n";
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    bool formattingEnabled = generator->initIndentationScheme ( options.getIndentScheme() );

    if ( !formattingEnabled && !options.getIndentScheme().empty() ) {
        cerr << "highlight: Undefined indentation scheme "
             << options.getIndentScheme()
             << ".\n";
        return EXIT_FAILURE;
    }

    //generator->setIndentationOptions(options.getAStyleOptions());

    string outDirectory = options.getOutDirectory();
#ifndef _WIN32
    ifstream dirTest ( outDirectory.c_str() );
    if ( !outDirectory.empty() && !options.quietMode() && !dirTest ) {
        cerr << "highlight: Output directory \""
             << outDirectory
             << "\" does not exist.\n";
        return EXIT_FAILURE;
    }
    dirTest.close();
#endif

    bool initError=false, IOError=false, twoPassMode=false;
    unsigned int fileCount=inFileList.size(),
                 fileCountWidth=getNumDigits ( fileCount ),
                 i=0,
                 numBadFormatting=0,
                 numBadInput=0,
                 numBadOutput=0;

    vector<string> badFormattedFiles, badInputFiles, badOutputFiles;
    std::set<string> usedFileNames;
    string inFileName, outFilePath;
    string suffix, lastSuffix;
    string twoPassOutFile=Platform::getTempFilePath();
    bool usesLSClient=false;

    if ( options.syntaxGiven() ) { // user defined language definition, valid for all files
        string syntaxByFile=options.getSyntaxByFilename();
        string testSuffix = syntaxByFile.empty() ? options.getSyntax() : dataDir.getFileSuffix(syntaxByFile);
        suffix = dataDir.guessFileType (testSuffix, syntaxByFile, syntaxByFile.empty(), options.getSingleOutFilename().length()==0 );
    }

    generator->setFilesCnt(fileCount);

    while ( (serviceModeCheck(options, generator.get(), suffix, i) || i < fileCount) && !initError ) {
        std::cout.flush();

        if ( Platform::fileSize(inFileList[i]) > options.getMaxFileSize() ) {

            if ( numBadInput++ < IO_ERROR_REPORT_LENGTH || options.verbosityLevel() ) {
                badInputFiles.push_back ( inFileList[i] + " (size)" );
            }
            ++i;
            continue;
        }

        if (i==0 && twoPassMode) {
             if ( !generator->initPluginScript(twoPassOutFile) ) {
                cerr << "highlight: "
                 << generator->getPluginScriptError()
                 << " in "
                 << twoPassOutFile
                 <<"\n";
                initError = true;
                break;
            }
        }

        if (!options.runServiceMode()) {
            if (suffix.empty() || !options.syntaxGiven() ) { // determine file type for each file
                suffix = dataDir.guessFileType ( dataDir.getFileSuffix ( inFileList[i] ), inFileList[i] );
            }
        }

        if ( suffix.empty()  && options.forceOutput()) suffix=options.getFallbackSyntax(); //avoid segfault

        if ( suffix.empty() ) {
            if ( !options.enableBatchMode() )
                cerr << "highlight: Undefined language definition. Use --"
                     << OPT_SYNTAX << " option.\n";
            if ( !options.forceOutput() ) {
                initError = true;
                break;
            }
        }

        if ( suffix != lastSuffix ) {

            string langDefPath=options.getAbsLangPath().empty() ? dataDir.getLangPath ( suffix+".lang" ) : options.getAbsLangPath();

            if (!Platform::fileExists(langDefPath) && !options.getFallbackSyntax().empty()) {
                langDefPath = dataDir.getLangPath ( options.getFallbackSyntax()+".lang" );
            }

            highlight::LoadResult loadRes= generator->loadLanguage( langDefPath );

            if ( loadRes==highlight::LOAD_FAILED_REGEX ) {
                cerr << "highlight: Regex error ( "
                     << generator->getSyntaxRegexError()
                     << " ) in "<<suffix<<".lang\n";
                initError = true;
                break;
            } else if ( loadRes==highlight::LOAD_FAILED_LUA ) {
                cerr << "highlight: Lua error ( "
                     << generator->getSyntaxLuaError()
                     << " ) in "<<suffix<<".lang\n";
                initError = true;
                break;
            } else if ( loadRes==highlight::LOAD_FAILED ) {
                // do also ignore error msg if --syntax parameter should be skipped
                if ( ! (options.forceOutput() || options.quietMode() || options.isSkippedExt ( suffix )) ) {
                    cerr << "highlight: Unknown source file extension \""
                         << suffix
                         << "\". Consider the "
                         << (options.enableBatchMode() ? "--skip" : "--force or --syntax")
                         << " option.\n";
                }
                if ( !options.forceOutput() ) {
                    initError = true;
                    break;
                }
            }
            if ( options.verbosityLevel() && loadRes==highlight::LOAD_OK ) {
                printDebugInfo ( generator->getSyntaxReader(), langDefPath, options.verbosityLevel() );
            }
            lastSuffix = suffix;

            string encoding= options.getEncoding();
            //user has explicitly defined the encoding:
            if (!options.encodingDefined()) {

                //syntax definition setting:
                string encodingHint= generator->getSyntaxEncodingHint();
                if (encodingHint.size())
                    encoding=encodingHint;

                // filetypes.conf setting has higher priority:
                encodingHint= dataDir.getEncodingHint(suffix);
                if (encodingHint.size())
                    encoding=encodingHint;
            }
            generator->setEncoding (encoding);

            if (lsSyntax==suffix) {

                if (options.getWrappingStyle()!=highlight::WRAP_DISABLED || options.getIndentScheme().size()) {
                    cerr << "highlight: no reformatting allowed with LSP options.\n";
                    initError = true;
                    break;
                }

                //LSP requires absolute paths
                if (inFileList[i].empty()) {
                    cerr << "highlight: no input file path defined.\n";
                    initError = true;
                    break;
                }

                if ( lsExecutable.empty() ) {
                    cerr << "highlight: no LS executable defined. Consider the --ls-exec or --ls-profile options.\n";
                    initError = true;
                    break;
                }

                highlight::LSResult lsInitRes=generator->initLanguageServer ( lsExecutable, lsOptions,
                                                                              options.getLsWorkspace(), lsSyntax, lsDelay,
                                                                              options.verbosityLevel(), lsLegacy );
                if ( lsInitRes==highlight::INIT_BAD_PIPE ) {
                    cerr << "highlight: language server connection failed\n";
                    initError = true;
                    break;
                } else if ( lsInitRes==highlight::INIT_BAD_REQUEST ) {
                    cerr << "highlight: language server initialization failed. Consider --ls-legacy option.\n";
                    initError = true;
                    break;
                }
                usesLSClient=true;

                generator->lsAddSyntaxErrorInfo( (options.isLsHover() || options.isLsSemantic()) && options.isLsSyntaxError() );

                if (options.isLsHover()) {
                    if (!generator->isHoverProvider()) {
                        cerr << "highlight: language server is no hover provider\n";
                        initError = true;
                        break;
                    }
                    generator->lsAddHoverInfo( true );
                }
            }
        }

        if (usesLSClient && lsSyntax==suffix) {
            generator->lsOpenDocument(inFileList[i], suffix);

            if (options.isLsSemantic()) {
                if (!generator->isSemanticTokensProvider()) {
                    cerr << "highlight: language server is no semantic token provider\n";
                    initError = true;
                    break;
                }
                generator->lsAddSemanticInfo(inFileList[i], suffix);
            }
        }

        if (twoPassMode && !generator->syntaxRequiresTwoPassRun()) {
            ++i;
            continue;
        }

        string::size_type pos= ( inFileList[i] ).find_last_of ( Platform::pathSeparator );
        inFileName = inFileList[i].substr ( pos+1 );

        if ( options.enableBatchMode() ) {
            if (usedFileNames.count(inFileName)) {
                string prefix = inFileList[i].substr(0, pos);
                // ignore dots in path
                prefix.erase(std::remove(prefix.begin(), prefix.end(), '.'), prefix.end());
                // replace path separators with underscores to create a flat prefix
                std::replace(prefix.begin(), prefix.end(), Platform::pathSeparator, '_');
                // remove leading underscore if any
                if (!prefix.empty() && prefix[0] == '_') {
                    prefix.erase(0, 1);
                }
                if (!prefix.empty()) {
                    inFileName.insert(0, prefix + "_");
                }
            } else {
                usedFileNames.insert(inFileName);
            }
            if (!options.forceStdout()){
                outFilePath = outDirectory;
                outFilePath += inFileName;
                outFilePath += options.getOutFileSuffix();
            }
            if ( !options.quietMode() &&  !options.forceStdout() ) {
                if ( options.printProgress() ) {
                    printProgressBar ( fileCount, i+1 );
                } else {
                    printCurrentAction ( outFilePath, fileCount, i+1, fileCountWidth );
                }
            }
        } else if (!options.forceStdout()) {
            outFilePath = options.getSingleOutFilename();
            if ( outFilePath.size() && outFilePath==options.getSingleInFilename() ) {
                cerr << "highlight: Output path equals input path: \""
                     << outFilePath << "\".\n";
                initError = true;
                break;
            }
        }

        if ( options.useFNamesAsAnchors() ) {
            generator->setHTMLAnchorPrefix ( inFileName );
        }

        generator->setTitle ( options.getDocumentTitle().empty() ?
                              inFileList[i]:options.getDocumentTitle() );

        generator->setKeyWordCase ( options.getKeywordCase() );
        highlight::ParseError error = generator->generateFile ( inFileList[i], outFilePath );

        if ( error==highlight::BAD_INPUT ) {
            if ( numBadInput++ < IO_ERROR_REPORT_LENGTH || options.verbosityLevel() ) {
                badInputFiles.push_back ( inFileList[i] );
            }
        } else if ( error==highlight::BAD_OUTPUT ) {
            if ( numBadOutput++ < IO_ERROR_REPORT_LENGTH || options.verbosityLevel() ) {
                badOutputFiles.push_back ( outFilePath );
            }
        }
        if ( formattingEnabled && !generator->formattingIsPossible() ) {
            if ( numBadFormatting++ < IO_ERROR_REPORT_LENGTH || options.verbosityLevel() ) {
                badFormattedFiles.push_back ( outFilePath );
            }
        }

        if (usesLSClient && lsSyntax==suffix) {
            //pyls hangs
            generator->lsCloseDocument(inFileList[i], suffix);
        }

        ++i;

        if (i==fileCount && outFilePath.size() && generator->requiresTwoPassParsing() && twoPassOutFile.size()
            && !numBadInput && !numBadOutput && !twoPassMode) {

            bool success=generator->printPersistentState(twoPassOutFile);
            if ( !success ) {
                cerr << "highlight: Could not write "<< twoPassOutFile <<".\n";
                IOError = true;
            } else {
                twoPassMode=true;
                if ( !options.quietMode() &&  !options.forceStdout() ) {
                    std::cout << "Enabling two-pass mode using "<<twoPassOutFile<<"\n";
                }
                //start over, add plug-in to list in next iteration
                usedFileNames.clear();
                generator->resetSyntaxReaders();
                i=0;
                lastSuffix.clear();
                numBadFormatting=0;
                badFormattedFiles.clear();
            }
        }
    }

    if ( i  && !options.includeStyleDef()
            && !options.inlineCSS()
            && styleFileWanted
            && options.formatSupportsExtStyle() ) {
        string cssOutFile=outDirectory  + options.getStyleOutFilename();
        bool success=generator->printExternalStyle ( cssOutFile );
        if ( !success ) {
            cerr << "highlight: Could not write " << cssOutFile <<".\n";
            IOError = true;
        }
    }

    if ( i && options.printIndexFile() ) {
        bool success=generator -> printIndexFile ( inFileList, outDirectory );
        if ( !success ) {
            cerr << "highlight: Could not write index file.\n";
            IOError = true;
        }
    }

    if ( numBadInput ) {
        printIOErrorReport ( numBadInput, badInputFiles, "read input", "<stdin>" );
        IOError = true;
    }
    if ( numBadOutput ) {
        printIOErrorReport ( numBadOutput, badOutputFiles, "write output",  "<stdout>" );
        IOError = true;
    }
    if ( numBadFormatting ) {
        printIOErrorReport ( numBadFormatting, badFormattedFiles, "reformat", "<stdout>" );
    }

    vector<string> posTestErrors = generator->getPosTestErrors();
    if (posTestErrors.size()){
        IOError = true;
        printIOErrorReport ( posTestErrors.size(), posTestErrors, "validate", "<stdin>" );
    }

    if (twoPassMode) {
        unlink(twoPassOutFile.c_str());
    }

    if (usesLSClient) {
        generator->exitLanguageServer();
    }

    return ( initError || IOError ) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main ( const int argc, const char *argv[] )
{
    try {
        HLCmdLineApp app;
        return app.run ( argc, argv );
    } catch (std::exception &e) {
        std::cerr << "highlight: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "highlight: An unknown error occurred." << std::endl;
        return EXIT_FAILURE;
    }
}
