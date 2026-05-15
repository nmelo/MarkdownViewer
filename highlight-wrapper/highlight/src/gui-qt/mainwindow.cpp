/***************************************************************************
                               mainwindow.cpp
                             -------------------
    begin                : Mo 16.03.2009
    copyright            : (C) 2009-2026 by Andre Simon
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

#include <memory>
#include <algorithm>

#include <Diluculum/LuaState.hpp>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include "showtextfile.h"

#include "io_report.h"
#include "syntax_chooser.h"


#ifdef Q_OS_WIN
#include <windows.h>
#endif
//#include <QDebug>

using std::ifstream;
using std::iterator;
using std::string;
using std::unique_ptr;
using std::vector;

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent), ui(new Ui::MainWindowClass)
{

    ui->setupUi(this);
    this->setWindowTitle(QString("Highlight %1").arg(QString::fromStdString( highlight::Info::getVersion())));

    // Read file open filter
    QFile  filterDef(getDistFileFilterPath());
    QRegularExpression rx(R"((\S+)\s?\(\*\.([\w\d]+))");

    if (filterDef.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&filterDef);
        QString line;
        while (!in.atEnd()) {
            line = in.readLine();
            fileOpenFilter+=line;
            fileOpenFilter+=";;";
            QRegularExpressionMatchIterator i = rx.globalMatch(line);
            if (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                ui->comboSelectSyntax->addItem(match.captured(1), match.captured(2));
            }
        }
    } else {
        fileOpenFilter="All files (*)";
    }

    // fill themes combo

#ifdef Q_OS_MACOS
    QDir themesDirClassic(QCoreApplication::applicationDirPath() + "/../Resources/themes");
    QDir themesDirBase16(QCoreApplication::applicationDirPath() + "/../Resources/themes/base16");

#else
    #ifdef HL_DATA_DIR
    QDir themesDirClassic(QString(HL_DATA_DIR) + "/themes");
    QDir themesDirBase16(QString(HL_DATA_DIR) + "/themes/base16");
    #else
    QDir themesDirClassic(QDir::currentPath()+"/themes");
    QDir themesDirBase16(QDir::currentPath()+"/themes/base16");
    #endif
#endif

    QStringList filesClassic = themesDirClassic.entryList(QStringList("*.theme"), QDir::Files, QDir::Name);
    QStringList filesBase16 = themesDirBase16.entryList(QStringList("*.theme"), QDir::Files, QDir::Name);

    Diluculum::LuaState ls;
    Diluculum::LuaValueMap categoryMap;

    try {

        for (const auto& fileName : filesClassic) {
            QString tPath(fileName);
            QString tCat;

            ls.doFile (themesDirClassic.absoluteFilePath(tPath).toStdString());
            QString tDesc (QString::fromStdString(ls["Description"].value().asString()));
            if (ls["Categories"].value() !=Diluculum::Nil){

                categoryMap = ls["Categories"].value().asTable();

                for(auto it = categoryMap.begin(); it != categoryMap.end(); ++it)
                {
                    tCat.append( QString::fromStdString( it->second.asString()));
                    tCat.append(",");
                }
            }
            QList<QString> themeEntry = {tPath.section('.',0, 0), tDesc, tCat, "0"};
            themesList.append(themeEntry);
        }

        for (const auto& fileName : filesBase16) {
            QString tPath(fileName);
            QString tCat;

            ls.doFile (themesDirBase16.absoluteFilePath(tPath).toStdString());
            QString tDesc (QString::fromStdString(ls["Description"].value().asString()));
            if (ls["Categories"].value() !=Diluculum::Nil){

                categoryMap = ls["Categories"].value().asTable();

                for(auto it = categoryMap.begin(); it != categoryMap.end(); ++it)
                {
                    tCat.append( QString::fromStdString( it->second.asString()));
                    tCat.append(",");
                }
            }
            QList<QString> themeEntry = { tPath.section('.',0, 0), tDesc, tCat, "1"};
            themesList.append(themeEntry);
        }
    }  catch (Diluculum::LuaError &err) {
        QMessageBox::warning(this, tr("Initialization error"),
                             tr("Could not read a colour theme: ") + QString::fromStdString(err.what()));
    }
    ui->comboTheme->setCurrentIndex(0);

    ui->comboThemeFilter->addItem(tr("light"), "light");
    ui->comboThemeFilter->addItem(tr("dark"), "dark");
    ui->comboThemeFilter->addItem(tr("B16 light"), "light");
    ui->comboThemeFilter->addItem(tr("B16 dark"), "dark");
    ui->comboThemeFilter->setCurrentIndex(0);

    ui->comboLSProfiles->addItem(tr("Please select a Server"));

    // load LSP profiles
    if (!loadLSProfiles()) {
        QMessageBox::warning(this, tr("Initialization error"),
                             tr("Could not find LSP profiles. Check installation."));
    }

    for (const auto& kv : lspProfiles) {
         ui->comboLSProfiles->addItem(QString::fromStdString( kv.first ));
    }

    QStringList fmts;
    fmts << "Allman" << "GNU" <<"Google"<< "Horstmann"<<"Lisp"<<"Java"<<"K&R"<<"Linux"
         <<"Mozilla"<<"OTBS"<<"Pico"<<"Ratliff"<<"Stroustrup"<<"VTK"<<"Webkit"<<"Whitesmith";
    ui->comboReformat->clear();
    ui->comboReformat->addItems(fmts);

    // load syntax mappings
    if (!loadFileTypeConfig()) {
        QMessageBox::warning(this, tr("Initialization error"),
                             tr("Could not find syntax definitions. Check installation."));
    }

    //avoid ugly buttons in MacOS
    #ifndef Q_OS_MACOS
    ui->pbPluginReadFilePath->setMaximumWidth(30);
    ui->pbOutputDest->setMaximumWidth(30);
    ui->pbHTMLChooseStyleIncFile->setMaximumWidth(30);
    ui->pbLATEXChooseStyleIncFile->setMaximumWidth(30);
    ui->pbTEXChooseStyleIncFile->setMaximumWidth(30);
    ui->pbSVGChooseStyleIncFile->setMaximumWidth(30);
    ui->tabIOSelection->setDocumentMode(false);
    #endif

    #ifdef Q_OS_WIN
    // https://forum.qt.io/topic/101391/windows-10-dark-theme/
    if (QSysInfo::productVersion()=="10") {
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",QSettings::NativeFormat);
        if(settings.value("AppsUseLightTheme")==0){
            qApp->setStyle(QStyleFactory::create("Fusion"));
            QPalette darkPalette;
            QColor darkColor = QColor(45,45,45);
            QColor disabledColor = QColor(127,127,127);
            darkPalette.setColor(QPalette::Window, darkColor);
            darkPalette.setColor(QPalette::WindowText, Qt::white);
            darkPalette.setColor(QPalette::Base, QColor(18,18,18));
            darkPalette.setColor(QPalette::AlternateBase, darkColor);
            darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
            darkPalette.setColor(QPalette::ToolTipText, Qt::white);
            darkPalette.setColor(QPalette::Text, Qt::white);
            darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
            darkPalette.setColor(QPalette::Button, darkColor);
            darkPalette.setColor(QPalette::ButtonText, Qt::white);
            darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
            darkPalette.setColor(QPalette::BrightText, Qt::red);
            darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

            darkPalette.setColor(QPalette::Highlight, QColor(153, 153, 153));
            darkPalette.setColor(QPalette::HighlightedText, Qt::black);
            darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

            qApp->setPalette(darkPalette);
            qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #404040; border: 1px solid white; }");
        }
    }
    #endif

    QObject::connect(ui->pbOpenFiles, SIGNAL(clicked()), this, SLOT(openFiles()));
    QObject::connect(ui->action_Open_files, SIGNAL(triggered()), this, SLOT(openFiles()));

    QObject::connect(ui->cbWrite2Src, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbIncLineNo, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbWrapping, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbEncoding, SIGNAL(clicked()), this, SLOT(plausibility()));

    QObject::connect(ui->comboFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(plausibility()));
    QObject::connect(ui->cbReformat, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbKwCase, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbHTMLEmbedStyle, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbHTMLAnchors, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbHTMLInlineCSS, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbLATEXEmbedStyle, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbTEXEmbedStyle, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbSVGEmbedStyle, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbFragment, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbLSHover, SIGNAL(clicked()), this, SLOT(plausibility()));
    QObject::connect(ui->cbLSSemantic, SIGNAL(clicked()), this, SLOT(plausibility()));

    QObject::connect(ui->tabIOSelection, SIGNAL(currentChanged(int)), this, SLOT(plausibility()));

    QObject::connect(ui->lvInputFiles, SIGNAL(itemSelectionChanged()), this, SLOT(updatePreview()));
    QObject::connect(ui->lvPluginScripts, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(updatePreview()));
    QObject::connect(ui->lvUserScripts, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(updatePreview()));
    QObject::connect(ui->lvUserScripts, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(plausibility()));

    QObject::connect(ui->cbIncLineNo, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->cbKwCase, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->cbPadZeroes, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->cbReformat, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->cbWrapping, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->cbAdvWrapping, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->cbValidateInput, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->cbEncoding, SIGNAL(clicked()), this, SLOT(updatePreview()));
    QObject::connect(ui->comboEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreview()));

    QObject::connect(ui->comboFontName, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreview()));
    QObject::connect(ui->comboKwCase, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreview()));
    QObject::connect(ui->comboReformat, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreview()));
    QObject::connect(ui->comboTheme, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreview()));
    QObject::connect(ui->comboSelectSyntax, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreview()));
    QObject::connect(ui->comboThemeFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePreview()));

    QObject::connect(ui->comboLSProfiles, SIGNAL(currentIndexChanged(int)), this, SLOT(loadLSProfile()));

    QObject::connect(ui->sbLineNoWidth, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
    QObject::connect(ui->sbLineNoStart, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
    QObject::connect(ui->leFontSize, SIGNAL(textChanged(QString)), this, SLOT(updatePreview()));
    QObject::connect(ui->cbOmitWrappedLineNumbers, SIGNAL(clicked()), this, SLOT(updatePreview()));

    QObject::connect(&scriptWatcher, SIGNAL(fileChanged(QString)), this, SLOT(updatePreview()));

    copyShortcut = new QShortcut(QKeySequence(QKeySequence::Copy), this);
    pasteShortcut = new QShortcut(QKeySequence(QKeySequence::Paste), this);

    QObject::connect(copyShortcut, SIGNAL(activated()), ui->pbCopyToCP, SLOT(click()));
    QObject::connect(pasteShortcut, SIGNAL(activated()), ui->pbPasteFromCB, SLOT(click()));

    ui->pbCopyToCP->setText(ui->pbCopyToCP->text().arg(QKeySequence(QKeySequence::Copy).toString(QKeySequence::NativeText)));
    ui->pbPasteFromCB->setText(ui->pbPasteFromCB->text().arg(QKeySequence(QKeySequence::Paste).toString(QKeySequence::NativeText)));

    setAcceptDrops(true);

    readSettings();

    fillThemeCombo(oldThemeIndex);

    twoPassOutFile=QDir::tempPath() + "/highlight_twopass.lua";

    plausibility();

    if (runFirstTime && shortNamesDisabled()) {
        QMessageBox::warning(this, tr("NTFS Short Names"), tr("NTFS short names may be disabled on your volumes. Highlight can not read input with multibyte file names if no short name is available. This information will no longer bother you."));
    }

    statusBar()->showMessage(tr("Always at your service"), 2500);
}

bool MainWindow::shortNamesDisabled() {
#ifdef Q_OS_WIN
    QSettings reg("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\FileSystem",
    QSettings::NativeFormat);
    int disableShortNames=reg.value("NtfsDisable8dot3NameCreation",0).toInt();
    return disableShortNames;
#else
    return false;
#endif
}

void MainWindow::fillThemeCombo(int restoreVal)
{
    QString catFilter = ui->comboThemeFilter->currentData().toString();
    bool isBase16 = ui->comboThemeFilter->currentIndex() >1;

    ui->comboTheme->clear();

    for (int i = 0; i < themesList.size(); ++i) {
        QList<QString> curTheme = themesList.at(i);
        if ( (!isBase16 && curTheme.at(2).contains( catFilter) && !curTheme.at(2).contains( "base16")) ||
             (isBase16 && curTheme.at(2).contains( catFilter) && curTheme.at(2).contains( "base16")))
          ui->comboTheme->addItem(curTheme.at(1), curTheme.at(0));
    }
    ui->comboTheme->setCurrentIndex(restoreVal);
}

MainWindow::~MainWindow()
{
    writeSettings();
    QFile::remove(twoPassOutFile);

    delete ui;
    delete copyShortcut;
    delete pasteShortcut;
}

void MainWindow::openFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
                            this,
                            tr("Select one or more files to open"),
                            "",
                            fileOpenFilter);

    addToView(files, ui->lvInputFiles);
}

void MainWindow::selectSingleFile(QLineEdit* edit, const QString& title, const QString& filter)
{
    QString fileName = QFileDialog::getOpenFileName(this, title, "", filter);
    if (!fileName.isEmpty()) edit->setText(fileName);
}

void MainWindow::addToView(const QStringList& list, QListWidget* listWidget, const QString& iconName, bool checkable, bool baseName)
{
    QListWidgetItem *listItem;
    QString croppedName;
    for (const auto& listElement : list) {

        for (int i = 0; i < listWidget->count(); ++i) {
            if (listWidget->item(i)->data(Qt::UserRole).toString()==listElement)
                return;
        }

        croppedName = baseName ? QFileInfo(listElement).baseName(): QFileInfo(listElement).fileName() ;
        if (croppedName.isEmpty()) break;
        listItem=new QListWidgetItem(QIcon(iconName), croppedName );
        listItem->setData(Qt::UserRole, listElement);
        listItem->setToolTip(listElement);
        if (checkable) listItem->setCheckState( Qt::Unchecked);
        listWidget->addItem(listItem );
    }
}

void MainWindow::on_pbOutputDest_clicked()
{
    ui->leOutputDest->setText(QFileDialog::getExistingDirectory(this, tr("Select destination directory"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks));
}


void MainWindow::readSettings()
{
    ui->tabIOSelection->setCurrentIndex(0);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "andre-simon.de", "highlight-gui");

    if (!QFile(settings.fileName()).exists()) return;

    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    settings.endGroup();

    settings.beginGroup("input");
    const char* name = "objectName";

    auto setCheckbox = [&](QCheckBox* box) {
        box->setChecked(settings.value(box->property(name).toString()).toBool());
    };

    auto setLineEdit = [&](QLineEdit* edit) {
        edit->setText(settings.value(edit->property(name).toString()).toString());
    };

    auto setComboBox = [&](QComboBox* combo, bool useString=false) {
        if (useString) {
            combo->insertItem(0, settings.value(combo->property(name).toString()).toString());
            combo->setCurrentIndex(0);
        } else {
            combo->setCurrentIndex(settings.value(combo->property(name).toString()).toInt());
        }
    };

    auto setSpinBox = [&](QSpinBox* spin) {
        spin->setValue(settings.value(spin->property(name).toString()).toInt());
    };

    auto setTabWidget = [&](QTabWidget* tab) {
       tab->setCurrentIndex(settings.value(tab->property(name).toString()).toInt());
    };

    addToView(settings.value(ui->lvInputFiles->property(name).toString()).toStringList(), ui->lvInputFiles);
    addToView(settings.value(ui->lvPluginScripts->property(name).toString()).toStringList(), ui->lvPluginScripts, ":/plugin.png", true, true);
    addToView(settings.value(ui->lvUserScripts->property(name).toString()).toStringList(), ui->lvUserScripts, ":/script.png", true, false);

    auto setSelectedItems = [&](QListWidget* view, const QString& key) {
        QVariantList selected = settings.value(key).toList();
        for (const auto& item : std::as_const(selected)) {
            view->item(item.toInt())->setCheckState(Qt::Checked);
            if (view == ui->lvUserScripts) on_lvUserScripts_itemClicked(view->item(item.toInt()));
        }
    };

    setSelectedItems(ui->lvPluginScripts, "selectedPlugins");
    setSelectedItems(ui->lvUserScripts, "selectedUserScripts");

    setLineEdit(ui->leOutputDest);
    setCheckbox(ui->cbWrite2Src);
    setCheckbox(ui->cbAdvWrapping);
    setCheckbox(ui->cbEncoding);
    setCheckbox(ui->cbFragment);
    setCheckbox(ui->cbKeepInjections);
    setCheckbox(ui->cbOmitVersionInfo);

    setCheckbox(ui->cbHTMLAnchors);
    setCheckbox(ui->cbHTMLEmbedStyle);
    setCheckbox(ui->cbHTMLEnclosePreTags);
    setCheckbox(ui->cbHTMLFileNameAnchor);
    setCheckbox(ui->cbHTMLIndex);
    setCheckbox(ui->cbHTMLInlineCSS);
    setCheckbox(ui->cbHTMLOrderedList);
    setCheckbox(ui->cbIncLineNo);
    setCheckbox(ui->cbKwCase);
    setCheckbox(ui->cbLATEXBabel);
    setCheckbox(ui->cbLATEXBeamer);
    setCheckbox(ui->cbLATEXEscQuotes);
    setCheckbox(ui->cbLATEXPrettySymbols);
    setCheckbox(ui->cbPadZeroes);
    setCheckbox(ui->cbReformat);
    setCheckbox(ui->cbRTFCharStyles);
    setCheckbox(ui->cbRTFPageColor);
    setCheckbox(ui->cbWrapping);
    setCheckbox(ui->cbValidateInput);
    setCheckbox(ui->cbLSHover);
    setCheckbox(ui->cbLSSemantic);
    setCheckbox(ui->cbLSSyntaxErrors);
    setCheckbox(ui->cbLSLegacy);

    setComboBox(ui->comboEncoding, true);
    setComboBox(ui->comboFontName, true);

    setComboBox(ui->comboFormat);
    setComboBox(ui->comboKwCase);
    setComboBox(ui->comboReformat);
    setComboBox(ui->comboRTFPageSize);
    setComboBox(ui->comboSelectSyntax);
    setComboBox(ui->comboLSProfiles);
    setComboBox(ui->comboThemeFilter);

    setLineEdit(ui->leHTMLStyleFile);
    setLineEdit(ui->leHTMLStyleIncFile);
    setLineEdit(ui->leLATEXStyleFile);
    setLineEdit(ui->leTEXStyleFile);
    setLineEdit(ui->leLATEXStyleIncFile);
    setLineEdit(ui->leTEXStyleIncFile);
    setLineEdit(ui->leSVGHeight);
    setLineEdit(ui->leSVGWidth);
    setLineEdit(ui->leFontSize);
    setLineEdit(ui->leHTMLCssPrefix);
    setLineEdit(ui->lePluginReadFilePath);
    setLineEdit(ui->leLSWorkspace);

    setSpinBox(ui->sbLineLength);
    setSpinBox(ui->sbTabWidth);
    setSpinBox(ui->sbLineNoWidth);
    setSpinBox(ui->sbLineNoStart);

    setTabWidget(ui->tabWidget);
    setTabWidget(ui->tabIOSelection);
    setTabWidget(ui->tabWidgetOptions);

    oldThemeIndex = settings.value(ui->comboTheme->property(name).toString()).toInt();

    runFirstTime = settings.value("runFirstTime", true).toBool();
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "andre-simon.de", "highlight-gui");

    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();

    settings.beginGroup("input");
    QStringList inFiles, plugins, userScripts;
    QVariantList selectedPlugins, selectedUserScripts;

    auto collectItems = [&](QListWidget* view, QStringList& list, QVariantList& selected) {
        for (int i = 0; i < view->count(); ++i) {
            list << view->item(i)->data(Qt::UserRole).toString();
            if (view->item(i)->checkState() == Qt::Checked)
                selected.push_back(QVariant(i));
        }
    };

    collectItems(ui->lvInputFiles, inFiles, selectedPlugins);
    collectItems(ui->lvPluginScripts, plugins, selectedPlugins);
    collectItems(ui->lvUserScripts, userScripts, selectedUserScripts);

    const char* name = "objectName";
    settings.setValue(ui->lvInputFiles->property(name).toString(), inFiles);
    settings.setValue(ui->lvPluginScripts->property(name).toString(), plugins);
    settings.setValue("selectedPlugins", selectedPlugins);
    settings.setValue(ui->lvUserScripts->property(name).toString(), userScripts);
    settings.setValue("selectedUserScripts", selectedUserScripts);

    auto saveCheckbox = [&](QCheckBox* box) {
        settings.setValue(box->property(name).toString(), box->isChecked());
    };

    auto saveLineEdit = [&](QLineEdit* edit) {
        settings.setValue(edit->property(name).toString(), edit->text());
    };

    auto saveComboBox = [&](QComboBox* combo, bool useString = false) {
        if (useString)
            settings.setValue(combo->property(name).toString(), combo->currentText());
        else
            settings.setValue(combo->property(name).toString(), combo->currentIndex());
    };

    auto saveSpinBox = [&](QSpinBox* spin) {
        settings.setValue(spin->property(name).toString(), spin->value());
    };

    auto saveTab = [&](QTabWidget* tab) {
        settings.setValue(tab->property(name).toString(), tab->currentIndex());
    };

    saveCheckbox(ui->cbWrite2Src);
    saveCheckbox(ui->cbAdvWrapping);
    saveCheckbox(ui->cbEncoding);
    saveCheckbox(ui->cbFragment);
    saveCheckbox(ui->cbKeepInjections);
    saveCheckbox(ui->cbHTMLAnchors);
    saveCheckbox(ui->cbOmitVersionInfo);
    saveCheckbox(ui->cbHTMLEmbedStyle);
    saveCheckbox(ui->cbHTMLEnclosePreTags);
    saveCheckbox(ui->cbHTMLFileNameAnchor);
    saveCheckbox(ui->cbHTMLIndex);
    saveCheckbox(ui->cbHTMLInlineCSS);
    saveCheckbox(ui->cbHTMLOrderedList);
    saveCheckbox(ui->cbIncLineNo);
    saveCheckbox(ui->cbKwCase);
    saveCheckbox(ui->cbLATEXBabel);
    saveCheckbox(ui->cbLATEXBeamer);
    saveCheckbox(ui->cbLATEXEscQuotes);
    saveCheckbox(ui->cbLATEXPrettySymbols);
    saveCheckbox(ui->cbPadZeroes);
    saveCheckbox(ui->cbReformat);
    saveCheckbox(ui->cbRTFCharStyles);
    saveCheckbox(ui->cbRTFPageColor);
    saveCheckbox(ui->cbWrapping);
    saveCheckbox(ui->cbHTMLPasteMIME);

    saveCheckbox(ui->cbLATEXEmbedStyle);
    saveCheckbox(ui->cbTEXEmbedStyle);
    saveCheckbox(ui->cbSVGEmbedStyle);
    saveCheckbox(ui->cbLSHover);
    saveCheckbox(ui->cbLSSemantic);
    saveCheckbox(ui->cbLSSyntaxErrors);
    saveCheckbox(ui->cbLSLegacy);
    saveCheckbox(ui->cbValidateInput);

    saveComboBox(ui->comboFormat);
    saveComboBox(ui->comboKwCase);
    saveComboBox(ui->comboReformat);
    saveComboBox(ui->comboRTFPageSize);
    saveComboBox(ui->comboLSProfiles);
    saveComboBox(ui->comboTheme);
    saveComboBox(ui->comboThemeFilter);
    saveComboBox(ui->comboSelectSyntax);
    saveComboBox(ui->comboEncoding, true);
    saveComboBox(ui->comboFontName, true);

    saveLineEdit(ui->leOutputDest);
    saveLineEdit(ui->leHTMLStyleFile);
    saveLineEdit(ui->leHTMLStyleIncFile);
    saveLineEdit(ui->leLATEXStyleFile);
    saveLineEdit(ui->leLATEXStyleIncFile);
    saveLineEdit(ui->leTEXStyleFile);
    saveLineEdit(ui->leTEXStyleIncFile);
    saveLineEdit(ui->leSVGStyleFile);
    saveLineEdit(ui->leSVGStyleIncFile);
    saveLineEdit(ui->lePluginReadFilePath);
    saveLineEdit(ui->leSVGHeight);
    saveLineEdit(ui->leSVGWidth);
    saveLineEdit(ui->leFontSize);
    saveLineEdit(ui->leLSWorkspace);
    saveLineEdit(ui->leHTMLCssPrefix);

    saveSpinBox(ui->sbLineLength);
    saveSpinBox(ui->sbTabWidth);
    saveSpinBox(ui->sbLineNoWidth);
    saveSpinBox(ui->sbLineNoStart);

    saveTab(ui->tabWidget);
    saveTab(ui->tabIOSelection);
    saveTab(ui->tabWidgetOptions);

    settings.setValue("runFirstTime", false);

    settings.endGroup();
}


void MainWindow::readLuaList(const string& paramName, const string& langName,Diluculum::LuaValue &luaVal, MMap* extMap){
    int extIdx=1;
    string val;
    while (luaVal[paramName][extIdx] !=Diluculum::Nil) {
        val = luaVal[paramName][extIdx].asString();
        extMap->insert ( make_pair ( val,  langName ) );
        extIdx++;
    }
}


bool MainWindow::loadLSProfiles()
{
    QString confPath=getDistFileConfigPath(QString("lsp.conf"));
#ifdef Q_OS_WIN
   confPath = getWindowsShortPath(confPath);
#endif

   try {
       Diluculum::LuaState ls;
       Diluculum::LuaValueList ret= ls.doFile (confPath.toStdString());

       int idx=1;
       std::string serverName;              ///< server name
       std::string executable;              ///< server executable path
       std::string syntax;                  ///< language definition which can be enhanced using the LS
       int delay=0;                         ///< server delay in milliseconds after initialization request
       bool legacy=false;                   ///< do not rely on a LS capabilities response
       std::vector<std::string> options;    ///< server executable start options
       Diluculum::LuaValue mapEntry;

       //{ Server="clangd", Exec="clangd", Syntax="c", Options={"--log=error"} },
       while ((mapEntry = ls["Servers"][idx].value()) !=Diluculum::Nil) {
           options.clear();
           serverName = mapEntry["Server"].asString();
           executable = mapEntry["Exec"].asString();
           syntax = mapEntry["Syntax"].asString();

           if (mapEntry["Options"] !=Diluculum::Nil) {
               int extIdx=1;
               while (mapEntry["Options"][extIdx] !=Diluculum::Nil) {
                   options.push_back(mapEntry["Options"][extIdx].asString());
                   extIdx++;
               }
           }

           delay = 0;
           if (mapEntry["Delay"] !=Diluculum::Nil) {
               delay = mapEntry["Delay"].asNumber();
           }

           legacy = false;
           if (mapEntry["Legacy"] !=Diluculum::Nil) {
                legacy = mapEntry["Legacy"].asBoolean();
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

       QMessageBox::warning(this, "Configuration error", QString::fromStdString( err.what()));
       return false;
   }
    return true;
}

bool MainWindow::loadFileTypeConfig()
{
    QString filetypesPath=getDistFileConfigPath(QString("filetypes.conf"));
#ifdef Q_OS_WIN
   filetypesPath = getWindowsShortPath(filetypesPath);
#endif

    try {
        Diluculum::LuaState ls;
        ls.doFile (filetypesPath.toStdString());
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
                encodingHints.insert ( make_pair ( langName, mapEntry["EncodingHint"].asString() ) );
            }
            idx++;
        }
    } catch (Diluculum::LuaError &err) {
        QMessageBox::warning(this, "Configuration error", QString::fromStdString( err.what()));
        return false;
    }
    return true;
}

string MainWindow::analyzeFile(const string& file)
{
    ifstream inFile(file.c_str());
    string firstLine;
    getline (inFile, firstLine);
    SMap::iterator it;

    boost::xpressive::sregex rex;
    boost::xpressive::smatch what;
    for ( it=assocByShebang.begin(); it!=assocByShebang.end(); it++ ) {
        rex = boost::xpressive::sregex::compile( it->first );
        if ( boost::xpressive::regex_search( firstLine, what, rex )  ) return it->second;
    }
    return "";
}

string MainWindow::getFileType(const string& suffix, const string &inputFile)
{
    string baseName = getFileBaseName(inputFile);
    if (assocByFilename.count(baseName)) return assocByFilename.find(baseName)->second;

    string lcSuffix = StringTools::change_case(suffix);
    if (assocByExtension.count(lcSuffix)) {

        string langAssociation;
        auto it = assocByExtension.find(lcSuffix);
        if (it!=assocByExtension.end()) langAssociation = it->second;

        std::pair <MMap::iterator, MMap::iterator> ret;
        ret = assocByExtension.equal_range(lcSuffix);

        std::list<std::string> lst;
        for (auto it=ret.first; it!=ret.second; ++it){
           lst.push_back( it->second );
        }

        if (lst.size() > 1){

            if (rememberedAssoc.count(lcSuffix)) return rememberedAssoc[lcSuffix];

            syntax_chooser chooser;
            chooser.setUnclearExtension(QString(lcSuffix.c_str()));
             for (auto it=lst.begin(); it != lst.end(); ++it){
                 chooser.addSyntaxName(QString((*it).c_str()));
            }
            chooser.exec();

            string selectedLang = chooser.getSelection().toStdString();

            if (chooser.getRememberFlag()){
                rememberedAssoc[lcSuffix]=selectedLang;
            }
            return selectedLang;
        }
        return langAssociation;
    }

    // Not configured in filetypes.conf, try shebang and finally the unmodified extension
    string shebang = analyzeFile(inputFile);
    if (!shebang.empty()) return shebang;
    return lcSuffix;
}

string MainWindow::getFileBaseName(const string& fileName){
    size_t psPos = fileName.rfind ( /*Platform::pathSeparator*/ '/' );
    return  (psPos==string::npos) ? fileName : fileName.substr(psPos+1, fileName.length());
}

