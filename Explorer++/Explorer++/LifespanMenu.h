// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <string>
#include <vector>

class LifespanModel;
class ResourceLoader;
class ShellBrowserImpl;

// Context menu shown when the lifespan column is clicked for one or more items. It allows a fixed
// duration, a permanent lifespan or a custom duration to be set, or an existing lifespan to be
// cleared.
class LifespanMenu : public MenuBase
{
public:
	LifespanMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser,
		std::vector<std::wstring> paths, HWND ownerWindow, const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const ResourceLoader *resourceLoader);

	void OnSetLifespan(int days);
	void OnSetPermanent();
	void OnCustom();
	void OnClear();

	LifespanModel *const m_lifespanModel;
	ShellBrowserImpl *const m_shellBrowser;
	std::vector<std::wstring> m_paths;
	const HWND m_ownerWindow;
	const ResourceLoader *const m_resourceLoader;
};
