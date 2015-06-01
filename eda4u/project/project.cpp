/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QPrinter>
#include <eda4ucommon/exceptions.h>
#include <eda4ucommon/fileio/filelock.h>
#include <eda4ucommon/fileio/smarttextfile.h>
#include <eda4ucommon/fileio/smartxmlfile.h>
#include <eda4ucommon/fileio/smartinifile.h>
#include <eda4ucommon/fileio/xmldomdocument.h>
#include <eda4ucommon/fileio/xmldomelement.h>
#include <eda4ucommon/systeminfo.h>
#include <eda4ucommon/undostack.h>
#include <eda4ucommon/schematiclayer.h>
#include "project.h"
#include "../workspace/workspace.h"
#include "../workspace/settings/workspacesettings.h"
#include "library/projectlibrary.h"
#include "circuit/circuit.h"
#include "schematics/schematiceditor.h"
#include "schematics/schematic.h"
#include "erc/ercmsglist.h"
#include "settings/projectsettings.h"
#include "boards/board.h"
#include "boards/boardeditor.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Project::Project(const FilePath& filepath, bool create) throw (Exception) :
    QObject(nullptr), IF_AttributeProvider(), mPath(filepath.getParentDir()),
    mFilepath(filepath), mXmlFile(nullptr), mFileLock(filepath), mIsRestored(false),
    mIsReadOnly(false), mDescriptionHtmlFile(nullptr), mProjectIsModified(false),
    mUndoStack(nullptr), mProjectSettings(nullptr), mProjectLibrary(nullptr),
    mErcMsgList(nullptr), mCircuit(nullptr), mSchematicEditor(nullptr),
    mBoardEditor(nullptr)
{
    qDebug() << (create ? "create project..." : "open project...");

    // Check if the filepath is valid
    if (mFilepath.getSuffix() != "e4u")
    {
        throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(),
            tr("The suffix of the project file must be \"e4u\"!"));
    }
    if (create)
    {
        if (mFilepath.isExistingDir() || mFilepath.isExistingFile())
        {
            throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(), QString(tr(
                "The file \"%1\" does already exist!")).arg(mFilepath.toNative()));
        }
        if (!mPath.mkPath())
        {
            throw RuntimeError(__FILE__, __LINE__, mPath.toStr(), QString(tr(
                "Could not create the directory \"%1\"!")).arg(mPath.toNative()));
        }
    }
    else
    {
        if (((!mFilepath.isExistingFile())) || (!mPath.isExistingDir()))
        {
            throw RuntimeError(__FILE__, __LINE__, mFilepath.toStr(),
                QString(tr("Invalid project file: \"%1\"")).arg(mFilepath.toNative()));
        }
    }

    // Check if the project is locked (already open or application was crashed). In case
    // of a crash, the user can decide if the last backup should be restored. If the
    // project should be opened, the lock file will be created/updated here.
    switch (mFileLock.getStatus()) // throws an exception on error
    {
        case FileLock::LockStatus_t::Unlocked:
        {
            // nothing to do here (the project will be locked later)
            break;
        }

        case FileLock::LockStatus_t::Locked:
        {
            // the project is locked by another application instance! open read only?
            QMessageBox::StandardButton btn = QMessageBox::question(0, tr("Open Read-Only?"),
                tr("The project is already opened by another application instance or user. "
                "Do you want to open the project in read-only mode?"), QMessageBox::Yes |
                QMessageBox::Cancel, QMessageBox::Cancel);
            switch (btn)
            {
                case QMessageBox::Yes: // open the project in read-only mode
                    mIsReadOnly = true;
                    break;
                default: // abort opening the project
                    throw UserCanceled(__FILE__, __LINE__);
            }
            break;
        }

        case FileLock::LockStatus_t::StaleLock:
        {
            // the application crashed while this project was open! ask the user what to do
            QMessageBox::StandardButton btn = QMessageBox::question(0, tr("Restore Project?"),
                tr("It seems that the application was crashed while this project was open. "
                "Do you want to restore the last automatic backup?"), QMessageBox::Yes |
                QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
            switch (btn)
            {
                case QMessageBox::Yes: // open the project and restore the last backup
                    mIsRestored = true;
                    break;
                case QMessageBox::No: // open the project without restoring the last backup
                    mIsRestored = false;
                    break;
                default: // abort opening the project
                    throw UserCanceled(__FILE__, __LINE__);
            }
            break;
        }

        default: Q_ASSERT(false); throw LogicError(__FILE__, __LINE__);
    }

    // the project can be opened by this application, so we will lock the whole project
    if (!mIsReadOnly) mFileLock.lock(); // throws an exception on error

    // check if the combination of "create", "mIsRestored" and "mIsReadOnly" is valid
    Q_ASSERT(!(create && (mIsRestored || mIsReadOnly)));


    // OK - the project is locked (or read-only) and can be opened!
    // Until this line, there was no memory allocated on the heap. But in the rest of the
    // constructor, a lot of object will be created on the heap. If an exception is
    // thrown somewhere, we must ensure that all the allocated memory gets freed.
    // This is done by a try/catch block. In the catch-block, all allocated memory will
    // be freed. Then the exception is rethrown to leave the constructor.

    try
    {
        // try to create/open the XML project file
        QSharedPointer<XmlDomDocument> doc;
        XmlDomElement* root = nullptr;
        if (create)
        {
            mXmlFile = SmartXmlFile::create(mFilepath);
        }
        else
        {
            mXmlFile = new SmartXmlFile(mFilepath, mIsRestored, mIsReadOnly);
            doc = mXmlFile->parseFileAndBuildDomTree();
            root = &doc->getRoot();
        }


        // load project attributes
        if (create)
        {
            mName = mFilepath.getCompleteBasename();
            mAuthor = SystemInfo::getFullUsername();
            mCreated = QDateTime::currentDateTime();
            mLastModified = QDateTime::currentDateTime();
        }
        else
        {
            mName = root->getFirstChild("meta/name", true, true)->getText();
            mAuthor = root->getFirstChild("meta/author", true, true)->getText();
            mCreated = root->getFirstChild("meta/created", true, true)->getText<QDateTime>();
            mLastModified = root->getFirstChild("meta/last_modified", true, true)->getText<QDateTime>();
        }

        // Load description HTML file
        if (create)
            mDescriptionHtmlFile = SmartTextFile::create(mPath.getPathTo("description/index.html"));
        else
            mDescriptionHtmlFile = new SmartTextFile(mPath.getPathTo("description/index.html"),
                                                     mIsRestored, mIsReadOnly);

        // Create all needed objects
        mProjectSettings = new ProjectSettings(*this, mIsRestored, mIsReadOnly, create);
        mProjectLibrary = new ProjectLibrary(*this, mIsRestored, mIsReadOnly);
        mUndoStack = new UndoStack();
        mErcMsgList = new ErcMsgList(*this, mIsRestored, mIsReadOnly, create);
        mCircuit = new Circuit(*this, mIsRestored, mIsReadOnly, create);

        // Load all schematic layers
        foreach (uint id, SchematicLayer::getAllLayerIDs())
            mSchematicLayers.insert(id, new SchematicLayer(id));

        // Load all schematics
        if (create)
        {
            FilePath fp = FilePath::fromRelative(mPath.getPathTo("schematics"), "main.xml");
            Schematic* schematic = Schematic::create(*this, fp, "Main Page");
            addSchematic(schematic);
        }
        else
        {
            for (XmlDomElement* node = root->getFirstChild("schematics/schematic", true, false);
                 node; node = node->getNextSibling("schematic"))
            {
                FilePath fp = FilePath::fromRelative(mPath.getPathTo("schematics"), node->getText(true));
                Schematic* schematic = new Schematic(*this, fp, mIsRestored, mIsReadOnly);
                addSchematic(schematic);
            }
            qDebug() << mSchematics.count() << "schematics successfully loaded!";
        }

        // Load all boards
        if (create)
        {
            FilePath fp = FilePath::fromRelative(mPath.getPathTo("boards"), "default.xml");
            Board* board = Board::create(*this, fp, "Default");
            addBoard(board);
        }
        else
        {
            for (XmlDomElement* node = root->getFirstChild("boards/board", true, false);
                 node; node = node->getNextSibling("board"))
            {
                FilePath fp = FilePath::fromRelative(mPath.getPathTo("boards"), node->getText(true));
                Board* board = new Board(*this, fp, mIsRestored, mIsReadOnly);
                addBoard(board);
            }
            qDebug() << mBoards.count() << "boards successfully loaded!";
        }

        // at this point, the whole circuit with all schematics and boards is successfully
        // loaded, so the ERC list now contains all the correct ERC messages.
        // So we can now restore the ignore state of each ERC message from the XML file.
        mErcMsgList->restoreIgnoreState();

        // create the whole schematic/board editor GUI inclusive FSM and so on
        mSchematicEditor = new SchematicEditor(*this, mIsReadOnly);
        mBoardEditor = new BoardEditor(*this, mIsReadOnly);

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

        if (create) saveProject(); // write all files to harddisc
    }
    catch (...)
    {
        // free the allocated memory in the reverse order of their allocation...
        delete mBoardEditor;            mBoardEditor = nullptr;
        delete mSchematicEditor;        mSchematicEditor = nullptr;
        foreach (Board* board, mBoards)
            try { removeBoard(board, true); } catch (...) {}
        foreach (Schematic* schematic, mSchematics)
            try { removeSchematic(schematic, true); } catch (...) {}
        qDeleteAll(mSchematicLayers);   mSchematicLayers.clear();
        delete mCircuit;                mCircuit = nullptr;
        delete mErcMsgList;             mErcMsgList = nullptr;
        delete mUndoStack;              mUndoStack = nullptr;
        delete mProjectLibrary;         mProjectLibrary = nullptr;
        delete mProjectSettings;        mProjectSettings = nullptr;
        delete mDescriptionHtmlFile;    mDescriptionHtmlFile = nullptr;
        delete mXmlFile;                mXmlFile = nullptr;
        throw; // ...and rethrow the exception
    }

    // project successfully opened! :-)

    // setup the timer for automatic backups, if enabled in the settings
    int intervalSecs =  Workspace::instance().getSettings().getProjectAutosaveInterval()->getInterval();
    if ((intervalSecs > 0) && (!mIsReadOnly))
    {
        // autosaving is enabled --> start the timer
        connect(&mAutoSaveTimer, &QTimer::timeout, this, &Project::autosaveProject);
        mAutoSaveTimer.start(1000 * intervalSecs);
    }

    qDebug() << "project successfully loaded!";
}

