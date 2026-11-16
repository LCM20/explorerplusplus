// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct LifespanEntry
{
	std::wstring path;
	FILETIME expiryTime;
};

// Holds the set of lifespans that have been attached to files and folders. A lifespan is an
// absolute expiry time; once it passes, the item will be moved to the recycle bin. Lifespans are
// keyed by item path, which means they're independent of any particular folder view.
class LifespanModel
{
public:
	std::optional<FILETIME> MaybeGetLifespan(const std::wstring &path) const;

	// Setting a lifespan overrides any existing lifespan for the item.
	void SetLifespan(const std::wstring &path, FILETIME expiryTime);
	void RemoveLifespan(const std::wstring &path);
	void RemoveAll();

	// Used when restoring lifespans from persistent storage, where the expiry time is already
	// known.
	void RestoreLifespan(LifespanEntry lifespanEntry);

	std::vector<LifespanEntry> GetAllLifespans() const;

private:
	// The key is the lowercase form of the item path, since paths are case-insensitive on Windows.
	std::unordered_map<std::wstring, LifespanEntry> m_lifespans;
};
