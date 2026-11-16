// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "NotesModel.h"
#include <string>

class ShellBrowserImpl;

// Dialog that allows a note to be added to, or edited for, a single item. The note is retrieved
// from the notes model when the dialog is initialized, then saved back to the model when the user
// confirms the change.
class AddNoteDialog : public BaseDialog
{
public:
	static AddNoteDialog *Create(const ResourceLoader *resourceLoader, HWND hParent,
		NotesModel *notesModel, ShellBrowserImpl *shellBrowser, const std::wstring &path);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

private:
	AddNoteDialog(const ResourceLoader *resourceLoader, HWND hParent, NotesModel *notesModel,
		ShellBrowserImpl *shellBrowser, const std::wstring &path);
	~AddNoteDialog() = default;

	void SaveNote();

	NotesModel *m_notesModel;
	ShellBrowserImpl *m_shellBrowser;
	std::wstring m_path;
};