string MainWindow::getFileSuffix(const string& fileName)
{
    size_t ptPos=fileName.rfind(".");
    size_t psPos = fileName.rfind ( /*Platform::pathSeparator*/ '/' );

    if (ptPos == string::npos) {
        return  (psPos==string::npos) ? fileName : fileName.substr(psPos+1, fileName.length());
    }
    return (psPos!=string::npos && psPos>ptPos) ? fileName.substr(psPos+1, fileName.length()) : fileName.substr(ptPos+1, fileName.length());
}

void MainWindow::on_action_Exit_triggered()
{
    this->close();
}

void MainWindow::on_action_About_Highlight_triggered()
{
    QMessageBox::about( this, "About Highlight",
                        QString("Highlight is a code to formatted text converter.\n\n"
                                "Highlight GUI %1\n"
                                "Copyright (C) 2002-2026 Andre Simon <a.simon at mailbox.org>\n\n"
                                "Artistic Style Classes\n(C) 1998-2026 The Artistic Style Authors\n\n"
                                "Diluculum Lua wrapper\n"
                                "Copyright (C) 2005-2013 by Leandro Motta Barros\n\n"
                                "xterm 256 color matching functions\n"
                                "Copyright (C) 2006 Wolfgang Frisch <wf at frexx.de>\n\n"
                                "PicoJSON library\n"
                                "Copyright (C) 2009-2010 Cybozu Labs, Inc.\n"
                                "Copyright (C) 2011-2014 Kazuho Oku\n\n"
                                "Built with Qt version %2\n\n"
                                "Released under the terms of the GNU GPL license.\n\n"
                                "The highlight logo is based on the image \"Alcedo Atthis\" by Lukasz Lukasik.\n"
                                "The original was published under the terms of the GNU FDL in the Wikimedia Commons database.\n\n"
                        ).arg(QString::fromStdString(highlight::Info::getVersion())).arg(QString(qVersion ())) );
}