Project::~Project() noexcept
{
    // inform the workspace that this project will get destroyed
    Workspace::instance().unregisterOpenProject(this);

    // stop the autosave timer
    mAutoSaveTimer.stop();

    // abort all active commands!
    mSchematicEditor->abortAllCommands();
    mBoardEditor->abortAllCommands();
    Q_ASSERT(mUndoStack->isCommandActive() == false);

    // delete all command objects in the undo stack (must be done before other important
    // objects are deleted, as undo command objects can hold pointers/references to them!)
    mUndoStack->clear();

    // free the allocated memory in the reverse order of their allocation

    delete mBoardEditor;            mBoardEditor = nullptr;
    delete mSchematicEditor;        mSchematicEditor = nullptr;

    // delete all boards and schematics (and catch all throwed exceptions)
    foreach (Board* board, mBoards)
        try { removeBoard(board, true); } catch (...) {}
    qDeleteAll(mRemovedBoards); mRemovedBoards.clear();
    foreach (Schematic* schematic, mSchematics)
        try { removeSchematic(schematic, true); } catch (...) {}
    qDeleteAll(mRemovedSchematics); mRemovedSchematics.clear();

    qDeleteAll(mSchematicLayers);   mSchematicLayers.clear();
    delete mCircuit;                mCircuit = nullptr;
    delete mErcMsgList;             mErcMsgList = nullptr;
    delete mUndoStack;              mUndoStack = nullptr;
    delete mProjectLibrary;         mProjectLibrary = nullptr;
    delete mProjectSettings;        mProjectSettings = nullptr;
    delete mDescriptionHtmlFile;    mDescriptionHtmlFile = nullptr;
    delete mXmlFile;                mXmlFile = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString Project::getDescription() const noexcept
{
    return mDescriptionHtmlFile->getContent();
}

/*****************************************************************************************
 *  Setters: Attributes
 ****************************************************************************************/

void Project::setName(const QString& newName) noexcept
{
    mName = newName;
    emit attributesChanged();
}

void Project::setDescription(const QString& newDescription) noexcept
{
    mDescriptionHtmlFile->setContent(newDescription.toUtf8());
    emit attributesChanged();
}

void Project::setAuthor(const QString& newAuthor) noexcept
{
    mAuthor = newAuthor;
    emit attributesChanged();
}

void Project::setCreated(const QDateTime& newCreated) noexcept
{
    mCreated = newCreated;
    emit attributesChanged();
}

void Project::setLastModified(const QDateTime& newLastModified) noexcept
{
    mLastModified = newLastModified;
    emit attributesChanged();
}

/*****************************************************************************************
 *  Schematic Methods
 ****************************************************************************************/

int Project::getSchematicIndex(const Schematic* schematic) const noexcept
{
    return mSchematics.indexOf(const_cast<Schematic*>(schematic));
}

Schematic* Project::getSchematicByUuid(const QUuid& uuid) const noexcept
{
    foreach (Schematic* schematic, mSchematics)
    {
        if (schematic->getUuid() == uuid)
            return schematic;
    }
    return nullptr;
}

Schematic* Project::getSchematicByName(const QString& name) const noexcept
{
    foreach (Schematic* schematic, mSchematics)
    {
        if (schematic->getName() == name)
            return schematic;
    }
    return nullptr;
}

Schematic* Project::createSchematic(const QString& name) throw (Exception)
{
    QString basename = name; /// @todo remove special characters to create a valid filename!
    FilePath filepath = mPath.getPathTo("schematics/" % basename % ".xml");
    return Schematic::create(*this, filepath, name);
}

void Project::addSchematic(Schematic* schematic, int newIndex) throw (Exception)
{
    Q_ASSERT(schematic);

    if ((newIndex < 0) || (newIndex > mSchematics.count()))
        newIndex = mSchematics.count();

    if (getSchematicByUuid(schematic->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, schematic->getUuid().toString(),
            QString(tr("There is already a schematic with the UUID \"%1\"!"))
            .arg(schematic->getUuid().toString()));
    }

    if (getSchematicByName(schematic->getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, schematic->getName(),
            QString(tr("There is already a schematic with the name \"%1\"!"))
            .arg(schematic->getName()));
    }

    schematic->addToProject(); // can throw an exception
    mSchematics.insert(newIndex, schematic);

    if (mRemovedSchematics.contains(schematic))
        mRemovedSchematics.removeOne(schematic);

    emit schematicAdded(newIndex);
    emit attributesChanged();
}

void Project::removeSchematic(Schematic* schematic, bool deleteSchematic) throw (Exception)
{
    Q_ASSERT(schematic);
    int index = mSchematics.indexOf(schematic);
    Q_ASSERT(index >= 0);
    Q_ASSERT(!mRemovedSchematics.contains(schematic));

    if ((!deleteSchematic) && (!schematic->isEmpty()))
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("There are still elements in the schematic \"%1\"!"))
            .arg(schematic->getName()));
    }

    schematic->removeFromProject(); // can throw an exception
    mSchematics.removeAt(index);

    emit schematicRemoved(index);
    emit attributesChanged();

    if (deleteSchematic)
        delete schematic;
    else
        mRemovedSchematics.append(schematic);
}

