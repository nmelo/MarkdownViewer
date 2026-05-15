/***************************************************************************
                          help.cpp  -  description
                             -------------------
    begin                : Die Apr 23 2002
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

#include <iostream>
#include "help.h"


namespace Help
{
void printHelp(const std::string &topic)
{
    if (topic=="syntax" || topic=="lang")  {
        std::cout <<"SYNTAX HELP:\n\n";
        std::cout <<"A language definition describes syntax elements of a programming language which\n";
        std::cout <<"will be highlighted by different colours and font types.\n";
        std::cout <<"\n";
        std::cout <<"These definitions are saved as Lua scripts in HL_DATA_DIR/langDefs.\n";
        std::cout <<"File extensions and shebangs are mapped to language definitions in \n";
        std::cout <<"filetypes.conf (see --print-config for the configuration search paths).\n";
        std::cout <<"\n";
        std::cout <<"Apply custom language definitions with --config-file or as absolute path to -S.\n";
        std::cout <<"\n";
        std::cout <<"Use plug-in scripts (--plug-in) to modify syntax parsing and output.\n";
        std::cout <<"\n";
        std::cout <<"Print all installed language definitions with --list-scripts=langs.\n";
        std::cout <<"\n";
        std::cout <<"Exemplary config files:\n";
        std::cout <<"c.lang:      C and C++ syntax file (compiled sources) \n";
        std::cout <<"python.lang: Python syntax file    (script sources)\n";
        std::cout <<"ini.lang:    INI syntax file       (configuration)\n";
        std::cout <<"xml.lang:    XML syntax file       (markup)\n";
        std::cout <<"\n";
        std::cout <<"Refer to README files for configuration syntax and examples.\n";

    } else if (topic=="theme") {
        std::cout <<"THEME HELP:\n\n";
        std::cout <<"A colour theme defines the formatting of recognized syntax elements.\n";
        std::cout <<"\n";
        std::cout <<"These descriptions are saved as Lua scripts in HL_DATA_DIR/themes.\n";
        std::cout <<"\n";
        std::cout <<"Apply custom themes with --config-file or as absolute path to --theme.\n";
        std::cout <<"\n";
        std::cout <<"Use plug-in scripts (--plug-in) to modify the formatting output.\n";
        std::cout <<"\n";
        std::cout <<"Print all installed themes with --list-scripts=themes.\n";
        std::cout <<"\n";
        std::cout <<"Add 'base16/' as prefix to read a theme of the Base16 set.\n";
        std::cout <<"\n";
        std::cout <<"Exemplary config files:\n";
        std::cout <<"darkspectrum.theme: dark canvas (vim) \n";
        std::cout <<"edit-eclipse:       light canvas (IDE)\n";
        std::cout <<"bespin.lang:        dark canvas  (Base16)\n";
        std::cout <<"\n";
        std::cout <<"Refer to README files for configuration syntax and examples.\n";

    } else if (topic=="config") {
        std::cout <<"CONFIG HELP:\n\n";
        std::cout <<"Highlight is configured by a set of Lua scripts and environment variables.\n";
        std::cout <<"See --print-config for the configuration file search paths.\n\n";
        std::cout <<"List of environment variables:\n";
        std::cout <<"HIGHLIGHT_DATADIR: sets the path to highlight's configuration scripts\n";
        std::cout <<"HIGHLIGHT_OPTIONS: may contain command line options, but no input file paths\n\n";
        std::cout <<"Highlight reads TERM and COLORTERM to determine the appropriate default output.\n";
    } else if (topic=="plugin" || topic=="plug-in") {
        std::cout <<"PLUG-IN HELP:\n\n";
        std::cout <<"The plug-in interface allows modifications of syntax parsing, colouring and\n";
        std::cout <<"the document's header and footer.\n";
        std::cout <<"\n";
        std::cout <<"The --plug-in option reads the path of a Lua script which overrides or\n";
        std::cout <<"enhances the settings of theme and language definition files. Plug-ins make\n";
        std::cout <<"it possible to apply custom settings without the need to edit installed\n";
        std::cout <<"configuration files.\n";
        std::cout <<"You can apply multiple plugins by using the --plug-in option more than once.\n";
        std::cout <<"\n";
        std::cout <<"Pass arguments to a plug-in with --plug-in-param.\n";
        std::cout <<"\n";
        std::cout <<"Print all installed themes with --list-scripts=plugins.\n";
        std::cout <<"\n";
        std::cout <<"Exemplary config files:\n";
        std::cout <<"cpp_qt.lua:             Add Qt keywords to C and C++ (simple structure)\n";
        std::cout <<"outhtml_codefold.lua:   Adds code folding for C style languages, and more (advanced)\n";
        std::cout <<"bash_ref_man7_org.lua:  Add man7.org reference links to Bash output (advanced)\n";
        std::cout <<"\n";
        std::cout <<"See README_PLUGINS.adoc for a detailed description and examples of packaged plugins.\n";

    } else if (topic=="test") {
        std::cout <<"SYNTAX TEST HELP:\n\n";
        std::cout <<"The syntax parsing can be validated with test state indicators embedded in comments.\n";
        std::cout <<"These are recognized in files saved as syntax_test_*.\n\n";
        std::cout <<"A test case is defined by two entities: column and expected state.\n";
        std::cout <<"The column is defined by ^ (here) or < (comment start / first column).\n";
        std::cout <<"This indicator points at the tested syntax element of the previous line.\n";
        std::cout <<"The state identifiers match the corresponding HTML output CSS class names.\n\n";
        std::cout <<"See README_TESTCASES.adoc for a detailed description and examples.\n";
    } else if (topic=="lsp") {
        std::cout <<"LANGUAGE SERVER PROTOCOL HELP:\n\n";
        std::cout <<"Highlight can invoke LSP servers to enhance its output. Warning: These features are WIP.\n";
        std::cout <<"Language servers are be configured in the lsp.conf file. Each parameter of this file\n";
        std::cout <<"can also be set using --ls-exec, --ls-option, --ls-delay and --ls-syntax.\n";
        std::cout <<"Important: LSP features require absolute input paths and disable reformatting (-F).\n";
    } else {
        std::cout<<"USAGE: highlight [OPTIONS]... [FILES]...\n";
        std::cout<<"\n";
        std::cout<<"General options:\n";
        std::cout<<"\n";
        std::cout<<" -B, --batch-recursive=<wc>     convert all matching files, searches subdirs\n";
        std::cout<<"                                  (Example: -B '*.cpp')\n";
        std::cout<<" -D, --data-dir=<directory>     set path to data directory\n";
        std::cout<<"     --config-file=<file>       set path to a lang or theme file\n";
        std::cout<<" -d, --outdir=<directory>       name of output directory\n";
        std::cout<<" -h, --help[=topic]             print this help or a topic description\n";
        std::cout<<"                                  <topic> = [syntax, theme, plugin, config, test, lsp]\n";
        std::cout<<" -i, --input=<file>             name of single input file\n";
        std::cout<<" -o, --output=<file>            name of single output file\n";
        std::cout<<" -P, --progress                 print progress bar in batch mode\n";
        std::cout<<" -q, --quiet                    suppress progress info in batch mode\n";
        std::cout<<" -S, --syntax=<type|path>       specify type of source code or syntax file path\n";
        std::cout<<"     --syntax-by-name=<name>    specify type of source code by given name\n";
        std::cout<<"                                  will not read a file of this name, useful for stdin\n";
        std::cout<<"     --syntax-supported         test if the given syntax can be loaded\n";
        std::cout<<" -v, --verbose                  print debug info; repeat to show more information\n";
        std::cout<<"     --force[=syntax]           generate output if input syntax is unknown\n";
        std::cout<<"     --list-scripts=<type>      list installed scripts\n";
        std::cout<<"                                  <type> = [langs, themes, plugins]\n";
        std::cout<<"     --list-cat=<categories>    filter the scripts by the given categories\n";
        std::cout<<"                                  (example: --list-cat='source;script')\n";
        std::cout<<"     --max-size=<size>          set maximum input file size\n";
        std::cout<<"                                  (examples: 512M, 1G; default: 256M)\n";
        std::cout<<"     --plug-in=<script>         execute Lua plug-in script; repeat option to\n";
        std::cout<<"                                  execute multiple plug-ins\n";
        std::cout<<"     --plug-in-param=<value>    set plug-in input parameter\n";
        std::cout<<"     --print-config             print path configuration\n";
        std::cout<<"     --print-style              print stylesheet only (see --style-outfile)\n";
        std::cout<<"     --skip=<list>              ignore listed unknown file types\n";
        std::cout<<"                                  (Example: --skip='bak;c~;h~')\n";

        std::cout<<"     --stdout                   output to stdout (batch mode, --print-style)\n";
        std::cout<<"     --validate-input           test if input is text, remove Unicode BOM\n";
        std::cout<<"     --service-mode             run in service mode, not stopping until signaled\n";
#if _WIN32
        std::cout<<"     --disable-echo             disable console echo back for stdin data\n";
#endif // _WIN32
        std::cout<<"     --version                  print version and copyright information\n";
        std::cout<<"\n\n";
        std::cout<<"Output formatting options:\n";
        std::cout<<"\n";
        std::cout<<" -O, --out-format=<format>      output file in given format\n";
        std::cout<<"                                  <format>=[html, xhtml, latex, tex, odt, rtf,\n";
        std::cout<<"                                  ansi, xterm256, truecolor, bbcode, pango, svg]\n";
        std::cout<<" -c, --style-outfile=<file>     name of style file or print to stdout, if\n";
        std::cout<<"                                  'stdout' is given as file argument\n";

        std::cout<<" -e, --style-infile=<file>      to be included in style-outfile (deprecated)\n";
        std::cout<<"                                  use a plug-in file instead\n";

        std::cout<<" -f, --fragment                 omit document header and footer\n";
        std::cout<<" -F, --reformat=<style>         reformats and indents output in given style\n";
        std::cout<<"                                  <style> = [allman, gnu, google, horstmann,\n";
        std::cout<<"                                  java, kr, linux, lisp, mozilla, otbs, pico,\n";
        std::cout<<"                                  vtk, ratliff, stroustrup, webkit, whitesmith]\n";

        std::cout<<" -I, --include-style            include style definition in output file\n";
        std::cout<<" -J, --line-length=<num>        line length before wrapping (see -V, -W)\n";
        std::cout<<" -j, --line-number-length=<num> line number width incl. left padding (default: 5)\n";
        std::cout<<"     --line-range=<start-end>   output only lines from number <start> to <end>\n";

        std::cout<<" -k, --font=<font>              set font (specific to output format)\n";
        std::cout<<" -K, --font-size=<num?>         set font size (specific to output format)\n";
        std::cout<<" -l, --line-numbers             print line numbers in output file\n";
        std::cout<<" -m, --line-number-start=<cnt>  start line numbering with cnt (assumes -l)\n";
        std::cout<<" -s, --style=<style|path>       set colour style (theme) or theme file path\n";
        std::cout<<" -t, --replace-tabs=<num>       replace tabs by <num> spaces\n";
        std::cout<<" -T, --doc-title=<title>        document title\n";
        std::cout<<" -u, --encoding=<enc>           set output encoding which matches input file\n";
        std::cout<<"                                  encoding; omit encoding info if set to NONE\n";
        std::cout<<" -V, --wrap-simple              wrap lines after 80 (default) characters w/o\n";
        std::cout<<"                                  indenting function parameters and statements\n";
        std::cout<<" -W, --wrap                     wrap lines after 80 (default) characters\n";
        std::cout<<"     --wrap-no-numbers          omit line numbers of wrapped lines\n";
        std::cout<<"                                  (assumes -l)\n";
        std::cout<<" -z, --zeroes                   pad line numbers with 0's\n";
        std::cout<<"     --isolate                  output each syntax token separately (verbose output)\n";
        std::cout<<"     --keep-injections          output plug-in injections in spite of -f\n";
        std::cout<<"     --kw-case=<case>           change case of case insensitive keywords\n";
        std::cout<<"                                  <case> =  [upper, lower, capitalize]\n";
        std::cout<<"     --no-trailing-nl[=mode]    omit trailing newline. If mode is empty-file, omit\n";
        std::cout<<"                                  only for empty input\n";
        std::cout<<"     --no-version-info          omit version info comment\n";

        std::cout<<"\n\n";
        std::cout<<"(X)HTML output options:\n";
        std::cout<<"\n";
        std::cout<<" -a, --anchors                  attach anchor to line numbers\n";
        std::cout<<" -y, --anchor-prefix=<str>      set anchor name prefix\n";
        std::cout<<" -N, --anchor-filename          use input file name as anchor prefix\n";
        std::cout<<" -C, --print-index              print index with hyperlinks to output files\n";
        std::cout<<" -n, --ordered-list             print lines as ordered list items\n";
        std::cout<<"     --class-name=<name>        set CSS class name prefix;\n";
        std::cout<<"                                  omit class name if set to NONE\n";
        std::cout<<"     --inline-css               output CSS within each tag (verbose output)\n";
        std::cout<<"     --enclose-pre              enclose fragmented output with pre tag \n";
        std::cout<<"                                  (assumes -f)\n";
        std::cout<<"\n\n";
        std::cout<<"LaTeX output options:\n";
        std::cout<<"\n";
        std::cout<<" -b, --babel                    disable Babel package shorthands\n";
        std::cout<<" -r, --replace-quotes           replace double quotes by \\dq{}\n";
        std::cout<<"     --beamer                   adapt output for the Beamer package\n";
        std::cout<<"     --pretty-symbols           improve appearance of brackets and other symbols\n";
        std::cout<<"\n\n";
        std::cout<<"RTF output options:\n";
        std::cout<<"\n";
        std::cout<<"     --page-color               include page color attributes\n";
        std::cout<<" -x, --page-size=<ps>           set page size \n";
        std::cout<<"                                  <ps> = [a3, a4, a5, b4, b5, b6, letter]\n";
        std::cout<<"     --char-styles              include character stylesheets\n";
        std::cout<<"\n\n";
        std::cout<<"SVG output options:\n";
        std::cout<<"\n";
        std::cout<<"     --height                   set image height (units allowed)\n";
        std::cout<<"     --width                    set image width (see --height)\n";
        std::cout<<"\n\n";
        std::cout<<"Terminal escape output options (xterm256 or truecolor):\n";
        std::cout<<"\n";
        std::cout<<"     --canvas[=width]           set background colour padding (default: 80)\n";
        std::cout<<"\n\n";

        std::cout<<"Language Server options:\n\n";
        std::cout<<"     --ls-profile=<server>      read LSP configuration from lsp.conf\n";
        std::cout<<"     --ls-delay=<ms>            set server initialization delay\n";
        std::cout<<"     --ls-exec=<bin>            set server executable name\n";
        std::cout<<"     --ls-option=<option>       set server CLI option (can be repeated)\n";
        std::cout<<"     --ls-hover                 execute hover requests (HTML output only)\n";
        std::cout<<"     --ls-semantic              retrieve semantic token types (requires LSP 3.16)\n";
        std::cout<<"     --ls-syntax=<lang>         set syntax which is understood by the server\n";
        std::cout<<"     --ls-syntax-error          retrieve syntax error information\n";
        std::cout<<"                                  (assumes --ls-hover or --ls-semantic)\n";
        std::cout<<"     --ls-workspace=<dir>       set workspace directory to init. the server\n";
        std::cout<<"     --ls-legacy                do not require a server capabilities response\n";
        std::cout<<"\n\n";

        std::cout<<"If no in- or output files are specified, stdin and stdout will be used.\n";
        std::cout<<"Reading from stdin can also be triggered using the '-' option.\n";
        std::cout<<"Default output format: xterm256 or truecolor if appropriate, HTML otherwise.\n";
        std::cout<<"Style definitions are stored in highlight.css (HTML, XHTML, SVG) or\n";
        std::cout<<"highlight.sty (LaTeX, TeX) if neither -c nor -I is given.\n";
        std::cout<<"Reformatting code (-F) will only work with C, C++, C# and Java input files.\n";
        std::cout<<"LSP features require absolute input paths and disable reformatting (-F).\n";
        std::cout<<"Wrapping lines with -V or -W will cause faulty highlighting of long single\n";
        std::cout<<"line comments and directives. Using line-range might interfere with multi\n";
        std::cout<<"line syntax elements. Use with caution.\n\n";

        std::cout<<"Run highlight --list-scripts=langs to see all supported syntax types.\n\n";
        std::cout<<"Refer to README files how to apply plug-ins to customize the output.\n\n";
        std::cout<<"Updates and information: http://www.andre-simon.de/\n";
    }
}

}
