// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NotesModel.h"
#include "NotesRegistryStorage.h"
#include "../Helper/RegistrySettings.h"

namespace NotesRegistryStorage
{

namespace
{

const wchar_t NOTES_KEY_PATH[] = L"Notes";

const wchar_t SETTING_PATH[] = L"Path";
const wchar_t SETTING_NOTE[] = L"Note";
const wchar_t SETTING_LAST_MODIFIED[] = L"LastModified";

std::unique_ptr<NoteEntry> LoadNote(HKEY key)
{
	std::wstring path;
	LSTATUS result = RegistrySettings::ReadString(key, SETTING_PATH, path);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	std::wstring note;
	result = RegistrySettings::ReadString(key, SETTING_NOTE, note);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	auto noteEntry = std::make_unique<NoteEntry>();
	noteEntry->path = path;
	noteEntry->note = note;

	// The timestamp isn't essential, so it's not treated as an error if it can't be read.
	RegistrySettings::ReadDateTime(key, SETTING_LAST_MODIFIED, noteEntry->lastModified);

	return noteEntry;
}

void LoadFromKey(HKEY parentKey, NotesModel *model)
{
	wil::unique_hkey childKey;
	size_t index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey)
		== ERROR_SUCCESS)
	{
		auto noteEntry = LoadNote(childKey.get());

		if (noteEntry)
		{
			model->RestoreNote(std::move(*noteEntry));
		}

		index++;
	}
}

void SaveNote(HKEY key, const NoteEntry &noteEntry)
{
	RegistrySettings::SaveString(key, SETTING_PATH, noteEntry.path);
	RegistrySettings::SaveString(key, SETTING_NOTE, noteEntry.note);
	RegistrySettings::SaveDateTime(key, SETTING_LAST_MODIFIED, noteEntry.lastModified);
}

void SaveToKey(HKEY parentKey, const NotesModel *model)
{
	size_t index = 0;

	for (const auto &noteEntry : model->GetAllNotes())
	{
		wil::unique_hkey childKey;
		LSTATUS res = RegCreateKeyEx(parentKey, std::to_wstring(index).c_str(), 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &childKey, nullptr);

		if (res == ERROR_SUCCESS)
		{
			SaveNote(childKey.get(), noteEntry);

			index++;
		}
	}
}

}

void Load(HKEY applicationKey, NotesModel *model)
{
	wil::unique_hkey notesKey;
	LSTATUS res = RegOpenKeyEx(applicationKey, NOTES_KEY_PATH, 0, KEY_READ, &notesKey);

	if (res == ERROR_SUCCESS)
	{
		model->RemoveAll();

		LoadFromKey(notesKey.get(), model);
	}
}

void Save(HKEY applicationKey, const NotesModel *model)
{
	wil::unique_hkey notesKey;
	LSTATUS res = RegCreateKeyEx(applicationKey, NOTES_KEY_PATH, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &notesKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		SaveToKey(notesKey.get(), model);
	}
}

}
