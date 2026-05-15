cmake_minimum_required(VERSION 3.16)

# Policy settings
cmake_policy(SET CMP0074 NEW) # find_package() uses _ROOT variables
cmake_policy(SET CMP0167 NEW) # FindBoost is deprecated, use Boost config

project(highlight-all VERSION 4.0)

# Build options
option(BUILD_GUI "Build and install the Qt GUI" ON)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include standard installation paths
include(GNUInstallDirs)

# Compile options
if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -O2 -DNDEBUG -D_FILE_OFFSET_BITS=64)
endif()

# Directories
set(CORE_DIR src/core)
set(CLI_DIR src/cli)
set(GUI_QT_DIR src/gui-qt)
set(INCLUDE_DIR src/include)

# Dependencies
find_package(Boost REQUIRED)

find_package(PkgConfig REQUIRED)
# Preferred: try PkgConfig first for Lua (useful on Linux)
pkg_check_modules(LUA lua5.4 QUIET)
if(NOT LUA_FOUND)
    pkg_check_modules(LUA lua5.3 QUIET)
endif()
if(NOT LUA_FOUND)
    pkg_check_modules(LUA lua QUIET)
endif()

# Fallback: standard FindLua
if(NOT LUA_FOUND)
    find_package(Lua REQUIRED)
    set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
    set(LUA_LIBRARIES ${LUA_LIBRARIES})
endif()

# Source lists
set(CORE_SOURCES
    ${CORE_DIR}/stylecolour.cpp
    ${CORE_DIR}/stringtools.cpp
    ${CORE_DIR}/xhtmlgenerator.cpp
    ${CORE_DIR}/latexgenerator.cpp
    ${CORE_DIR}/texgenerator.cpp
    ${CORE_DIR}/rtfgenerator.cpp
    ${CORE_DIR}/htmlgenerator.cpp
    ${CORE_DIR}/ansigenerator.cpp
    ${CORE_DIR}/svggenerator.cpp
    ${CORE_DIR}/codegenerator.cpp
    ${CORE_DIR}/xterm256generator.cpp
    ${CORE_DIR}/pangogenerator.cpp
    ${CORE_DIR}/bbcodegenerator.cpp
    ${CORE_DIR}/odtgenerator.cpp
    ${CORE_DIR}/syntaxreader.cpp
    ${CORE_DIR}/elementstyle.cpp
    ${CORE_DIR}/themereader.cpp
    ${CORE_DIR}/keystore.cpp
    ${CORE_DIR}/lspclient.cpp
    ${CORE_DIR}/datadir.cpp
    ${CORE_DIR}/preformatter.cpp
    ${CORE_DIR}/platform_fs.cpp
    ${CORE_DIR}/astyle/ASStreamIterator.cpp
    ${CORE_DIR}/astyle/ASResource.cpp
    ${CORE_DIR}/astyle/ASFormatter.cpp
    ${CORE_DIR}/astyle/ASBeautifier.cpp
    ${CORE_DIR}/astyle/ASEnhancer.cpp
    ${CORE_DIR}/Diluculum/InternalUtils.cpp
    ${CORE_DIR}/Diluculum/LuaExceptions.cpp
    ${CORE_DIR}/Diluculum/LuaFunction.cpp
    ${CORE_DIR}/Diluculum/LuaState.cpp
    ${CORE_DIR}/Diluculum/LuaUserData.cpp
    ${CORE_DIR}/Diluculum/LuaUtils.cpp
    ${CORE_DIR}/Diluculum/LuaValue.cpp
    ${CORE_DIR}/Diluculum/LuaVariable.cpp
    ${CORE_DIR}/Diluculum/LuaWrappers.cpp
)

set(CLI_SOURCES
    ${CLI_DIR}/arg_parser.cc
    ${CLI_DIR}/cmdlineoptions.cpp
    ${CLI_DIR}/help.cpp
    ${CLI_DIR}/main.cpp
)

