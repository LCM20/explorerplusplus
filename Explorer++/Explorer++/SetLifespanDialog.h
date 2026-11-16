// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "LifespanModel.h"
#include <string>

class ShellBrowserImpl;

// Dialog that allows a lifespan to be set for a single item. If the item already has a lifespan,
// the current expiry time is shown; confirming a new duration overrides it.
class SetLifespanDialog : public BaseDialog
{
public:
	static SetLifespanDialog *Create(const ResourceLoader *resourceLoader, HWND hParent,
		LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser, const std::wstring &path);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

private:
	SetLifespanDialog(const ResourceLoader *resourceLoader, HWND hParent,
		LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser, const std::wstring &path);
	~SetLifespanDialog() = default;

	bool SaveLifespan();
	std::optional<int> MaybeGetSelectedDurationInDays() const;

	LifespanModel *m_lifespanModel;
	ShellBrowserImpl *m_shellBrowser;
	std::wstring m_path;
};
