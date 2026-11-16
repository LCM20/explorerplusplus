// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct NoteEntry
{
	std::wstring path;
	std::wstring note;
	FILETIME lastModified;
};

// Holds the set of notes that have been attached to files and folders. Notes are keyed by item
// path, which means they're independent of any particular folder view; the note for an item will
// be shown whenever that item is displayed in details view.
class NotesModel
{
public:
	std::optional<std::wstring> MaybeGetNote(const std::wstring &path) const;

	// An empty note will remove any existing note for the item.
	void SetNote(const std::wstring &path, const std::wstring &note);
	void RemoveNote(const std::wstring &path);
	void RemoveAll();

	// Used when restoring notes from persistent storage, where the last modified timestamp is
	// already known.
	void RestoreNote(NoteEntry noteEntry);

	std::vector<NoteEntry> GetAllNotes() const;

private:
	// The key is the lowercase form of the item path, since paths are case-insensitive on Windows.
	std::unordered_map<std::wstring, NoteEntry> m_notes;
};