highlight::OutputType MainWindow::getOutputType()
{
    switch (ui->comboFormat->currentIndex()) {
    case 0:
        return highlight::HTML;
    case 1:
        return highlight::XHTML;
    case 2:
        return highlight::LATEX;
    case 3:
        return highlight::TEX;
    case 4:
        return highlight::RTF;
    case 5:
        return highlight::ODTFLAT;
    case 6:
        return highlight::SVG;
    case 7:
        return highlight::BBCODE;

    case 8:
        return highlight::ESC_ANSI;
    case 9:
        return highlight::ESC_XTERM256;
    case 10:
        return highlight::ESC_TRUECOLOR;
    }
    return highlight::HTML;
}

QString MainWindow::getOutFileSuffix()
{
    switch (ui->comboFormat->currentIndex()) {
    case 0:
        return ".html";
    case 1:
        return ".xhtml";
    case 2:
    case 3:
        return ".tex";
    case 4:
        return ".rtf";
    case 5:
        return ".fodt";
    case 6:
        return ".svg";
    case 7:
        return ".bbcode";
    case 8:
    case 9:
    case 10:
        return ".esc";
    }
    return ".html";
}

void MainWindow::applyCtrlValues(highlight::CodeGenerator* generator, bool previewMode)
{
    if (!generator) return;

    highlight::OutputType outType=getOutputType();

    if (!previewMode) {
        QLineEdit * styleIncFile=NULL;
        QLineEdit * styleOutFile=NULL;
        QCheckBox * embedStyle=NULL;

        switch(outType) {
        case highlight::HTML:
        case highlight::XHTML:
            styleIncFile = ui->leHTMLStyleIncFile;
            styleOutFile = ui->leHTMLStyleFile;
            embedStyle  = ui->cbHTMLEmbedStyle;
            break;
        case highlight::LATEX:
            styleIncFile = ui->leLATEXStyleIncFile;
            styleOutFile = ui->leLATEXStyleFile;
            embedStyle  = ui->cbLATEXEmbedStyle;
            break;
        case highlight::TEX:
            styleIncFile = ui->leTEXStyleIncFile;
            styleOutFile = ui->leTEXStyleFile;
            embedStyle  = ui->cbTEXEmbedStyle;
            break;
        case highlight::SVG:
            styleIncFile = ui->leSVGStyleIncFile;
            styleOutFile = ui->leSVGStyleFile;
            embedStyle  = ui->cbSVGEmbedStyle;
            break;
        default:
            break;
        }
        if (styleIncFile)
            generator->setStyleInputPath(QDir::toNativeSeparators(styleIncFile->text()).toStdString());
        if (styleOutFile)
            generator->setStyleOutputPath(styleOutFile->text().toStdString());
        if (embedStyle)
            generator->setIncludeStyle(embedStyle->isChecked());

        generator->setFragmentCode(ui->cbFragment->isChecked());
        generator->setKeepInjections(ui->cbKeepInjections->isChecked());

        generator->setHTMLAttachAnchors(ui->cbHTMLAnchors->isChecked());
        generator->setHTMLOrderedList(ui->cbIncLineNo->isChecked() && ui->cbHTMLOrderedList->isChecked());
        generator->setHTMLInlineCSS(ui->cbHTMLInlineCSS->isChecked() && ui->cbHTMLInlineCSS->isEnabled());
        generator->setHTMLEnclosePreTag(ui->cbHTMLEnclosePreTags->isChecked());

        if (ui->leHTMLCssPrefix->text().size())
            generator->setHTMLClassName(ui->leHTMLCssPrefix->text().toStdString());

        generator->setLATEXReplaceQuotes(ui->cbLATEXEscQuotes->isChecked());
        generator->setLATEXNoShorthands(ui->cbLATEXBabel->isChecked());
        generator->setLATEXPrettySymbols(ui->cbLATEXPrettySymbols->isChecked());
        generator->setLATEXBeamerMode(ui->cbLATEXBeamer->isChecked());

        generator->setRTFPageSize(ui->comboRTFPageSize->currentText().toLower().toStdString());
        generator->setRTFCharStyles(ui->cbRTFCharStyles->isChecked());
        generator->setRTFPageColor(ui->cbRTFPageColor->isChecked());

        generator->setSVGSize(ui->leSVGWidth->text().toStdString(), ui->leSVGHeight->text().toStdString());
    }

    generator->setPrintLineNumbers( ui->cbIncLineNo->isChecked(), ui->sbLineNoStart->value());
    generator->setPrintZeroes(ui->cbPadZeroes->isEnabled() && ui->cbPadZeroes->isChecked());

    generator->setPluginParameter(ui->lePluginReadFilePath->text().toStdString());
    generator->setOmitVersionComment(ui->cbHTMLPasteMIME->isChecked() || ui->cbOmitVersionInfo->isChecked());

    generator->setFilesCnt( previewMode ? 1 :  ui->lvInputFiles->count() );

    QString themePath=getUserScriptPath("theme");

    if (themePath.isEmpty()){
        themePath=getDistThemePath();
    }

#ifdef Q_OS_WIN
        themePath = getWindowsShortPath(themePath);
#endif

    for (int i=0; i<ui->lvPluginScripts->count(); i++) {
        if (ui->lvPluginScripts->item(i)->checkState()==Qt::Checked) {
            QString pluginPath=ui->lvPluginScripts->item(i)->data(Qt::UserRole).toString();
#ifdef Q_OS_WIN
        pluginPath = getWindowsShortPath(pluginPath);
#endif
            if (!generator->initPluginScript(pluginPath.toStdString()) ) {
                QMessageBox::critical(this,"Plug-In init error", QString::fromStdString(generator->getPluginScriptError()));
            }
        }
    }

    if (!generator->initTheme(themePath.toStdString(), ui->cbLSSemantic->isChecked() )) {
        QMessageBox::critical(this,"Theme init error", QString::fromStdString(generator->getThemeInitError()));
    }

    generator->setValidateInput(ui->cbValidateInput->isChecked());
    generator->setLineNumberWidth(ui->sbLineNoWidth->value());
    generator->setNumberWrappedLines(!ui->cbOmitWrappedLineNumbers->isChecked());

    if (getOutputType()!=highlight::LATEX && getOutputType()!=highlight::TEX) {
        string fntName=ui->comboFontName->currentText().toStdString();
        if(fntName.size()) generator->setBaseFont(fntName);
        string fntSize=ui->leFontSize->text().toStdString();
        if(fntSize.size()) generator->setBaseFontSize(fntSize);
    }

    if ( !ui->cbLSSemantic->isChecked() && !ui->cbLSHover->isChecked()) {
        int lineLength = 0;
        if (ui->cbWrapping->isChecked()) {
            lineLength = (   ui->cbIncLineNo->isChecked()
                             && ui->sbLineNoWidth->value() >  ui->sbLineLength->value())?
                         ui->sbLineLength->value() - ui->sbLineNoWidth->value()
                         :  ui->sbLineLength->value();
        }
        generator->setPreformatting(getWrappingStyle(), lineLength, ui->sbTabWidth->value());
    }

    if(ui->cbKwCase->isChecked()) {
        StringTools::KeywordCase kwCase=StringTools::CASE_UNCHANGED;
        switch (ui->comboKwCase->currentIndex()) {
        case 0:
            kwCase=StringTools::CASE_UPPER;
            break;
        case 1:
            kwCase=StringTools::CASE_LOWER;
            break;
        case 2:
            kwCase=StringTools::CASE_CAPITALIZE;
            break;
        }
        generator->setKeyWordCase(kwCase);
    }

    if (ui->cbReformat->isChecked() && !ui->cbLSSemantic->isChecked() && !ui->cbLSHover->isChecked()) {
        generator->initIndentationScheme(ui->comboReformat->currentText().toLower().toStdString());
    }
}

