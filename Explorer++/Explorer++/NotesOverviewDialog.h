// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "NotesModel.h"
#include <vector>

class NotesOverviewDialog;
class ShellBrowserImpl;

class NotesOverviewDialog : public BaseDialog
{
public:
	static NotesOverviewDialog *Create(const ResourceLoader *resourceLoader, HWND hParent,
		NotesModel *notesModel, ShellBrowserImpl *shellBrowser);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

private:
	static const int NOTE_COLUMN_INDEX = 2;

	NotesOverviewDialog(const ResourceLoader *resourceLoader, HWND hParent, NotesModel *notesModel,
		ShellBrowserImpl *shellBrowser);
	~NotesOverviewDialog() = default;

	std::vector<ResizableDialogControl> GetResizableControls() override;

	void InsertNoteEntries();
	std::optional<NoteEntry> MaybeGetSelectedNoteEntry() const;

	void OnItemActivated(const NoteEntry &noteEntry);
	void OnEditNote();
	void OnDeleteNote();

	NotesModel *m_notesModel;
	ShellBrowserImpl *m_shellBrowser;

	std::vector<NoteEntry> m_noteEntries;
};
