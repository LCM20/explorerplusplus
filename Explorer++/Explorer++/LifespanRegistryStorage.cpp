// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LifespanModel.h"
#include "LifespanRegistryStorage.h"
#include "../Helper/RegistrySettings.h"

namespace LifespanRegistryStorage
{

namespace
{

const wchar_t LIFESPANS_KEY_PATH[] = L"Lifespans";

const wchar_t SETTING_PATH[] = L"Path";
const wchar_t SETTING_EXPIRY_TIME[] = L"ExpiryTime";

std::unique_ptr<LifespanEntry> LoadLifespan(HKEY key)
{
	std::wstring path;
	LSTATUS result = RegistrySettings::ReadString(key, SETTING_PATH, path);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	auto lifespanEntry = std::make_unique<LifespanEntry>();
	lifespanEntry->path = path;

	// The expiry time is essential; an entry without it is ignored, since there's no sensible
	// fallback value.
	if (!RegistrySettings::ReadDateTime(key, SETTING_EXPIRY_TIME, lifespanEntry->expiryTime))
	{
		return nullptr;
	}

	return lifespanEntry;
}

void LoadFromKey(HKEY parentKey, LifespanModel *model)
{
	wil::unique_hkey childKey;
	size_t index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey)
		== ERROR_SUCCESS)
	{
		auto lifespanEntry = LoadLifespan(childKey.get());

		if (lifespanEntry)
		{
			model->RestoreLifespan(std::move(*lifespanEntry));
		}

		index++;
	}
}

void SaveLifespan(HKEY key, const LifespanEntry &lifespanEntry)
{
	RegistrySettings::SaveString(key, SETTING_PATH, lifespanEntry.path);
	RegistrySettings::SaveDateTime(key, SETTING_EXPIRY_TIME, lifespanEntry.expiryTime);
}

void SaveToKey(HKEY parentKey, const LifespanModel *model)
{
	// Any existing entries are first removed, so that the saved state exactly reflects the state
	// of the model (indices aren't simply overwritten).
	std::vector<std::wstring> subKeyNames;
	wil::unique_hkey childKey;
	size_t index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey)
		== ERROR_SUCCESS)
	{
		subKeyNames.push_back(std::to_wstring(index));
		index++;
	}

	for (const auto &subKeyName : subKeyNames)
	{
		RegDeleteTree(parentKey, subKeyName.c_str());
	}

	index = 0;

	for (const auto &lifespanEntry : model->GetAllLifespans())
	{
		LSTATUS res = RegCreateKeyEx(parentKey, std::to_wstring(index).c_str(), 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &childKey, nullptr);

		if (res == ERROR_SUCCESS)
		{
			SaveLifespan(childKey.get(), lifespanEntry);

			index++;
		}
	}
}

}

void Load(HKEY applicationKey, LifespanModel *model)
{
	wil::unique_hkey lifespansKey;
	LSTATUS res = RegOpenKeyEx(applicationKey, LIFESPANS_KEY_PATH, 0, KEY_READ, &lifespansKey);

	if (res == ERROR_SUCCESS)
	{
		model->RemoveAll();

		LoadFromKey(lifespansKey.get(), model);
	}
}

void Save(HKEY applicationKey, const LifespanModel *model)
{
	wil::unique_hkey lifespansKey;
	LSTATUS res = RegCreateKeyEx(applicationKey, LIFESPANS_KEY_PATH, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &lifespansKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		SaveToKey(lifespansKey.get(), model);
	}
}

}