highlight::WrapMode MainWindow::getWrappingStyle()
{
    if (!ui->cbAdvWrapping->isChecked() && ui->cbAdvWrapping->isEnabled())
        return highlight::WRAP_SIMPLE;
    return (ui->cbWrapping->isChecked())?highlight::WRAP_DEFAULT:highlight::WRAP_DISABLED;
}

void MainWindow::applyEncoding(highlight::CodeGenerator* generator, QString &langDefPath) {
    if (ui->cbEncoding->isChecked()) {
        generator->setEncoding(ui->comboEncoding->currentText().toStdString());
    } else {

        string encoding="none";
        //syntax definition setting:
        string encodingHint= generator->getSyntaxEncodingHint();
        if (encodingHint.size())
            encoding=encodingHint;

        // filetypes.conf setting has higher priority:
        encodingHint= encodingHints[QFileInfo(langDefPath).baseName().toStdString()];
        if (encodingHint.size())
            encoding=encodingHint;

        generator->setEncoding(encoding);
    }
}

void MainWindow::on_pbStartConversion_clicked()
{
    if (!ui->lvInputFiles->count()) return;

    if (!ui->cbWrite2Src->isChecked() ) {

        if (ui->leOutputDest->text().isEmpty()){
            ui->pbOutputDest->click();
            return;
        }
        if (!QDir(ui->leOutputDest->text()).exists()){
            QMessageBox::warning(this, tr("Output error"), tr("Output directory does not exist!"));
            ui->leOutputDest->setFocus();
            return;
        }
    }

    highlight::OutputType outType = getOutputType();
    QCheckBox* cbEmbed=NULL;
    QLineEdit* leStyleFile=NULL;

    if ( outType==highlight::HTML || outType==highlight::XHTML) {
        cbEmbed = ui->cbHTMLEmbedStyle;
        leStyleFile = ui->leHTMLStyleFile;
    } else if ( outType==highlight::LATEX ) {
        cbEmbed = ui->cbLATEXEmbedStyle;
        leStyleFile = ui->leLATEXStyleFile;
    } else if (outType==highlight::TEX ) {
        cbEmbed = ui->cbTEXEmbedStyle;
        leStyleFile = ui->leTEXStyleFile;
    }    else if (outType==highlight::SVG ) {
        cbEmbed = ui->cbSVGEmbedStyle;
        leStyleFile = ui->leSVGStyleFile;
    }

    if (cbEmbed && leStyleFile ) {
        if ( !cbEmbed->isChecked() && leStyleFile->text().isEmpty()) {
            ui->tabWidget->setCurrentIndex(1);
            if ( outType==highlight::HTML || outType==highlight::XHTML) {
                ui->tabWidget->setCurrentIndex(0);
            }
            leStyleFile->setFocus();
            QMessageBox::warning(this, tr("Output error"), tr("You must define a style output file!"));
            return;
        }
    }
    ui->pbStartConversion->setDisabled(true);
    ui->pbCopyFile2CP->setDisabled(true);
    this->setCursor(Qt::WaitCursor);

    QElapsedTimer t;
    t.start();

    std::unique_ptr<highlight::CodeGenerator> generator(highlight::CodeGenerator::getInstance(outType));

    applyCtrlValues(generator.get(), false);
    string currentFile;
    string outfileName;
    highlight::ParseError error;
    highlight::LoadResult loadRes;
    bool twoPassMode=false;

    QString langDefPath;
    QString userLangPath = getUserScriptPath("lang");

    QString inFileName, inFilePath, origFilePath;
    QString lastStyleDestDir;
    QSet<QString> usedFileNames;

    QStringList inputErrors, outputErrors, reformatErrors, syntaxTestErrors;

    bool usesLSClient=false;
    bool useLSForInput=false;

    int i=-1;
    while ( ++i<ui->lvInputFiles->count()) {

        if (i==0 && twoPassMode) {

             if ( !generator->initPluginScript(twoPassOutFile.toStdString()) ) {
                 QMessageBox::critical(this,"Plug-In init error", QString::fromStdString(generator->getPluginScriptError()));
                 break;
            }
        }

        inFilePath = origFilePath =  ui->lvInputFiles->item(i)->data(Qt::UserRole).toString();

#ifdef Q_OS_WIN

        inFilePath = getWindowsShortPath(inFilePath);

#endif
        currentFile = inFilePath.toLocal8Bit().data();

        statusBar()->showMessage(tr("Processing %1 (%2/%3)").arg(origFilePath).arg(i+1).arg(ui->lvInputFiles->count()));

        langDefPath = (userLangPath.isEmpty()) ? getDistLangPath(getFileType(getFileSuffix(currentFile), currentFile)) : userLangPath;

#ifdef Q_OS_WIN
        langDefPath = getWindowsShortPath(langDefPath);
#endif

        loadRes=generator->loadLanguage(langDefPath.toStdString());
        if (loadRes==highlight::LOAD_FAILED_REGEX) {
            QMessageBox::warning(this, tr("Language definition error"),
                                 tr("Invalid regular expression in %1:\n%2").arg(langDefPath).arg(
                                     QString::fromStdString( generator->getSyntaxRegexError())));
            break;
        } else if (loadRes==highlight::LOAD_FAILED) {
            QMessageBox::warning(this, tr("Unknown syntax"), tr("Could not convert %1").arg(origFilePath));
            inputErrors.append(origFilePath);
        } else  if (loadRes==highlight::LOAD_FAILED_LUA) {
            QMessageBox::warning(this, tr("Lua error"), tr("Could not convert %1:\nLua Syntax error: %2").arg(origFilePath).arg(QString::fromStdString(generator->getSyntaxLuaError())) );
            inputErrors.append(origFilePath);
        } else {

            if (twoPassMode && !generator->syntaxRequiresTwoPassRun()) {
                 continue;
            }

            if (ui->cbReformat->isChecked()&& !generator->formattingIsPossible()) {
                reformatErrors.append(inFilePath);
            }

            inFileName = ui->lvInputFiles->item(i)->text();
            if (ui->cbWrite2Src->isChecked()) {

                QString absOutPath=origFilePath + getOutFileSuffix();

#ifdef Q_OS_WIN
                QFile file( absOutPath );
                if ( file.open(QIODevice::ReadWrite) )
                {
                    absOutPath = getWindowsShortPath(absOutPath);
                    file.close();
                }
#endif
                outfileName = absOutPath.toLocal8Bit().data();
            } else {
                QFileInfo outFileInfo;
                QString fName=inFileName;
                if (usedFileNames.contains(fName)) {

                    QString prefix=QFileInfo(inFilePath).canonicalPath()+ QDir::separator();
#ifdef Q_OS_WIN
                    prefix.remove(0,2); // Get rid of drive letter and ':'
#endif
                    prefix.replace('/', '_');
                    prefix.replace('\\', '_');
                    fName.insert(0, prefix);

                } else {
                    usedFileNames.insert(fName);
                }
                outFileInfo.setFile(ui->leOutputDest->text(),fName);

                QString absOutPath=outFileInfo.absoluteFilePath() + getOutFileSuffix();

#ifdef Q_OS_WIN
                QFile file( absOutPath );
                if ( file.open(QIODevice::ReadWrite) )
                {
                    absOutPath = getWindowsShortPath(absOutPath);
                    file.close();
                }
#endif
                outfileName = absOutPath.toLocal8Bit().data();
            }

            if (ui->cbHTMLFileNameAnchor->isChecked()) {
                generator->setHTMLAnchorPrefix(inFileName.toStdString());
            }
            generator->setTitle(inFileName.toStdString());

            applyEncoding(generator.get(), langDefPath);

            string syntaxName = QFileInfo(langDefPath).baseName().toStdString();

            if ((ui->cbLSSemantic->isChecked() || ui->cbLSHover->isChecked()) && usesLSClient==false && lsSyntax==syntaxName) {
                if (initializeLS(generator.get(), false )) {
                        usesLSClient=true;
                }
            }

            useLSForInput = usesLSClient && lsSyntax==syntaxName;

            if ( useLSForInput ) {

                generator->lsAddSyntaxErrorInfo(  ui->cbLSSyntaxErrors->isChecked() );

                generator->lsAddHoverInfo( ui->cbLSHover->isChecked() );

                generator->lsOpenDocument(currentFile, syntaxName);

                if (ui->cbLSSemantic->isChecked())
                    generator->lsAddSemanticInfo(currentFile, syntaxName);
            }

            error = generator->generateFile(currentFile, outfileName );
            if (error != highlight::PARSE_OK) {
                if (error == highlight::BAD_INPUT) {
                    inputErrors.append(origFilePath);
                } else {
                    outputErrors.append(origFilePath);
                }
            }
            if (useLSForInput) {
                generator->lsCloseDocument(currentFile, generator->getSyntaxDescription());
            }
            ui->progressBar->setValue(100*i / ui->lvInputFiles->count());
        }

        if (i==ui->lvInputFiles->count()-1 && generator->requiresTwoPassParsing() && twoPassOutFile.size()
                    && inputErrors.size()==0 && outputErrors.size()==0 && !twoPassMode) {

                bool success=generator->printPersistentState(twoPassOutFile.toStdString());
                if ( !success ) {
                    outputErrors.append(twoPassOutFile);
                } else {
                    twoPassMode=true;

                    //start over, add plug-in to list in next iteration
                    usedFileNames.clear();
                    generator->resetSyntaxReaders();
                    i=-1;
                    ui->progressBar->setValue(0);
                }
         }
    }

    generator->clearPersistentSnippets();

    if (usesLSClient) {
        generator->exitLanguageServer();
    }

    // write external Stylesheet
    if ( cbEmbed && leStyleFile && !cbEmbed->isChecked()) {

        QString styleDestDir(ui->cbWrite2Src->isChecked() ? QFileInfo(inFilePath).path() : ui->leOutputDest->text() );

        if (lastStyleDestDir!=styleDestDir){
            lastStyleDestDir = styleDestDir;
            QString stylePath=QFileInfo(styleDestDir, leStyleFile->text()).absoluteFilePath();

#ifdef Q_OS_WIN
            QFile file( stylePath );
            if ( file.open(QIODevice::ReadWrite) )
            {
                stylePath = getWindowsShortPath(stylePath);
            }
#endif
            bool styleFileOK=generator -> printExternalStyle(QDir::toNativeSeparators(stylePath).toStdString());
            if (!styleFileOK) {
                outputErrors.append(stylePath);
            }
        }
    }

    // write HTML index file
    if (    (outType==highlight::HTML || outType==highlight::XHTML)
            && ui->cbHTMLIndex->isChecked() && !ui->cbWrite2Src->isChecked()) {
        vector <string>  fileList;
        for (int i=0; i<ui->lvInputFiles->count(); i++) {
            fileList.push_back(QDir::toNativeSeparators(ui->lvInputFiles->item(i)->data(Qt::UserRole).toString()).toStdString());
        }
        QString outPath = QDir::toNativeSeparators(ui->leOutputDest->text());
        if (!outPath.endsWith(QDir::separator())) outPath.append(QDir::separator());
        outPath.append("index");
        outPath.append(getOutFileSuffix());
#ifdef Q_OS_WIN
                QFile file( outPath );
                if ( file.open(QIODevice::ReadWrite) )
                {
                    outPath = getWindowsShortPath(outPath);
                }
#endif

        bool indexFileOK=generator->printIndexFile(fileList, outPath.toStdString());
        if (!indexFileOK) {
            outputErrors.append(outPath);
        }
    }

    auto posTestErrors = generator->getPosTestErrors();
    for (const auto& error : posTestErrors ) {
           syntaxTestErrors.append(QString::fromStdString(error));
    }

    statusBar()->showMessage(tr("Converted %1 files in %2 ms").arg(ui->lvInputFiles->count()).arg(t.elapsed()));
    ui->progressBar->reset();
    this->setCursor(Qt::ArrowCursor);
    ui->pbStartConversion->setEnabled(true);
    ui->pbCopyFile2CP->setEnabled(true);

    if (!inputErrors.isEmpty() || !outputErrors.isEmpty() || !reformatErrors.isEmpty() || !syntaxTestErrors.isEmpty()) {
        io_report report;
        report.addInputErrors(inputErrors);
        report.addOutputErrors(outputErrors);
        report.addReformatErrors(reformatErrors);
        report.addSyntaxTestErrors(syntaxTestErrors);

        report.exec();
        if (report.removeInputErrorFiles()) {

            for (int i=0; i<inputErrors.count(); i++) {
                for (int j = 0 ; j < ui->lvInputFiles->count(); ++j) {
                    if (ui->lvInputFiles->item(j)->data(Qt::UserRole).toString()==inputErrors.at(i)) {
                        delete ui->lvInputFiles->item(j);
                        break;
                    }
                }
            }
        }
    }
}

