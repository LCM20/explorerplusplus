// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <ShlObj.h>

// Allows the AddressBarView controller to be notified of events that occur within the view.
class AddressBarViewDelegate
{
public:
	virtual ~AddressBarViewDelegate() = default;

	virtual bool OnKeyPressed(UINT key) = 0;
	virtual void OnBeginDrag(PCIDLIST_ABSOLUTE pidl) = 0;
	virtual void OnFocused() = 0;

	// Called when a breadcrumb segment (or an item in one of the dropdown menus) is selected.
	virtual void OnSegmentSelected(PCIDLIST_ABSOLUTE pidl) = 0;
};
