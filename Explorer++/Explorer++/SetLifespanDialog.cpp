// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SetLifespanDialog.h"
#include "LifespanHelper.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "../Helper/ShellHelper.h"
#include <Shlwapi.h>

namespace
{

// Durations, in days, for the fixed set of choices shown in the dialog.
const int DURATIONS_IN_DAYS[] = { 7, 30, 90, 182, 365 };

// Radio button IDs, in the same order as DURATIONS_IN_DAYS.
const int DURATION_RADIO_BUTTON_IDS[] = { IDC_SET_LIFESPAN_RADIO_WEEK, IDC_SET_LIFESPAN_RADIO_MONTH,
	IDC_SET_LIFESPAN_RADIO_THREE_MONTHS, IDC_SET_LIFESPAN_RADIO_HALF_YEAR,
	IDC_SET_LIFESPAN_RADIO_YEAR };

// The maximum number of days that can be entered as a custom duration. This is effectively
// arbitrary; it simply prevents nonsensical values (e.g. a value that would overflow the
// FILETIME arithmetic).
const int MAX_CUSTOM_DURATION_DAYS = 100000;

}

SetLifespanDialog *SetLifespanDialog::Create(const ResourceLoader *resourceLoader, HWND hParent,
	LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser, const std::wstring &path)
{
	return new SetLifespanDialog(resourceLoader, hParent, lifespanModel, shellBrowser, path);
}

SetLifespanDialog::SetLifespanDialog(const ResourceLoader *resourceLoader, HWND hParent,
	LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser, const std::wstring &path) :
	BaseDialog(resourceLoader, IDD_SET_LIFESPAN, hParent, DialogSizingType::None),
	m_lifespanModel(lifespanModel),
	m_shellBrowser(shellBrowser),
	m_path(path)
{
}

INT_PTR SetLifespanDialog::OnInitDialog()
{
	TCHAR fileName[MAX_PATH];
	StringCchCopy(fileName, std::size(fileName), m_path.c_str());
	PathStripPath(fileName);
	SetDlgItemText(m_hDlg, IDC_SET_LIFESPAN_FILENAME, fileName);

	// If the item already has a lifespan, the current expiry time is shown. That allows the
	// current value to be reviewed before it's overridden.
	auto expiryTime = m_lifespanModel->MaybeGetLifespan(m_path);

	if (expiryTime)
	{
		auto expiryText = LifespanHelper::FormatExpiryTime(*expiryTime);
		SetDlgItemText(m_hDlg, IDC_SET_LIFESPAN_CURRENT_VALUE, expiryText.c_str());
	}

	CheckRadioButton(m_hDlg, IDC_SET_LIFESPAN_RADIO_WEEK, IDC_SET_LIFESPAN_RADIO_CUSTOM,
		IDC_SET_LIFESPAN_RADIO_WEEK);

	return 0;
}

INT_PTR SetLifespanDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDOK:
		if (SaveLifespan())
		{
			EndDialog(m_hDlg, RETURN_OK);
		}
		break;

	case IDCANCEL:
		EndDialog(m_hDlg, RETURN_CANCEL);
		break;
	}

	return 0;
}

INT_PTR SetLifespanDialog::OnClose()
{
	EndDialog(m_hDlg, RETURN_CANCEL);
	return 0;
}

bool SetLifespanDialog::SaveLifespan()
{
	auto days = MaybeGetSelectedDurationInDays();

	if (!days)
	{
		MessageBox(m_hDlg, m_resourceLoader->LoadString(IDS_LIFESPAN_INVALID_DAYS).c_str(),
			m_resourceLoader->LoadString(IDS_LIFESPAN_TITLE).c_str(), MB_ICONERROR | MB_OK);
		return false;
	}

	m_lifespanModel->SetLifespan(m_path, LifespanHelper::GetExpiryTimeFromNow(*days));

	// The item being edited may be shown in the browser, so the lifespan column there needs to be
	// updated as well.
	m_shellBrowser->RefreshLifespanColumn();

	return true;
}

std::optional<int> SetLifespanDialog::MaybeGetSelectedDurationInDays() const
{
	for (int i = 0; i < static_cast<int>(std::size(DURATION_RADIO_BUTTON_IDS)); i++)
	{
		if (IsDlgButtonChecked(m_hDlg, DURATION_RADIO_BUTTON_IDS[i]) == BST_CHECKED)
		{
			return DURATIONS_IN_DAYS[i];
		}
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_SET_LIFESPAN_RADIO_CUSTOM) == BST_CHECKED)
	{
		BOOL translated;
		UINT days = GetDlgItemInt(m_hDlg, IDC_SET_LIFESPAN_EDIT_DAYS, &translated, FALSE);

		if (!translated || days < 1 || days > MAX_CUSTOM_DURATION_DAYS)
		{
			return std::nullopt;
		}

		return static_cast<int>(days);
	}

	return std::nullopt;
}