void MainWindow::on_pbCopyFile2CP_clicked()
{
    highlight2Clipboard(false);
}

void MainWindow::on_browserPreview_selectionChanged(){
    int blockStart=ui->browserPreview->textCursor().selectionStart();
    int blockEnd=ui->browserPreview->textCursor().selectionEnd();
    ui->cbCopyRange->setEnabled(blockStart!=blockEnd);
}

void MainWindow::highlight2Clipboard(bool getDataFromCP)
{

    if ((!getDataFromCP && NULL==ui->lvInputFiles->currentItem())
            || (getDataFromCP && savedClipboardContent.isEmpty())) return;

    this->setCursor(Qt::WaitCursor);

    unique_ptr<highlight::CodeGenerator> generator(
        highlight::CodeGenerator::getInstance(getOutputType()));

    applyCtrlValues(generator.get(), false);

    bool applyLS=false;
    string suffix;
    string currentFile;
    QString previewFilePath = (getDataFromCP) ? "": ui->lvInputFiles->currentItem()->data(Qt::UserRole).toString();

    if (getDataFromCP) {

        suffix= getFileType((ui->comboSelectSyntax->itemData(ui->comboSelectSyntax->currentIndex())).toString().toStdString(),"");

        if (ui->cbCopyRange->isEnabled() && ui->cbCopyRange->isChecked()) {
            int blockStart=ui->browserPreview->textCursor().selectionStart();
            int blockEnd=ui->browserPreview->textCursor().selectionEnd();
            if (blockStart != blockEnd) {
                QTextCursor selCursor=ui->browserPreview->textCursor();
                selCursor.setPosition(blockStart);
                int lineStart=selCursor.blockNumber();
                selCursor.setPosition(blockEnd-1); // -1 to enable line selection by triple click
                int lineEnd=selCursor.blockNumber() - lineStart + 1;

                if (lineEnd>0) {
                    generator->setStartingInputLine(lineStart+1);
                    generator->setMaxInputLineCnt(lineEnd);
                }
            }
        }
    } else {

#ifdef Q_OS_WIN

        previewFilePath = getWindowsShortPath(previewFilePath);

#endif

        currentFile = previewFilePath.toStdString();
        suffix = getFileType(getFileSuffix(currentFile), currentFile);

        QString inFileName = QFileInfo(previewFilePath).fileName();
        generator->setTitle(inFileName.toStdString());

        applyLS = (ui->cbLSSemantic->isChecked() || ui->cbLSHover->isChecked())
                && !currentFile.empty() && lsSyntax==suffix;
    }

    QString langPath = getUserScriptPath("lang");
    if (langPath.isEmpty()) {
        langPath = getDistLangPath(suffix);
    }

#ifdef Q_OS_WIN
        langPath = getWindowsShortPath(langPath);
#endif

    QString clipBoardData;

    for (int twoPass=0; twoPass<2; twoPass++) {

        if ( generator->loadLanguage(langPath.toStdString()) != highlight::LOAD_FAILED) {

            applyEncoding(generator.get(), langPath);

            if ( applyLS ) {

                if (initializeLS(generator.get(), false )) {

                    generator->lsAddSyntaxErrorInfo(  ui->cbLSSyntaxErrors->isChecked() );
                    generator->lsAddHoverInfo( ui->cbLSHover->isChecked() );
                    generator->lsOpenDocument(currentFile, suffix);

                    if (ui->cbLSSemantic->isChecked())
                        generator->lsAddSemanticInfo(currentFile, suffix);
                }
            }

            if (getDataFromCP) {
                clipBoardData= QString::fromStdString( generator->generateString(savedClipboardContent.toStdString()));
            } else {
                clipBoardData= QString::fromStdString( generator->generateStringFromFile(previewFilePath.toStdString()));
            }

            if ( twoPass==0 ) {

                if (generator->requiresTwoPassParsing()) {

                        if (generator->printPersistentState(twoPassOutFile.toStdString())) {
                            generator->resetSyntaxReaders();
                            generator->initPluginScript(twoPassOutFile.toStdString());
                        }
                        continue;
                } else {

                    //TODO method
                    QClipboard *clipboard = QApplication::clipboard();
                    if (clipboard) {
                        highlight::OutputType outputType = getOutputType();
                        if ( outputType==highlight::RTF) {
                            auto *mimeData = new QMimeData();

                            #ifdef Q_OS_WIN
                            mimeData->setData("Rich Text Format", clipBoardData.toLatin1());
                            #else
                            mimeData->setData("text/rtf", clipBoardData.toLatin1());
                            #endif

                            clipboard->setMimeData(mimeData);
                        }
                        else if ( (outputType==highlight::HTML || outputType==highlight::XHTML) && ui->cbHTMLPasteMIME->isChecked()) {
                            auto *mimeData = new QMimeData();

                                if (ui->cbEncoding->isChecked() && ui->comboEncoding->currentText().toLower()=="utf-8") {
                                    mimeData->setHtml(clipBoardData.toUtf8());
                                } else {
                                    mimeData->setHtml(clipBoardData.toLatin1());
                                }
                                clipboard->setMimeData(mimeData);
                        }
                        else {
                            clipboard->setText(clipBoardData);
                        }
                    }
                    break;
                }
            }

            //TODO method
            QClipboard *clipboard = QApplication::clipboard();
            if (clipboard) {
                highlight::OutputType outputType = getOutputType();
                if ( outputType==highlight::RTF) {
                    auto *mimeData = new QMimeData();

                    #ifdef Q_OS_WIN
                    mimeData->setData("Rich Text Format", clipBoardData.toLatin1());
                    #else
                    mimeData->setData("text/rtf", clipBoardData.toLatin1());
                    #endif

                    clipboard->setMimeData(mimeData);
                }
                else if ( (outputType==highlight::HTML || outputType==highlight::XHTML) && ui->cbHTMLPasteMIME->isChecked()) {
                    auto *mimeData = new QMimeData();

                    if (ui->cbEncoding->isChecked() && ui->comboEncoding->currentText().toLower()=="utf-8") {
                        mimeData->setHtml(clipBoardData.toUtf8());
                    } else {
                        mimeData->setHtml(clipBoardData.toLatin1());
                    }
                    clipboard->setMimeData(mimeData);
                }
                else {
                    clipboard->setText(clipBoardData);
                }
            }
        } else {
            statusBar()->showMessage(
                tr("Conversion of \"%1\" not possible.").arg((getDataFromCP)?tr("clipboard data"):previewFilePath));
        }

    }

    generator->clearPersistentSnippets();
    if ( applyLS ) {

        generator->lsCloseDocument(currentFile, suffix);

    }
    this->setCursor(Qt::ArrowCursor);
}

