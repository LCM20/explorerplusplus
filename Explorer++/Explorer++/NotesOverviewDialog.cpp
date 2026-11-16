// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NotesOverviewDialog.h"
#include "AddNoteDialog.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include <Shlwapi.h>

NotesOverviewDialog *NotesOverviewDialog::Create(const ResourceLoader *resourceLoader,
	HWND hParent, NotesModel *notesModel, ShellBrowserImpl *shellBrowser)
{
	return new NotesOverviewDialog(resourceLoader, hParent, notesModel, shellBrowser);
}

NotesOverviewDialog::NotesOverviewDialog(const ResourceLoader *resourceLoader, HWND hParent,
	NotesModel *notesModel, ShellBrowserImpl *shellBrowser) :
	BaseDialog(resourceLoader, IDD_NOTES_OVERVIEW, hParent, DialogSizingType::Both),
	m_notesModel(notesModel),
	m_shellBrowser(shellBrowser)
{
}

INT_PTR NotesOverviewDialog::OnInitDialog()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_NOTES_LISTVIEW);
	ListView_SetExtendedListViewStyleEx(listView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	std::vector<std::pair<int, std::wstring>> columns = {
		{ IDS_NOTES_COLUMN_NAME, m_resourceLoader->LoadString(IDS_NOTES_COLUMN_NAME) },
		{ IDS_NOTES_COLUMN_LOCATION, m_resourceLoader->LoadString(IDS_NOTES_COLUMN_LOCATION) },
		{ IDS_NOTES_COLUMN_NOTE, m_resourceLoader->LoadString(IDS_NOTES_COLUMN_NOTE) },
		{ IDS_NOTES_COLUMN_LAST_MODIFIED,
			m_resourceLoader->LoadString(IDS_NOTES_COLUMN_LAST_MODIFIED) }
	};

	int columnIndex = 0;

	for (auto &column : columns)
	{
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
		lvColumn.pszText = column.second.data();
		lvColumn.cx = 150;
		ListView_InsertColumn(listView, columnIndex, &lvColumn);

		columnIndex++;
	}

	InsertNoteEntries();

	ListView_SetColumnWidth(listView, NOTE_COLUMN_INDEX, LVSCW_AUTOSIZE);

	if (ListView_GetItemCount(listView) > 0)
	{
		ListViewHelper::SelectItem(listView, 0, true);
	}

	SetFocus(listView);

	return 0;
}

std::vector<ResizableDialogControl> NotesOverviewDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_NOTES_LISTVIEW), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_NOTES_BUTTON_EDIT), MovingType::Both,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_NOTES_BUTTON_DELETE), MovingType::Both,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	return controls;
}

INT_PTR NotesOverviewDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_NOTES_BUTTON_EDIT:
		OnEditNote();
		break;

	case IDC_NOTES_BUTTON_DELETE:
		OnDeleteNote();
		break;

	case IDOK:
	case IDCANCEL:
		EndDialog(m_hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR NotesOverviewDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case NM_DBLCLK:
		if (pnmhdr->hwndFrom == GetDlgItem(m_hDlg, IDC_NOTES_LISTVIEW))
		{
			auto noteEntry = MaybeGetSelectedNoteEntry();

			if (noteEntry)
			{
				OnItemActivated(*noteEntry);
			}
		}
		break;
	}

	return 0;
}

INT_PTR NotesOverviewDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void NotesOverviewDialog::InsertNoteEntries()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_NOTES_LISTVIEW);

	m_noteEntries = m_notesModel->GetAllNotes();

	std::sort(m_noteEntries.begin(), m_noteEntries.end(),
		[](const NoteEntry &noteEntry1, const NoteEntry &noteEntry2)
		{ return StrCmpLogicalW(noteEntry1.path.c_str(), noteEntry2.path.c_str()) < 0; });

	int iItem = 0;

	for (const auto &noteEntry : m_noteEntries)
	{
		TCHAR fileName[MAX_PATH];
		StringCchCopy(fileName, std::size(fileName), noteEntry.path.c_str());
		PathStripPath(fileName);

		TCHAR directory[MAX_PATH];
		StringCchCopy(directory, std::size(directory), noteEntry.path.c_str());
		PathRemoveFileSpec(directory);

		TCHAR dateModified[64];
		BOOL res = CreateFileTimeString(&noteEntry.lastModified, dateModified,
			std::size(dateModified), TRUE);

		if (!res)
		{
			dateModified[0] = L'\0';
		}

		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = fileName;
		lvItem.lParam = static_cast<LPARAM>(iItem);
		ListView_InsertItem(listView, &lvItem);

		ListView_SetItemText(listView, iItem, 1, directory);
		ListView_SetItemText(listView, iItem, NOTE_COLUMN_INDEX,
			const_cast<TCHAR *>(noteEntry.note.c_str()));
		ListView_SetItemText(listView, iItem, 3, dateModified);

		iItem++;
	}
}

