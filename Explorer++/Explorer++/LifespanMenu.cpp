// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LifespanMenu.h"
#include "LifespanHelper.h"
#include "LifespanModel.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "SetLifespanDialog.h"
#include "ShellBrowser/ShellBrowserImpl.h"

namespace
{

constexpr int ONE_DAY_IN_DAYS = 1;
constexpr int ONE_WEEK_IN_DAYS = 7;
constexpr int ONE_MONTH_IN_DAYS = 30;
constexpr int HALF_YEAR_IN_DAYS = 182;

}

LifespanMenu::LifespanMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	LifespanModel *lifespanModel, ShellBrowserImpl *shellBrowser, std::vector<std::wstring> paths,
	HWND ownerWindow, const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_lifespanModel(lifespanModel),
	m_shellBrowser(shellBrowser),
	m_paths(std::move(paths)),
	m_ownerWindow(ownerWindow),
	m_resourceLoader(resourceLoader)
{
	BuildMenu(resourceLoader);
}

void LifespanMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	m_rootMenuView->AppendItem(MenuItemParams(resourceLoader->LoadString(IDS_LIFESPAN_MENU_1_DAY))
		.SetSelectedCallback([this] { OnSetLifespan(ONE_DAY_IN_DAYS); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_LIFESPAN_MENU_1_WEEK))
			.SetSelectedCallback([this] { OnSetLifespan(ONE_WEEK_IN_DAYS); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_LIFESPAN_MENU_1_MONTH))
			.SetSelectedCallback([this] { OnSetLifespan(ONE_MONTH_IN_DAYS); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_LIFESPAN_MENU_HALF_YEAR))
			.SetSelectedCallback([this] { OnSetLifespan(HALF_YEAR_IN_DAYS); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_LIFESPAN_PERMANENT))
			.SetSelectedCallback([this] { OnSetPermanent(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(MenuItemParams(resourceLoader->LoadString(IDS_LIFESPAN_MENU_CUSTOM))
		.SetSelectedCallback([this] { OnCustom(); }));

	m_rootMenuView->AppendItem(MenuItemParams(resourceLoader->LoadString(IDS_LIFESPAN_MENU_CLEAR))
		.SetSelectedCallback([this] { OnClear(); }));
}

void LifespanMenu::OnSetLifespan(int days)
{
	auto expiryTime = LifespanHelper::GetExpiryTimeFromNow(days);

	for (const auto &path : m_paths)
	{
		m_lifespanModel->SetLifespan(path, expiryTime);
	}

	m_shellBrowser->RefreshLifespanColumn();
}

void LifespanMenu::OnSetPermanent()
{
	auto expiryTime = LifespanHelper::GetPermanentExpiryTime();

	for (const auto &path : m_paths)
	{
		m_lifespanModel->SetLifespan(path, expiryTime);
	}

	m_shellBrowser->RefreshLifespanColumn();
}

void LifespanMenu::OnCustom()
{
	if (m_paths.empty())
	{
		return;
	}

	// The custom duration dialog only handles a single item, so the first path is used when
	// multiple items are selected.
	auto *setLifespanDialog = SetLifespanDialog::Create(m_resourceLoader, m_ownerWindow,
		m_lifespanModel, m_shellBrowser, m_paths.front());
	setLifespanDialog->ShowModalDialog();
}

void LifespanMenu::OnClear()
{
	for (const auto &path : m_paths)
	{
		m_lifespanModel->RemoveLifespan(path);
	}

	m_shellBrowser->RefreshLifespanColumn();
}