void MainWindow::plausibility()
{
    bool semanticOptionIsChecked = ui->cbLSSemantic->isChecked() || ui->cbLSHover->isChecked();
    ui->cbLSSyntaxErrors->setEnabled(semanticOptionIsChecked);
    ui->leOutputDest->setEnabled(!ui->cbWrite2Src->isChecked());
    ui->pbOutputDest->setEnabled(!ui->cbWrite2Src->isChecked());
    ui->pbBrowseOutDir->setEnabled(!ui->cbWrite2Src->isChecked());

    ui->cbPadZeroes->setEnabled(ui->cbIncLineNo->isChecked());
    ui->cbWrapping->setEnabled( !semanticOptionIsChecked);
    ui->cbAdvWrapping->setEnabled(ui->cbWrapping->isChecked() && !semanticOptionIsChecked);
    ui->sbLineLength->setEnabled(ui->cbWrapping->isChecked() && !semanticOptionIsChecked);
    ui->cbOmitWrappedLineNumbers->setEnabled(ui->cbWrapping->isChecked() && !semanticOptionIsChecked);
    ui->comboEncoding->setEnabled(ui->cbEncoding->isChecked());
    ui->cbReformat->setEnabled( !semanticOptionIsChecked);
    ui->comboReformat->setEnabled(ui->cbReformat->isChecked() && !semanticOptionIsChecked);
    ui->comboKwCase->setEnabled(ui->cbKwCase->isChecked());
    ui->comboTheme->setEnabled(getUserScriptPath("theme").isEmpty());
    ui->comboSelectSyntax->setEnabled(getUserScriptPath("lang").isEmpty());

    ui->cbHTMLInlineCSS->setEnabled(ui->cbHTMLEmbedStyle->isChecked());
    ui->cbHTMLFileNameAnchor->setEnabled(ui->cbHTMLAnchors->isChecked());
    ui->leHTMLStyleFile->setEnabled(!ui->cbHTMLEmbedStyle->isChecked());
    ui->leHTMLStyleIncFile->setEnabled(!(ui->cbHTMLInlineCSS->isEnabled() && ui->cbHTMLInlineCSS->isChecked()));
    ui->pbHTMLChooseStyleIncFile->setEnabled(!(ui->cbHTMLInlineCSS->isEnabled() && ui->cbHTMLInlineCSS->isChecked()));
    ui->leHTMLCssPrefix->setEnabled(!ui->cbHTMLInlineCSS->isChecked());
    ui->leLATEXStyleFile->setEnabled(!ui->cbLATEXEmbedStyle->isChecked());
    ui->leTEXStyleFile->setEnabled(!ui->cbTEXEmbedStyle->isChecked());
    ui->leSVGStyleFile->setEnabled(!ui->cbSVGEmbedStyle->isChecked());

    ui->cbFragment->setEnabled( getOutputType()!=highlight::RTF && getOutputType()!=highlight::SVG );
    ui->sbLineNoWidth->setEnabled(ui->cbIncLineNo->isChecked());
    ui->sbLineNoStart->setEnabled(ui->cbIncLineNo->isChecked());
    ui->cbHTMLIndex->setEnabled(!ui->cbWrite2Src->isChecked());
    ui->cbHTMLEnclosePreTags->setEnabled(ui->cbFragment->isChecked());
    ui->cbKeepInjections->setEnabled(ui->cbFragment->isEnabled() && ui->cbFragment->isChecked());
    ui->cbHTMLAnchors->setEnabled(ui->cbIncLineNo->isChecked());
    ui->cbHTMLFileNameAnchor->setEnabled(ui->cbIncLineNo->isChecked());
    ui->cbHTMLOrderedList->setEnabled(ui->cbIncLineNo->isChecked());
    ui->pbCopyToCP->setEnabled(!savedClipboardContent.isEmpty());

    copyShortcut->setEnabled(ui->tabIOSelection->currentWidget()==ui->tab_clipboard);
    pasteShortcut->setEnabled(ui->tabIOSelection->currentWidget()==ui->tab_clipboard);

    switch (ui->comboFormat->currentIndex()) {
    case 0:
    case 1:
        ui->stackedSpecificOptions->setCurrentIndex(0);
        break;
    case 2:
        ui->stackedSpecificOptions->setCurrentIndex(1);
        break;
    case 3:
        ui->stackedSpecificOptions->setCurrentIndex(2);
        break;
    case 4:
        ui->stackedSpecificOptions->setCurrentIndex(3);
        break;
    case 6:
        ui->stackedSpecificOptions->setCurrentIndex(4);
        break;
    default:
        ui->stackedSpecificOptions->setCurrentIndex(5);
        break;
    }
    ui->tabWidgetOptions->setTabText(1, tr("%1 options").arg(ui->comboFormat->currentText() ));
}

