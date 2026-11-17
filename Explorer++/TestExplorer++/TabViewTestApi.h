// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabView.h"

class TabViewTestApi
{
public:
	TabViewTestApi(TabView *view);

	void Paint(HDC hdc, const RECT &paintRect);
	void SetHotItemDetails(int index);
	void ClearHotItemDetails();
	RECT GetTabRect(int index) const;
	TabView::TabContentLayout GetTabContentLayout(int index) const;

private:
	TabView *const m_view;
};
