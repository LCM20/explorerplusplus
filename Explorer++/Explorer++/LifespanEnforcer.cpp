// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LifespanEnforcer.h"
#include "AppServices.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "LifespanHelper.h"
#include "LifespanModel.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/ShellHelper.h"

LifespanEnforcer::LifespanEnforcer(AppServices *appServices) : m_appServices(appServices)
{
}

void LifespanEnforcer::CheckForExpiredItems()
{
	auto *lifespanModel = m_appServices->GetLifespanModel();
	auto entries = lifespanModel->GetAllLifespans();

	for (const auto &entry : entries)
	{
		// Check whether the item still exists. Items that have been deleted or moved no longer
		// need to be tracked. A renamed item will also no longer be found at the original path,
		// in which case the entry is simply removed.
		DWORD attributes = GetFileAttributes(entry.path.c_str());

		if (attributes == INVALID_FILE_ATTRIBUTES)
		{
			lifespanModel->RemoveLifespan(entry.path);
			continue;
		}

		if (!LifespanHelper::IsExpired(entry.expiryTime))
		{
			continue;
		}

		PidlAbsolute pidl;
		HRESULT hr = SHParseDisplayName(entry.path.c_str(), nullptr, PidlOutParam(pidl), 0,
			nullptr);

		if (FAILED(hr))
		{
			continue;
		}

		FileActionHandler fileActionHandler;

		// The item is moved to the recycle bin, rather than being permanently deleted. If the
		// delete operation fails (e.g. because the recycle bin isn't available for a network
		// path), the entry is left in place and will be retried on the next check. The item is
		// never permanently deleted.
		HRESULT deleteResult = fileActionHandler.DeleteFiles(MaybeGetOwnerWindow(),
			{ pidl.Raw() }, false, true);

		if (SUCCEEDED(deleteResult))
		{
			lifespanModel->RemoveLifespan(entry.path);
		}
	}
}

HWND LifespanEnforcer::MaybeGetOwnerWindow() const
{
	auto *browser = m_appServices->GetBrowserList()->GetLastActive();

	if (!browser)
	{
		return nullptr;
	}

	return browser->GetHWND();
}
