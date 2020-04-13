/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "ses_base.h"

#include "../schematiceditor.h"

#include <librepcb/project/project.h>
#include <librepcb/projecteditor/projecteditor.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SES_Base::SES_Base(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                   GraphicsView& editorGraphicsView, UndoStack& undoStack)
  : QObject(0),
    mWorkspace(editor.getProjectEditor().getWorkspace()),
    mProject(editor.getProject()),
    mCircuit(editor.getProject().getCircuit()),
    mEditor(editor),
    mEditorUi(editorUi),
    mEditorGraphicsView(editorGraphicsView),
    mUndoStack(undoStack) {
}

SES_Base::~SES_Base() {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

const LengthUnit& SES_Base::getDefaultLengthUnit() const noexcept {
  return mWorkspace.getSettings().defaultLengthUnit.get();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