void Project::exportSchematicsAsPdf(const FilePath& filepath) throw (Exception)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPaperSize(QPrinter::A4);
    printer.setOrientation(QPrinter::Landscape);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setCreator(QString("EDA4U %1.%2").arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR));
    printer.setOutputFileName(filepath.toStr());

    QList<uint> pages;
    for (int i = 0; i < mSchematics.count(); i++)
        pages.append(i);

    printSchematicPages(printer, pages);

    QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.toStr()));
}

/*****************************************************************************************
 *  Board Methods
 ****************************************************************************************/

int Project::getBoardIndex(const Board* board) const noexcept
{
    return mBoards.indexOf(const_cast<Board*>(board));
}

Board* Project::getBoardByUuid(const QUuid& uuid) const noexcept
{
    foreach (Board* board, mBoards)
    {
        if (board->getUuid() == uuid)
            return board;
    }
    return nullptr;
}

Board* Project::getBoardByName(const QString& name) const noexcept
{
    foreach (Board* board, mBoards)
    {
        if (board->getName() == name)
            return board;
    }
    return nullptr;
}

Board* Project::createBoard(const QString& name) throw (Exception)
{
    QString basename = name; /// @todo remove special characters to create a valid filename!
    FilePath filepath = mPath.getPathTo("boards/" % basename % ".xml");
    return Board::create(*this, filepath, name);
}