std::optional<NoteEntry> NotesOverviewDialog::MaybeGetSelectedNoteEntry() const
{
	HWND listView = GetDlgItem(m_hDlg, IDC_NOTES_LISTVIEW);

	int selected = ListView_GetNextItem(listView, -1, LVNI_SELECTED);

	if (selected == -1)
	{
		return std::nullopt;
	}

	LVITEM lvItem;
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = selected;
	lvItem.iSubItem = 0;
	BOOL res = ListView_GetItem(listView, &lvItem);

	if (!res)
	{
		return std::nullopt;
	}

	int index = static_cast<int>(lvItem.lParam);

	if (index < 0 || index >= static_cast<int>(m_noteEntries.size()))
	{
		return std::nullopt;
	}

	return m_noteEntries[index];
}

void NotesOverviewDialog::OnItemActivated(const NoteEntry &noteEntry)
{
	PidlAbsolute itemPidl;
	HRESULT hr = SHParseDisplayName(noteEntry.path.c_str(), nullptr, PidlOutParam(itemPidl), 0,
		nullptr);

	if (FAILED(hr))
	{
		return;
	}

	TCHAR parentPath[MAX_PATH];
	StringCchCopy(parentPath, std::size(parentPath), noteEntry.path.c_str());
	PathRemoveFileSpec(parentPath);

	// A path without a parent (e.g. a root drive) will be left unchanged by
	// PathRemoveFileSpec(). In that case, the item itself is navigated to directly.
	if (lstrcmpi(parentPath, noteEntry.path.c_str()) == 0)
	{
		auto navigateParams = NavigateParams::Normal(itemPidl.Raw());
		m_shellBrowser->GetNavigationController()->Navigate(navigateParams);
		EndDialog(m_hDlg, 1);
		return;
	}

	PidlAbsolute parentPidl;
	hr = SHParseDisplayName(parentPath, nullptr, PidlOutParam(parentPidl), 0, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	// Navigate to the folder containing the item and select the item once the navigation
	// completes.
	auto navigateParams = NavigateParams::Normal(parentPidl.Raw());
	navigateParams.itemsToSelect = { itemPidl };
	m_shellBrowser->GetNavigationController()->Navigate(navigateParams);

	EndDialog(m_hDlg, 1);
}

void NotesOverviewDialog::OnEditNote()
{
	auto noteEntry = MaybeGetSelectedNoteEntry();

	if (!noteEntry)
	{
		return;
	}

	auto *addNoteDialog = AddNoteDialog::Create(m_resourceLoader, m_hDlg, m_notesModel,
		m_shellBrowser, noteEntry->path);
	INT_PTR res = addNoteDialog->ShowModalDialog();

	if (res == RETURN_OK)
	{
		HWND listView = GetDlgItem(m_hDlg, IDC_NOTES_LISTVIEW);
		ListView_DeleteAllItems(listView);
		InsertNoteEntries();
	}
}

void NotesOverviewDialog::OnDeleteNote()
{
	auto noteEntry = MaybeGetSelectedNoteEntry();

	if (!noteEntry)
	{
		return;
	}

	m_notesModel->RemoveNote(noteEntry->path);

	HWND listView = GetDlgItem(m_hDlg, IDC_NOTES_LISTVIEW);

	int selected = ListView_GetNextItem(listView, -1, LVNI_SELECTED);

	if (selected != -1)
	{
		m_noteEntries.erase(m_noteEntries.begin() + selected);
		ListView_DeleteItem(listView, selected);
	}

	m_shellBrowser->RefreshNoteColumn();
}
