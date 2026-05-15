### Build and install highlight (lib, cli and gui).
### This meson build file is NOT mandatory, you can still use the old makefile
### if preferred.
### The script tries to compile the GUI using Qt6, with Qt5 5.15 as fallback.

project('highlight-all', 'cpp',
  meson_version: '>=0.63',
  default_options : ['cpp_std=c++17', 'warning_level=3', 'optimization=2'])

cmake = import('cmake')
pkg = import('pkgconfig')

# Installation directories
data_dir = get_option('prefix') / get_option('datadir')
man_dir = get_option('prefix') / get_option('mandir')
conf_dir = get_option('prefix') / get_option('sysconfdir')

highlight_conf_dir = conf_dir / 'highlight'

highlight_data_dir = data_dir / 'highlight'
highlight_doc_dir = data_dir / 'doc' / 'highlight'
gui_files_dir = highlight_data_dir / 'gui_files'

# Compiler options

add_project_arguments(
    '-DNDEBUG',
    '-D_FILE_OFFSET_BITS=64',
    f'-DHL_CONFIG_DIR="@highlight_conf_dir@/"',
    f'-DHL_DATA_DIR="@highlight_data_dir@/"',
    f'-DHL_DOC_DIR="@highlight_doc_dir@/"',
    language: 'cpp'
)

# Include directories
inc = include_directories('src/include')

# Dependencies
boost_dep = dependency('boost')
lua_dep = dependency('lua', version: '>=5.5', required: false)
if not lua_dep.found()
  lua_dep = dependency('lua', required: true)
endif

# Subdirectories
core_dir = 'src/core'
cli_dir = 'src/cli'
gui_qt_dir = 'src/gui-qt'

# Source files
core_objects = files(
  'src/core/stylecolour.cpp',
  'src/core/stringtools.cpp',
  'src/core/xhtmlgenerator.cpp',
  'src/core/latexgenerator.cpp',
  'src/core/texgenerator.cpp',
  'src/core/rtfgenerator.cpp',
  'src/core/htmlgenerator.cpp',
  'src/core/ansigenerator.cpp',
  'src/core/svggenerator.cpp',
  'src/core/codegenerator.cpp',
  'src/core/xterm256generator.cpp',
  'src/core/pangogenerator.cpp',
  'src/core/bbcodegenerator.cpp',
  'src/core/odtgenerator.cpp',
  'src/core/syntaxreader.cpp',
  'src/core/elementstyle.cpp',
  'src/core/themereader.cpp',
  'src/core/keystore.cpp',
  'src/core/lspclient.cpp',
  'src/core/datadir.cpp',
  'src/core/preformatter.cpp',
  'src/core/platform_fs.cpp',
  'src/core/astyle/ASStreamIterator.cpp',
  'src/core/astyle/ASResource.cpp',
  'src/core/astyle/ASFormatter.cpp',
  'src/core/astyle/ASBeautifier.cpp',
  'src/core/astyle/ASEnhancer.cpp',
  'src/core/Diluculum/InternalUtils.cpp',
  'src/core/Diluculum/LuaExceptions.cpp',
  'src/core/Diluculum/LuaFunction.cpp',
  'src/core/Diluculum/LuaState.cpp',
  'src/core/Diluculum/LuaUserData.cpp',
  'src/core/Diluculum/LuaUtils.cpp',
  'src/core/Diluculum/LuaValue.cpp',
  'src/core/Diluculum/LuaVariable.cpp',
  'src/core/Diluculum/LuaWrappers.cpp'
)

cli_objects = files(
  'src/cli/arg_parser.cc',
  'src/cli/cmdlineoptions.cpp',
  'src/cli/help.cpp',
  'src/cli/main.cpp'
)

# Library
highlight_lib = static_library('highlight-lib', core_objects,
  include_directories: inc,
  dependencies: [boost_dep, lua_dep]
)

# CLI executable
highlight = executable('highlight', cli_objects,
  include_directories: inc,
  dependencies: [boost_dep, lua_dep],
  link_with: highlight_lib,
  install: true
)

# GUI components

qt_version = 'qt6'
qt = dependency('qt6', modules : ['Core', 'Gui', 'Widgets'], required: false)
if not qt.found()
  qt = dependency('qt5', version : '>=5.15', modules : ['Core', 'Gui', 'Widgets'])
  qt_version = 'qt5'
endif

gui_objects = files(
  'src/gui-qt/main.cpp',
  'src/gui-qt/mainwindow.cpp',
  'src/gui-qt/io_report.cpp',
  'src/gui-qt/showtextfile.cpp',
  'src/gui-qt/syntax_chooser.cpp'
)

qt5_mod = import(qt_version)
processed = qt5_mod.preprocess(
  moc_headers : ['src/gui-qt/mainwindow.h', 'src/gui-qt/io_report.h', 'src/gui-qt/showtextfile.h', 'src/gui-qt/syntax_chooser.h'],   # Only headers that need moc should be put here
  # moc_sources : 'helperFile.cpp', # must have #include"moc_helperFile.cpp"
  ui_files    : ['src/gui-qt/mainwindow.ui', 'src/gui-qt/io_report.ui', 'src/gui-qt/showtextfile.ui', 'src/gui-qt/syntax_chooser.ui'],
  qresources  : 'src/gui-qt/highlight-gui.qrc',
)

gui_exe = executable('highlight-gui',
  sources: [gui_objects, processed],
  include_directories: inc,
  dependencies: [boost_dep, lua_dep, qt],
  link_with: highlight_lib,
  install: true
)

bash_comp_dir = data_dir / 'bash-completion' / 'completions'
fish_comp_dir = data_dir / 'fish' / 'vendor_completions.d'
zsh_comp_dir = data_dir / 'zsh' / 'site-functions'

desktop_apps_dir = data_dir / 'applications'
desktop_icons_dir = data_dir / 'icons' / 'hicolor' / '256x256' / 'apps'

# Install files and directories
install_data('filetypes.conf', install_dir: highlight_conf_dir)
install_subdir('langDefs', install_dir: highlight_data_dir)
install_subdir('themes', install_dir: highlight_data_dir)
install_subdir('plugins', install_dir: highlight_data_dir)
install_subdir('extras', install_dir: highlight_doc_dir)
install_data(['AUTHORS', 'README.adoc', 'README_LSP_CLIENT.adoc', 'README_PLUGINS.adoc', 'ChangeLog.adoc', 'COPYING', 'INSTALL'], install_dir: highlight_doc_dir)
install_data('man/highlight.1', install_dir: man_dir / 'man1')
install_data('man/filetypes.conf.5', install_dir: man_dir / 'man5')
install_data(
  'sh-completion/highlight.bash',
  rename: 'highlight',
  install_dir: bash_comp_dir,
)
install_data('sh-completion/highlight.fish', install_dir: fish_comp_dir)
install_data(
  'sh-completion/highlight.zsh',
  rename: '_highlight',
  install_dir: zsh_comp_dir,
)
install_data('highlight.desktop', install_dir: desktop_apps_dir)
install_data('src/gui-qt/highlight.png', install_dir: desktop_icons_dir)
install_data('gui_files/ext/fileopenfilter.conf', install_dir: gui_files_dir / 'ext')
#install_subdir('gui_files/l10n', install_dir: join_paths(gui_files_dir, 'l10n'), files: '*.qm')

# Platform-specific configurations
if host_machine.system() == 'windows'
  # Windows specific configurations
elif host_machine.system() == 'darwin'
  # macOS specific configurations
endif
