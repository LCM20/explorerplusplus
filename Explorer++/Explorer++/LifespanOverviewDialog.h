// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "LifespanModel.h"
#include <vector>

class LifespanOverviewDialog;
class ShellBrowserImpl;

class LifespanOverviewDialog : public BaseDialog
{
public:
	static LifespanOverviewDialog *Create(const ResourceLoader *resourceLoader, HWND hParent,
		LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

private:
	static const int REMAINING_COLUMN_INDEX = 2;

	LifespanOverviewDialog(const ResourceLoader *resourceLoader, HWND hParent,
		LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser);
	~LifespanOverviewDialog() = default;

	std::vector<ResizableDialogControl> GetResizableControls() override;

	void InsertLifespanEntries();
	std::optional<LifespanEntry> MaybeGetSelectedLifespanEntry() const;

	void OnItemActivated(const LifespanEntry &lifespanEntry);
	void OnEditLifespan();
	void OnCancelLifespan();

	LifespanModel *m_lifespanModel;
	ShellBrowserImpl *m_shellBrowser;

	std::vector<LifespanEntry> m_lifespanEntries;
};
