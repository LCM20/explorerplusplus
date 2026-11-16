// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NotesModel.h"
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

std::optional<std::wstring> NotesModel::MaybeGetNote(const std::wstring &path) const
{
	auto itr = m_notes.find(GetKey(path));

	if (itr == m_notes.end())
	{
		return std::nullopt;
	}

	return itr->second.note;
}

void NotesModel::SetNote(const std::wstring &path, const std::wstring &note)
{
	if (note.empty())
	{
		RemoveNote(path);
		return;
	}

	NoteEntry noteEntry;
	noteEntry.path = path;
	noteEntry.note = note;
	GetSystemTimeAsFileTime(&noteEntry.lastModified);

	m_notes[GetKey(path)] = noteEntry;
}

void NotesModel::RemoveNote(const std::wstring &path)
{
	m_notes.erase(GetKey(path));
}

void NotesModel::RemoveAll()
{
	m_notes.clear();
}

void NotesModel::RestoreNote(NoteEntry noteEntry)
{
	m_notes[GetKey(noteEntry.path)] = std::move(noteEntry);
}

std::vector<NoteEntry> NotesModel::GetAllNotes() const
{
	std::vector<NoteEntry> notes;
	notes.reserve(m_notes.size());

	for (const auto &entry : m_notes)
	{
		notes.push_back(entry.second);
	}

	return notes;
}
