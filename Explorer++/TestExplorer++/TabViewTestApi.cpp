// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabViewTestApi.h"

TabViewTestApi::TabViewTestApi(TabView *view) : m_view(view)
{
}

void TabViewTestApi::Paint(HDC hdc, const RECT &paintRect)
{
	m_view->Paint(hdc, paintRect);
}

void TabViewTestApi::SetHotItemDetails(int index)
{
	m_view->m_hotItemIndex = index;
}

void TabViewTestApi::ClearHotItemDetails()
{
	m_view->m_hotItemIndex.reset();
}

RECT TabViewTestApi::GetTabRect(int index) const
{
	return m_view->GetTabRect(index);
}

TabView::TabContentLayout TabViewTestApi::GetTabContentLayout(int index) const
{
	return m_view->GetTabContentLayout(index);
}