void MainWindow::updatePreview()
{
    //toggle input source flag if file is selcetd or clipboard is pasted
    if (sender()== ui->pbPasteFromCB|| sender()==ui->lvInputFiles) {
        getDataFromCP = sender()==ui->pbPasteFromCB;
    }

    if ( ui->comboTheme->currentIndex()<0
         || (!getDataFromCP && NULL==ui->lvInputFiles->currentItem())
         || (getDataFromCP && savedClipboardContent.isEmpty())) return;

    int vScroll = ui->browserPreview->verticalScrollBar()->value();
    int hScroll = ui->browserPreview->horizontalScrollBar()->value();
    this->setCursor(Qt::WaitCursor);
    highlight::HtmlGenerator pwgenerator;
    pwgenerator.setIncludeStyle(true);
    pwgenerator.setHTMLInlineCSS(true);
    if (!getDataFromCP) {
        pwgenerator.setMaxInputLineCnt(1000);
    }
    applyCtrlValues(&pwgenerator, true);

    string suffix;
    QString previewInputPath = (getDataFromCP) ? "" : ui->lvInputFiles->currentItem()->data(Qt::UserRole).toString();
    QString croppedName = QFileInfo(previewInputPath).fileName();

#ifdef Q_OS_WIN
    previewInputPath = getWindowsShortPath(previewInputPath);
#endif
     string currentFile;
    if (getDataFromCP) {
        suffix= getFileType((ui->comboSelectSyntax->itemData(ui->comboSelectSyntax->currentIndex())).toString().toStdString(),"");
    } else {
        currentFile = previewInputPath.toStdString();
        suffix = getFileType(getFileSuffix(currentFile), currentFile);
    }

    QString langPath = getUserScriptPath("lang");
    QString langInfo=tr("(user script)");
    if (langPath.isEmpty()) {
        langInfo="";
        langPath = getDistLangPath(suffix);
    }

#ifdef Q_OS_WIN
    langPath = getWindowsShortPath(langPath);
#endif

    QString themePath = getUserScriptPath("theme");
    QString themeInfo=tr("(user script)");
    QString previewData;
    bool applyLS=false;

    if (themePath.isEmpty()) {
        themeInfo="";
    }

    for (int twoPass=0; twoPass<2; twoPass++) {

        if ( pwgenerator.loadLanguage(langPath.toStdString()) != highlight::LOAD_FAILED) {

            if ( twoPass==0 ) {
                langInfo.append(QString(" [%1]").arg(QString::fromStdString(pwgenerator.getSyntaxCatDescription())));
                themeInfo.append(QString(" [%1]").arg(QString::fromStdString(pwgenerator.getThemeCatDescription())));

                ui->lbPreview->setText(tr("Preview (%1):").arg(
                                           (getDataFromCP)?tr("clipboard data"):croppedName) );

                QString syntaxDesc = tr("Current syntax: %1 %2").arg(QString::fromStdString(pwgenerator.getSyntaxDescription()), langInfo);
                QString themeDesc = tr("Current theme: %1 %2").arg(QString::fromStdString(pwgenerator.getThemeDescription()), themeInfo);
                float contrast = pwgenerator.getThemeContrast();
                QString contrastLowHint(contrast<4.5 ? "!":"");
                QString contrastDesc = tr("Contrast: %1 %2").arg(contrast, 1, 'f', 2).arg(contrastLowHint);

                statusBar()->showMessage(QString("%1 | %2 | %3").arg(syntaxDesc, themeDesc, contrastDesc));
            }

            applyLS = (ui->cbLSSemantic->isChecked() || ui->cbLSHover->isChecked()) && !currentFile.empty() && lsSyntax==suffix && initializeLS(&pwgenerator, false );
            if ( applyLS ) {

                pwgenerator.lsAddSyntaxErrorInfo( ui->cbLSSyntaxErrors->isChecked() );

                pwgenerator.lsAddHoverInfo( ui->cbLSHover->isChecked() );
                pwgenerator.lsOpenDocument(currentFile, suffix);

                if (ui->cbLSSemantic->isChecked())
                    pwgenerator.lsAddSemanticInfo(currentFile, suffix);
            }

            // fix utf-8 data preview - to be improved (other encodings??)
            if (ui->cbEncoding->isChecked() && ui->comboEncoding->currentText().toLower()=="utf-8") {
                if (getDataFromCP) {
                    previewData= QString::fromUtf8( pwgenerator.generateString(savedClipboardContent.toStdString()).c_str());
                } else {
                    previewData= QString::fromUtf8( pwgenerator.generateStringFromFile(previewInputPath.toLocal8Bit().data()).c_str());
                }
            } else {
                if (getDataFromCP) {
                    previewData= QString::fromStdString( pwgenerator.generateString(savedClipboardContent.toStdString()));
                } else {
                    previewData= QString::fromStdString( pwgenerator.generateStringFromFile(previewInputPath.toLocal8Bit().data()));
                }
            }

            if ( twoPass==0 ) {

                if (pwgenerator.requiresTwoPassParsing() ) {

                   if (pwgenerator.printPersistentState(twoPassOutFile.toStdString())) {
                        pwgenerator.resetSyntaxReaders();
                        pwgenerator.initPluginScript(twoPassOutFile.toStdString());
                    }
                    continue;
                } else {
                    ui->browserPreview->setHtml(previewData);
                    break;
                }
            }

            ui->browserPreview->setHtml(previewData);
        } else {
            statusBar()->showMessage(tr("Preview of \"%1\" not possible.").arg((getDataFromCP)?tr("clipboard data"):croppedName));
            break;
        }
    }

    pwgenerator.clearPersistentSnippets();

    if ( applyLS ) {
        pwgenerator.lsCloseDocument(currentFile, suffix);
    }

    ui->browserPreview->verticalScrollBar()->setValue(vScroll);
    ui->browserPreview->horizontalScrollBar()->setValue(hScroll);
    this->setCursor(Qt::ArrowCursor);
}

void MainWindow::showHelpFile(const QString& file){
    ShowTextFile show;
    show.setFileName(file);
    show.exec();
}

void MainWindow::on_action_Manual_triggered()
{
    showHelpFile("README.adoc");
}

void MainWindow::on_action_Changelog_triggered()
{
    showHelpFile("ChangeLog.adoc");
}

void MainWindow::on_action_Plug_Ins_triggered()
{
    showHelpFile("README_PLUGINS.adoc");
}

void MainWindow::on_action_License_triggered()
{
    showHelpFile("COPYING");
}

void MainWindow::on_pbHTMLChooseStyleIncFile_clicked()
{
    selectSingleFile(ui->leHTMLStyleIncFile, tr("Choose a style include file"), "*.css");
}
void MainWindow::on_pbSVGChooseStyleIncFile_clicked()
{
    selectSingleFile(ui->leSVGStyleIncFile, tr("Choose a style include file"), "*.css");
}
void MainWindow::on_pbLATEXChooseStyleIncFile_clicked()
{
    selectSingleFile(ui->leLATEXStyleIncFile, tr("Choose a style include file"), "*.sty");
}

void MainWindow::on_pbTEXChooseStyleIncFile_clicked()
{
    selectSingleFile(ui->leTEXStyleIncFile, tr("Choose a style include file"), "*.sty");
}

void MainWindow::on_actionAbout_translations_triggered()
{
    QMessageBox::information(this, tr("About providing translations"),
                             tr("The GUI was developed using the Qt toolkit, and "
                                "translations may be provided using the tools Qt Linguist and lrelease.\n"
                                "The highlight.ts file for Linguist resides in the src/gui-qt subdirectory.\n"
                                "The qm file generated by lrelease has to be saved in gui-files/l10n.\n\n"
                                "Please send a note to as (at) andre-simon (dot) de if you have issues during translating "
                                "or if you have finished or updated a translation."));
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData && mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        QStringList localURLs;
        for (int i = 0; i < urlList.size(); ++i) {
            localURLs << urlList.at(i).toLocalFile();
        }
        addToView(localURLs, ui->lvInputFiles);
    }
    event->acceptProposedAction();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void MainWindow::on_pbPasteFromCB_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard) {
        savedClipboardContent = clipboard->text();
        updatePreview();
        ui->pbCopyToCP->setEnabled(!savedClipboardContent.isEmpty());
    }
}

void MainWindow::on_pbCopyToCP_clicked()
{
    highlight2Clipboard(true);
}

void MainWindow::on_pbSelectPlugin_clicked()
{
    QString pluginsPath = getDistPluginPath();
    QStringList files = QFileDialog::getOpenFileNames(
                            this,
                            tr("Select one or more plug-ins"),
                            pluginsPath,
                            "*.lua");

    addToView(files, ui->lvPluginScripts, ":/plugin.png",  true, true);
}

void MainWindow::on_pbSelectScript_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(
                            this,
                            tr("Select one or more syntax or theme scripts"),
                            "",
                            "Language definitions (*.lang)\nColour themes (*.theme)");

    addToView(files, ui->lvUserScripts, ":/script.png",  true, false);
}

void MainWindow::on_pbClearSelPlugin_clicked()
{
    QList<QListWidgetItem *> selectedItems = ui->lvPluginScripts->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        delete selectedItems.at(i);
    }
    ui->lblPluginDescription->setText("");
}

