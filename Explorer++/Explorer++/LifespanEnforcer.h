// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class AppServices;

// Periodically checks the set of lifespans and moves any items whose lifespan has expired to the
// recycle bin. Items that no longer exist are simply removed from the model.
class LifespanEnforcer
{
public:
	LifespanEnforcer(AppServices *appServices);

	void CheckForExpiredItems();

private:
	HWND MaybeGetOwnerWindow() const;

	AppServices *m_appServices;
};