void Project::addBoard(Board* board, int newIndex) throw (Exception)
{
    Q_ASSERT(board);

    if ((newIndex < 0) || (newIndex > mBoards.count()))
        newIndex = mBoards.count();

    if (getBoardByUuid(board->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, board->getUuid().toString(),
            QString(tr("There is already a board with the UUID \"%1\"!"))
            .arg(board->getUuid().toString()));
    }

    if (getBoardByName(board->getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, board->getName(),
            QString(tr("There is already a board with the name \"%1\"!"))
            .arg(board->getName()));
    }

    board->addToProject(); // can throw an exception
    mBoards.insert(newIndex, board);

    if (mRemovedBoards.contains(board))
        mRemovedBoards.removeOne(board);

    emit boardAdded(newIndex);
    emit attributesChanged();
}

void Project::removeBoard(Board* board, bool deleteBoard) throw (Exception)
{
    Q_ASSERT(board);
    int index = mBoards.indexOf(board);
    Q_ASSERT(index >= 0);
    Q_ASSERT(!mRemovedBoards.contains(board));

    if ((!deleteBoard) && (!board->isEmpty()))
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("There are still elements in the board \"%1\"!"))
            .arg(board->getName()));
    }

    board->removeFromProject(); // can throw an exception
    mBoards.removeAt(index);

    emit boardRemoved(index);
    emit attributesChanged();

    if (deleteBoard)
        delete board;
    else
        mRemovedBoards.append(board);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool Project::windowIsAboutToClose(QMainWindow* window) noexcept
{
    int countOfOpenWindows = 0;
    if (mSchematicEditor->isVisible())  {countOfOpenWindows++;}
    if (mBoardEditor->isVisible())      {countOfOpenWindows++;}

    if (countOfOpenWindows <= 1)
    {
        // the last open window (schematic editor, board editor, ...) is about to close.
        // --> close the whole project
        return close(window);
    }

    return true; // this is not the last open window, so no problem to close it...
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool Project::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                   bool passToParents, QString& value) const noexcept
{
    Q_UNUSED(passToParents); // TODO: pass to workspace?!

    if ((attrNS == QLatin1String("PRJ")) || (attrNS.isEmpty()))
    {
        if (attrKey == QLatin1String("NAME"))
            return value = mName, true;
        else if (attrKey == QLatin1String("AUTHOR"))
            return value = mAuthor, true;
        else if (attrKey == QLatin1String("CREATED"))
            return value = mCreated.toString(Qt::SystemLocaleShortDate), true;
        else if (attrKey == QLatin1String("LAST_MODIFIED"))
            return value = mLastModified.toString(Qt::SystemLocaleShortDate), true;
    }

    return false;
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void Project::showSchematicEditor() noexcept
{
    mSchematicEditor->show();
    mSchematicEditor->raise();
    mSchematicEditor->activateWindow();
}

void Project::showBoardEditor() noexcept
{
    mBoardEditor->show();
    mBoardEditor->raise();
    mBoardEditor->activateWindow();
}

bool Project::saveProject() noexcept
{
    QStringList errors;

    // step 1: save whole project to temporary files
    qDebug() << "Begin saving the project to temporary files...";
    if (!save(false, errors))
    {
        QMessageBox::critical(0, tr("Error while saving the project"),
            QString(tr("The project could not be saved!\n\nError Message:\n%1",
            "variable count of error messages", errors.count())).arg(errors.join("\n")));
        qCritical() << "Project saving (1) finished with" << errors.count() << "errors!";
        return false;
    }

    if (errors.count() > 0) // This should not happen! There must be an error in the code!
    {
        QMessageBox::critical(0, tr("Error while saving the project"),
            QString(tr("The project could not be saved!\n\nError Message:\n%1",
            "variable count of error messages", errors.count())).arg(errors.join("\n")));
        qCritical() << "save() has returned true, but there are" << errors.count() << "errors!";
        return false;
    }

    // step 2: save whole project to original files
    qDebug() << "Begin saving the project to original files...";
    if (!save(true, errors))
    {
        QMessageBox::critical(0, tr("Error while saving the project"),
            QString(tr("The project could not be saved!\n\nError Message:\n%1",
            "variable count of error messages", errors.count())).arg(errors.join("\n")));
        qCritical() << "Project saving (2) finished with" << errors.count() << "errors!";
        return false;
    }

    // saving to the original files was successful --> clean the undo stack and clear the "modified" flag
    mUndoStack->setClean();
    mProjectIsModified = false;
    qDebug() << "Project successfully saved";
    return true;
}

bool Project::autosaveProject() noexcept
{
    if ((!mIsRestored) && (mUndoStack->isClean()) && (!mProjectIsModified))
        return false; // do not save if there are no changes

    if (mUndoStack->isCommandActive())
    {
        // the user is executing a command at the moment, so we should not save now,
        // try it a few seconds later instead...
        QTimer::singleShot(10000, this, SLOT(autosave()));
        return false;
    }

    QStringList errors;

    qDebug() << "Autosave the project...";

    if (!save(false, errors))
    {
        qCritical() << "Project autosave finished with" << errors.count() << "errors!";
        return false;
    }

    qDebug() << "Project autosave was successful";
    return true;
}

bool Project::close(QWidget* msgBoxParent) noexcept
{
    if (((!mIsRestored) && (mUndoStack->isClean()) && (!mProjectIsModified)) || (mIsReadOnly))
    {
        // no unsaved changes or opened in read-only mode --> the project can be closed
        deleteLater();  // this project object will be deleted later in the event loop
        return true;
    }

    QString msg1 = tr("You have unsaved changes in the project.\n"
                      "Do you want to save them bevore closing the project?");
    QString msg2 = tr("Attention: The project was restored from a backup, so if you "
                      "don't save the project now the current state of the project (and "
                      "the backup) will be lost forever!");

    QMessageBox::StandardButton choice = QMessageBox::question(msgBoxParent,
         tr("Save Project?"), (mIsRestored ? msg1 % QStringLiteral("\n\n") % msg2 : msg1),
         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

    switch (choice)
    {
        case QMessageBox::Yes: // save and close project
            if (saveProject())
            {
                deleteLater(); // this project object will be deleted later in the event loop
                return true;
            }
            else
                return false;

        case QMessageBox::No: // close project without saving
            deleteLater(); // this project object will be deleted later in the event loop
            return true;

        default: // cancel, don't close the project
            return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Project::checkAttributesValidity() const noexcept
{
    if (mName.isEmpty())    return false;
    return true;
}

XmlDomElement* Project::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("project"));

    // meta
    XmlDomElement* meta = root->appendChild("meta");
    meta->appendTextChild("name", mName);
    meta->appendTextChild("author", mAuthor);
    meta->appendTextChild("created", mCreated);
    meta->appendTextChild("last_modified", mLastModified);

    // schematics
    FilePath schematicsPath(mPath.getPathTo("schematics"));
    XmlDomElement* schematics = root->appendChild("schematics");
    foreach (Schematic* schematic, mSchematics)
        schematics->appendTextChild("schematic", schematic->getFilePath().toRelative(schematicsPath));

    // boards
    FilePath boardsPath(mPath.getPathTo("boards"));
    XmlDomElement* boards = root->appendChild("boards");
    foreach (Board* board, mBoards)
        boards->appendTextChild("board", board->getFilePath().toRelative(boardsPath));

    return root.take();
}

bool Project::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    if (mIsReadOnly)
    {
        errors.append(tr("The project was opened in read-only mode."));
        return false;
    }

    if (mUndoStack->isCommandActive())
    {
        errors.append(tr("A command is active at the moment."));
        return false;
    }

    // Save *.e4u project file
    try
    {
        setLastModified(QDateTime::currentDateTime());
        XmlDomElement* root = serializeToXmlDomElement();
        XmlDomDocument doc(*root, true);
        mXmlFile->save(doc, toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    // Save "description/index.html"
    try
    {
        mDescriptionHtmlFile->save(toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    // Save circuit
    if (!mCircuit->save(toOriginal, errors))
        success = false;

    // Save all added schematics (*.xml files)
    foreach (Schematic* schematic, mSchematics)
    {
        if (!schematic->save(toOriginal, errors))
            success = false;
    }
    // Save all removed schematics (*.xml files)
    foreach (Schematic* schematic, mRemovedSchematics)
    {
        if (!schematic->save(toOriginal, errors))
            success = false;
    }

    // Save all added boards (*.xml files)
    foreach (Board* board, mBoards)
    {
        if (!board->save(toOriginal, errors))
            success = false;
    }
    // Save all removed boards (*.xml files)
    foreach (Board* board, mRemovedBoards)
    {
        if (!board->save(toOriginal, errors))
            success = false;
    }

    // Save settings
    if (!mProjectSettings->save(toOriginal, errors))
        success = false;

    // Save ERC messages list
    if (!mErcMsgList->save(toOriginal, errors))
        success = false;

    // if the project was restored from a backup, reset the mIsRestored flag as the current
    // state of the project is no longer a restored backup but a properly saved project
    if (mIsRestored && success && toOriginal)
        mIsRestored = false;

    return success;
}

void Project::printSchematicPages(QPrinter& printer, QList<uint>& pages) throw (Exception)
{
    if (pages.isEmpty())
        throw RuntimeError(__FILE__, __LINE__, QString(), tr("No schematic pages selected."));

    QPainter painter(&printer);

    for (int i = 0; i < pages.count(); i++)
    {
        Schematic* schematic = getSchematicByIndex(pages[i]);
        if (!schematic)
        {
            throw RuntimeError(__FILE__, __LINE__, QString(),
                QString(tr("No schematic page with the index %1 found.")).arg(pages[i]));
        }
        schematic->clearSelection();
        schematic->renderToQPainter(painter);

        if (i != pages.count() - 1)
        {
            if (!printer.newPage())
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    tr("Unknown error while printing."));
            }
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project