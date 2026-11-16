// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddNoteDialog.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "../Helper/ShellHelper.h"
#include <Shlwapi.h>

AddNoteDialog *AddNoteDialog::Create(const ResourceLoader *resourceLoader, HWND hParent,
	NotesModel *notesModel, ShellBrowserImpl *shellBrowser, const std::wstring &path)
{
	return new AddNoteDialog(resourceLoader, hParent, notesModel, shellBrowser, path);
}

AddNoteDialog::AddNoteDialog(const ResourceLoader *resourceLoader, HWND hParent,
	NotesModel *notesModel, ShellBrowserImpl *shellBrowser, const std::wstring &path) :
	BaseDialog(resourceLoader, IDD_ADD_NOTE, hParent, DialogSizingType::None),
	m_notesModel(notesModel),
	m_shellBrowser(shellBrowser),
	m_path(path)
{
}

INT_PTR AddNoteDialog::OnInitDialog()
{
	TCHAR fileName[MAX_PATH];
	StringCchCopy(fileName, std::size(fileName), m_path.c_str());
	PathStripPath(fileName);
	SetDlgItemText(m_hDlg, IDC_ADD_NOTE_FILENAME, fileName);

	const auto note = m_notesModel->MaybeGetNote(m_path);
	SetDlgItemText(m_hDlg, IDC_ADD_NOTE_EDIT, note ? note->c_str() : L"");

	HWND editControl = GetDlgItem(m_hDlg, IDC_ADD_NOTE_EDIT);
	SendMessage(editControl, EM_SETLIMITTEXT, 0, 0);
	SetFocus(editControl);

	return 0;
}

INT_PTR AddNoteDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDOK:
		SaveNote();
		EndDialog(m_hDlg, RETURN_OK);
		break;

	case IDCANCEL:
		EndDialog(m_hDlg, RETURN_CANCEL);
		break;
	}

	return 0;
}

INT_PTR AddNoteDialog::OnClose()
{
	EndDialog(m_hDlg, RETURN_CANCEL);
	return 0;
}

void AddNoteDialog::SaveNote()
{
	HWND editControl = GetDlgItem(m_hDlg, IDC_ADD_NOTE_EDIT);

	int length = GetWindowTextLength(editControl);

	std::wstring text(length + 1, L'\0');
	GetDlgItemText(m_hDlg, IDC_ADD_NOTE_EDIT, text.data(), length + 1);
	text.resize(length);

	m_notesModel->SetNote(m_path, text);

	// The item being edited may be shown in the browser, so the note column there needs to be
	// updated as well.
	m_shellBrowser->RefreshNoteColumn();
}
