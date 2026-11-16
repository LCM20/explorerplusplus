// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LifespanOverviewDialog.h"
#include "LifespanHelper.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "SetLifespanDialog.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include <Shlwapi.h>

LifespanOverviewDialog *LifespanOverviewDialog::Create(const ResourceLoader *resourceLoader,
	HWND hParent, LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser)
{
	return new LifespanOverviewDialog(resourceLoader, hParent, lifespanModel, shellBrowser);
}

LifespanOverviewDialog::LifespanOverviewDialog(const ResourceLoader *resourceLoader, HWND hParent,
	LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser) :
	BaseDialog(resourceLoader, IDD_LIFESPAN_OVERVIEW, hParent, DialogSizingType::Both),
	m_lifespanModel(lifespanModel),
	m_shellBrowser(shellBrowser)
{
}

INT_PTR LifespanOverviewDialog::OnInitDialog()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_LIFESPAN_LISTVIEW);
	ListView_SetExtendedListViewStyleEx(listView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	std::vector<std::pair<int, std::wstring>> columns = {
		{ IDS_LIFESPAN_COLUMN_NAME, m_resourceLoader->LoadString(IDS_LIFESPAN_COLUMN_NAME) },
		{ IDS_LIFESPAN_COLUMN_LOCATION,
			m_resourceLoader->LoadString(IDS_LIFESPAN_COLUMN_LOCATION) },
		{ IDS_LIFESPAN_COLUMN_REMAINING,
			m_resourceLoader->LoadString(IDS_LIFESPAN_COLUMN_REMAINING) },
		{ IDS_LIFESPAN_COLUMN_EXPIRY, m_resourceLoader->LoadString(IDS_LIFESPAN_COLUMN_EXPIRY) }
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

	InsertLifespanEntries();

	if (ListView_GetItemCount(listView) > 0)
	{
		ListViewHelper::SelectItem(listView, 0, true);
	}

	SetFocus(listView);

	return 0;
}

std::vector<ResizableDialogControl> LifespanOverviewDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_LIFESPAN_LISTVIEW), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_LIFESPAN_BUTTON_EDIT), MovingType::Both,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_LIFESPAN_BUTTON_CANCEL), MovingType::Both,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	return controls;
}

INT_PTR LifespanOverviewDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_LIFESPAN_BUTTON_EDIT:
		OnEditLifespan();
		break;

	case IDC_LIFESPAN_BUTTON_CANCEL:
		OnCancelLifespan();
		break;

	case IDOK:
	case IDCANCEL:
		EndDialog(m_hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR LifespanOverviewDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case NM_DBLCLK:
		if (pnmhdr->hwndFrom == GetDlgItem(m_hDlg, IDC_LIFESPAN_LISTVIEW))
		{
			auto lifespanEntry = MaybeGetSelectedLifespanEntry();

			if (lifespanEntry)
			{
				OnItemActivated(*lifespanEntry);
			}
		}
		break;
	}

	return 0;
}

INT_PTR LifespanOverviewDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void LifespanOverviewDialog::InsertLifespanEntries()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_LIFESPAN_LISTVIEW);

	m_lifespanEntries = m_lifespanModel->GetAllLifespans();

	std::sort(m_lifespanEntries.begin(), m_lifespanEntries.end(),
		[](const LifespanEntry &lifespanEntry1, const LifespanEntry &lifespanEntry2)
		{ return StrCmpLogicalW(lifespanEntry1.path.c_str(), lifespanEntry2.path.c_str()) < 0; });

	int iItem = 0;

	for (const auto &lifespanEntry : m_lifespanEntries)
	{
		TCHAR fileName[MAX_PATH];
		StringCchCopy(fileName, std::size(fileName), lifespanEntry.path.c_str());
		PathStripPath(fileName);

		TCHAR directory[MAX_PATH];
		StringCchCopy(directory, std::size(directory), lifespanEntry.path.c_str());
		PathRemoveFileSpec(directory);

		auto remainingText = LifespanHelper::FormatRemainingTime(lifespanEntry.expiryTime,
			m_resourceLoader);
		auto expiryText = LifespanHelper::FormatExpiryTime(lifespanEntry.expiryTime);

		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = fileName;
		lvItem.lParam = static_cast<LPARAM>(iItem);
		ListView_InsertItem(listView, &lvItem);

		ListView_SetItemText(listView, iItem, 1, directory);
		ListView_SetItemText(listView, iItem, REMAINING_COLUMN_INDEX,
			const_cast<TCHAR *>(remainingText.c_str()));
		ListView_SetItemText(listView, iItem, 3,
			const_cast<TCHAR *>(expiryText.c_str()));

		iItem++;
	}
}

std::optional<LifespanEntry> LifespanOverviewDialog::MaybeGetSelectedLifespanEntry() const
{
	HWND listView = GetDlgItem(m_hDlg, IDC_LIFESPAN_LISTVIEW);

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

	if (index < 0 || index >= static_cast<int>(m_lifespanEntries.size()))
	{
		return std::nullopt;
	}

	return m_lifespanEntries[index];
}

void LifespanOverviewDialog::OnItemActivated(const LifespanEntry &lifespanEntry)
{
	PidlAbsolute itemPidl;
	HRESULT hr = SHParseDisplayName(lifespanEntry.path.c_str(), nullptr, PidlOutParam(itemPidl), 0,
		nullptr);

	if (FAILED(hr))
	{
		return;
	}

	TCHAR parentPath[MAX_PATH];
	StringCchCopy(parentPath, std::size(parentPath), lifespanEntry.path.c_str());
	PathRemoveFileSpec(parentPath);

	// A path without a parent (e.g. a root drive) will be left unchanged by
	// PathRemoveFileSpec(). In that case, the item itself is navigated to directly.
	if (lstrcmpi(parentPath, lifespanEntry.path.c_str()) == 0)
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

void LifespanOverviewDialog::OnEditLifespan()
{
	auto lifespanEntry = MaybeGetSelectedLifespanEntry();

	if (!lifespanEntry)
	{
		return;
	}

	auto *setLifespanDialog = SetLifespanDialog::Create(m_resourceLoader, m_hDlg, m_lifespanModel,
		m_shellBrowser, lifespanEntry->path);
	INT_PTR res = setLifespanDialog->ShowModalDialog();

	if (res == RETURN_OK)
	{
		HWND listView = GetDlgItem(m_hDlg, IDC_LIFESPAN_LISTVIEW);
		ListView_DeleteAllItems(listView);
		InsertLifespanEntries();
	}
}

void LifespanOverviewDialog::OnCancelLifespan()
{
	auto lifespanEntry = MaybeGetSelectedLifespanEntry();

	if (!lifespanEntry)
	{
		return;
	}

	m_lifespanModel->RemoveLifespan(lifespanEntry->path);

	HWND listView = GetDlgItem(m_hDlg, IDC_LIFESPAN_LISTVIEW);

	int selected = ListView_GetNextItem(listView, -1, LVNI_SELECTED);

	if (selected != -1)
	{
		m_lifespanEntries.erase(m_lifespanEntries.begin() + selected);
		ListView_DeleteItem(listView, selected);
	}

	m_shellBrowser->RefreshLifespanColumn();
}