void MainWindow::on_pbClearSelScript_clicked()
{
    QList<QListWidgetItem *> selectedItems = ui->lvUserScripts->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        scriptWatcher.removePath( ui->lvUserScripts->item(i)->data(Qt::UserRole).toString());
        delete selectedItems.at(i);
    }
    ui->lblScriptDescription->setText("");
}
void MainWindow::on_pbClearSelection_clicked()
{
    QList<QListWidgetItem *> selectedItems = ui->lvInputFiles->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        delete selectedItems.at(i);
    }
}

void MainWindow::on_pbClearAll_clicked()
{
    ui->lvInputFiles->clear();
}

void MainWindow::on_pbClearAllPlugins_clicked()
{
    ui->lvPluginScripts->clear();
}

void MainWindow::on_pbClearAllScripts_clicked()
{
    ui->lvUserScripts->clear();
    scriptWatcher.removePaths(scriptWatcher.files());
}

void MainWindow::on_actionVisit_website_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.andre-simon.de/"));
}

void MainWindow::on_pbPluginReadFilePath_clicked()
{
    selectSingleFile(ui->lePluginReadFilePath, tr("Choose a plug-in input file"), "*");
}

void MainWindow::on_pbCopyAndPaste_clicked()
{
    ui->pbPasteFromCB->click();
    ui->pbCopyToCP->click();
}

void MainWindow::on_pbBrowseOutDir_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(ui->leOutputDest->text()));
}

void MainWindow::on_lvUserScripts_itemClicked(QListWidgetItem *item)
{
     QString scriptPath= item->data(Qt::UserRole).toString();
     try {
         Diluculum::LuaState ls;
         ls.doFile ( scriptPath.toStdString());
         ui->lblScriptDescription->setText(QString::fromStdString(ls["Description"].value().asString()));

         if (item->checkState()==Qt::Checked){
             scriptWatcher.addPath(scriptPath);
             QString fileExt=QFileInfo(scriptPath).completeSuffix();
             QString otherScriptPath;
             for (int i = 0; i < ui->lvUserScripts->count(); ++i) {
                 otherScriptPath = ui->lvUserScripts->item(i)->data(Qt::UserRole).toString();
                 if (ui->lvUserScripts->item(i)!=item && otherScriptPath.endsWith(fileExt)){
                     ui->lvUserScripts->item(i)->setCheckState(Qt::Unchecked);
                     scriptWatcher.removePath(otherScriptPath);
                 }
             }
         } else {
             scriptWatcher.removePath(scriptPath);
         }
     } catch (Diluculum::LuaError &err) {
         QMessageBox::warning(this, "User script error", QString::fromStdString( err.what()));
     }
}

QString MainWindow::getUserScriptPath(QString type){
    for (int i = 0; i < ui->lvUserScripts->count(); ++i) {
        QString path=ui->lvUserScripts->item(i)->data(Qt::UserRole).toString();
        if (ui->lvUserScripts->item(i)->checkState()==Qt::Checked && path.endsWith(type)){
            return path;
        }
    }
    return "";
}

void MainWindow::on_lvPluginScripts_itemClicked(QListWidgetItem *item)
{
    try {
        Diluculum::LuaState ls;
        //TODO Check Windows MB Path
        ls.doFile ( item->data(Qt::UserRole).toString().toStdString());
        ui->lblPluginDescription->setText(QString::fromStdString(ls["Description"].value().asString()));
        statusBar()->showMessage(tr("Some plug-in effects may not be visible in the preview."));
    } catch (Diluculum::LuaError &err) {
        QMessageBox::warning(this, "Plug-In error", QString::fromStdString( err.what()));
    }
}


QString MainWindow::getDistThemePath(){
    QString themeLocation =  ui->comboThemeFilter->currentIndex() >1 ? "themes/base16" : "themes";
    QString selectedTheme = ui->comboTheme->currentData().toString();
#ifdef Q_OS_MACOS
     return QString("%1/../Resources/%3/%2.theme").arg(
                            QCoreApplication::applicationDirPath()).arg(selectedTheme, themeLocation);
#else
    #ifdef HL_DATA_DIR
     return  QString("%1%3/%2.theme").arg(
                            HL_DATA_DIR).arg(selectedTheme, themeLocation);
    #else
     return  QString("%1/%3/%2.theme").arg(
                            QDir::currentPath()).arg(selectedTheme, themeLocation);
    #endif
#endif
}

QString MainWindow::getDistLangPath(const string & suffix){
#ifdef Q_OS_MACOS
    return QDir::toNativeSeparators(QString("%1/../Resources/langDefs/%2.lang").arg(
                           QCoreApplication::applicationDirPath()).arg(QString::fromStdString(suffix)));
#else
    #ifdef HL_DATA_DIR
    return QDir::toNativeSeparators(QString("%1/langDefs/%2.lang").arg(
                           HL_DATA_DIR).arg(QString::fromStdString(suffix)));
    #else
    return QDir::toNativeSeparators(QString("%1/langDefs/%2.lang").arg(
                           QDir::currentPath()).arg(QString::fromStdString(suffix)));
    #endif
#endif
}

QString MainWindow::getDistPluginPath(){
#ifdef Q_OS_MACOS
    return QString("%1/../Resources/plugins").arg(QCoreApplication::applicationDirPath());
#else
    #ifdef HL_DATA_DIR
    return QDir::toNativeSeparators(QString("%1/plugins").arg(HL_DATA_DIR));
    #else
    return QDir::toNativeSeparators(QString("%1/plugins").arg(QDir::currentPath()));
    #endif
#endif
}

QString MainWindow::getDistFileConfigPath(QString name){
#ifdef Q_OS_MACOS
    return QDir::toNativeSeparators(QString("%1/../Resources/%2").arg(QCoreApplication::applicationDirPath()).arg(name));
#else
    #ifdef HL_CONFIG_DIR
    return QDir::toNativeSeparators(QString("%1/%2").arg(HL_CONFIG_DIR).arg(name));
    #else
    return QDir::toNativeSeparators(QString("%1/%2").arg(QDir::currentPath()).arg(name));
    #endif
#endif
}


QString MainWindow::getDistFileFilterPath(){
#ifdef Q_OS_MACOS
    return QCoreApplication::applicationDirPath()+"/../Resources/gui_files/ext/fileopenfilter.conf";
#else
    #ifdef HL_DATA_DIR
    return QString(HL_DATA_DIR) + "/gui_files/ext/fileopenfilter.conf";
    #else
    return QDir::currentPath()+"/gui_files/ext/fileopenfilter.conf";
    #endif
#endif
}

void MainWindow::on_comboThemeFilter_currentIndexChanged(int /*index*/ )
{
    if (ui->comboTheme->count())
        fillThemeCombo(0);
}

QString MainWindow::getWindowsShortPath(const QString & path){

    QString shortPath(path);

#ifdef Q_OS_WIN
        int length = GetShortPathName( (const wchar_t*)path.utf16(),0,0);
        if (length > 0) {
            wchar_t* buffer = new wchar_t[length];

            length = GetShortPathName( (const wchar_t*)path.utf16(), buffer, length);
            shortPath = QString::fromUtf16((const char16_t*)buffer, length);
            delete[] buffer;
        }
#endif

    return shortPath;
}

void MainWindow::loadLSProfile() {

    if (ui->comboLSProfiles->currentIndex()==0)
        return;

    lsProfile=ui->comboLSProfiles->currentText().toStdString();

    if (lsProfile.size()) {
        if (lspProfiles.count(lsProfile)) {
            highlight::LSPProfile profile = lspProfiles[lsProfile];
            lsExecutable = profile.executable;
            lsSyntax = profile.syntax;
            lsOptions = profile.options;
            lsDelay = profile.delay;

            ui->cbLSLegacy->setChecked(profile.legacy);

            ui->leLSExec->setText(QString::fromStdString(lsExecutable));
        }
    }
}


bool MainWindow::initializeLS(highlight::CodeGenerator* generator, bool tellMe)
{
    if (!generator) {
        return false;
    }

    std::string lsWorkSpace(ui->leLSWorkspace->text().toStdString());

    highlight::LSResult lsInitRes=generator->initLanguageServer ( lsExecutable, lsOptions,
                                                                  lsWorkSpace, lsSyntax,
                                                                  lsDelay,
                                                                  ui->cbLSDebug->isChecked() ? 2 : 0,
                                                                  ui->cbLSLegacy->isChecked() );
    if ( lsInitRes==highlight::INIT_OK ) {
        if (tellMe) {

            if (generator->isHoverProvider()) {
                ui->cbLSHover->setIcon(QIcon(":/ls_supported.png"));
                ui->cbLSHover->setChecked(true);
                ui->cbLSHover->setEnabled(true);
            } else {
                ui->cbLSHover->setIcon(QIcon(":/ls_not_supported.png"));;
                ui->cbLSHover->setChecked(false);
                ui->cbLSHover->setEnabled(false);
            }

            if (generator->isSemanticTokensProvider()) {
                ui->cbLSSemantic->setIcon(QIcon(":/ls_supported.png"));
                ui->cbLSSemantic->setChecked(true);
                ui->cbLSSemantic->setEnabled(true);
            } else {
                ui->cbLSSemantic->setIcon(QIcon(":/ls_not_supported.png"));;
                ui->cbLSSemantic->setChecked(false);
                ui->cbLSSemantic->setEnabled(false);
            }
            generator->exitLanguageServer();
            QMessageBox::information(this, "LSP Init. OK", tr("Language server initialization successful"));
        }
        return true;
    }
    else if ( lsInitRes==highlight::INIT_BAD_PIPE ) {
        QMessageBox::critical(this, "LSP Error", tr("Language server connection failed"));
    } else if ( lsInitRes==highlight::INIT_BAD_REQUEST ) {
        QMessageBox::critical(this,"LSP Error", tr("Language server initialization failed"));
    }

    return false;
}

void MainWindow::on_pbLSInitialize_clicked(){

    if (ui->leLSWorkspace->text().isEmpty()) {
        ui->leLSWorkspace->setFocus();
        return;
    }

    if (lsExecutable.empty()) {
        ui->leLSExec->setFocus();
        return;
    }

    highlight::HtmlGenerator lspgenerator;

    initializeLS(&lspgenerator, true);
}

void MainWindow::on_leLSExec_textChanged(){
    lsExecutable = ui->leLSExec->text().toStdString();
}

void MainWindow::on_pbSelWorkspace_clicked(){
    ui->leLSWorkspace->setText(QFileDialog::getExistingDirectory(this, tr("Select workspace directory"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks));
}

void MainWindow::on_pbSelExecutable_clicked(){
    selectSingleFile(ui->leLSExec, tr("Choose the Language Server executable"), "*");
}