# Shared Core Library
add_library(highlight-lib STATIC ${CORE_SOURCES})
target_include_directories(highlight-lib PUBLIC ${INCLUDE_DIR})
target_include_directories(highlight-lib PRIVATE ${Boost_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS})
target_compile_definitions(highlight-lib PRIVATE
    HL_CONFIG_DIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}/highlight/"
    HL_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/highlight/"
    HL_DOC_DIR="${CMAKE_INSTALL_FULL_DOCDIR}/highlight/"
)

# CLI Application
add_executable(highlight ${CLI_SOURCES})
target_link_libraries(highlight PRIVATE highlight-lib ${LUA_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_compile_definitions(highlight PRIVATE
    HL_CONFIG_DIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}/highlight/"
    HL_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/highlight/"
    HL_DOC_DIR="${CMAKE_INSTALL_FULL_DOCDIR}/highlight/"
)

# Installation Rules
install(TARGETS highlight RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES filetypes.conf lsp.conf DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/highlight/)
install(DIRECTORY langDefs themes plugins DESTINATION "${CMAKE_INSTALL_DATADIR}/highlight")
install(DIRECTORY extras DESTINATION "${CMAKE_INSTALL_DOCDIR}/highlight")
install(FILES AUTHORS README.adoc README_LSP_CLIENT.adoc README_PLUGINS.adoc ChangeLog.adoc COPYING INSTALL DESTINATION "${CMAKE_INSTALL_DOCDIR}/highlight")
install(FILES man/highlight.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES man/filetypes.conf.5 DESTINATION ${CMAKE_INSTALL_MANDIR}/man5)

# Shell Completion
install(FILES sh-completion/highlight.bash RENAME _highlight DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions)
install(FILES sh-completion/highlight.fish DESTINATION ${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d)
install(FILES sh-completion/highlight.zsh RENAME _highlight DESTINATION ${CMAKE_INSTALL_DATADIR}/zsh/site-functions)

# Qt GUI
if(BUILD_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
    if(Qt6_FOUND)
        set(Qt_LIBS Qt6::Core Qt6::Gui Qt6::Widgets)
    else()
        find_package(Qt5 5.15 COMPONENTS Core Gui Widgets REQUIRED)
        set(Qt_LIBS Qt5::Core Qt5::Gui Qt5::Widgets)
    endif()

    set(GUI_SOURCES
        ${GUI_QT_DIR}/main.cpp
        ${GUI_QT_DIR}/mainwindow.cpp
        ${GUI_QT_DIR}/io_report.cpp
        ${GUI_QT_DIR}/showtextfile.cpp
        ${GUI_QT_DIR}/syntax_chooser.cpp
        ${GUI_QT_DIR}/highlight-gui.qrc
    )

    add_executable(highlight-gui ${GUI_SOURCES})
    target_link_libraries(highlight-gui PRIVATE highlight-lib ${Qt_LIBS} ${LUA_LIBRARIES} ${Boost_LIBRARIES})
    target_include_directories(highlight-gui PRIVATE ${INCLUDE_DIR} ${LUA_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
    target_compile_definitions(highlight-gui PRIVATE
        HL_CONFIG_DIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}/highlight/"
        HL_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/highlight/"
        HL_DOC_DIR="${CMAKE_INSTALL_FULL_DOCDIR}/highlight/"
        QT
    )

    install(TARGETS highlight-gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    install(FILES highlight.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES src/gui-qt/highlight.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
    install(FILES gui_files/ext/fileopenfilter.conf DESTINATION "${CMAKE_INSTALL_DATADIR}/highlight/gui_files/ext")
    install(DIRECTORY gui_files/l10n/ DESTINATION "${CMAKE_INSTALL_DATADIR}/highlight/gui_files/l10n" FILES_MATCHING PATTERN "*.qm")
endif()

# Uninstall Logic
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    )
endif()
