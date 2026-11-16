// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LifespanModel.h"
#include <windowsx.h>

namespace
{

std::wstring GetKey(const std::wstring &path)
{
	std::wstring key = path;
	std::transform(key.begin(), key.end(), key.begin(), towlower);
	return key;
}

}

std::optional<FILETIME> LifespanModel::MaybeGetLifespan(const std::wstring &path) const
{
	auto itr = m_lifespans.find(GetKey(path));

	if (itr == m_lifespans.end())
	{
		return std::nullopt;
	}

	return itr->second.expiryTime;
}

void LifespanModel::SetLifespan(const std::wstring &path, FILETIME expiryTime)
{
	LifespanEntry lifespanEntry;
	lifespanEntry.path = path;
	lifespanEntry.expiryTime = expiryTime;

	m_lifespans[GetKey(path)] = lifespanEntry;
}

void LifespanModel::RemoveLifespan(const std::wstring &path)
{
	m_lifespans.erase(GetKey(path));
}

void LifespanModel::RemoveAll()
{
	m_lifespans.clear();
}

void LifespanModel::RestoreLifespan(LifespanEntry lifespanEntry)
{
	m_lifespans[GetKey(lifespanEntry.path)] = std::move(lifespanEntry);
}

std::vector<LifespanEntry> LifespanModel::GetAllLifespans() const
{
	std::vector<LifespanEntry> lifespans;
	lifespans.reserve(m_lifespans.size());

	for (const auto &entry : m_lifespans)
	{
		lifespans.push_back(entry.second);
	}

	return lifespans;
}
