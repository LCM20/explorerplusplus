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
#include "Storage.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include <format>
#include <fstream>

namespace
{

// The name of the file that records items deleted because their lifespan expired. The log is
// written to the same directory as the configuration file, so that it's easy to locate.
constexpr wchar_t DELETION_LOG_FILENAME[] = L"lifespan-deletions.log";

std::wstring GetDeletionLogPath()
{
	std::filesystem::path path(Storage::GetConfigFilePath());
	path.replace_filename(DELETION_LOG_FILENAME);
	return path.wstring();
}

// Returns the current local time in a sortable, human-readable form (e.g. "2026-11-17 23:46:14").
std::wstring GetCurrentTimeString()
{
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	return std::format(L"{:04}-{:02}-{:02} {:02}:{:02}:{:02}", localTime.wYear, localTime.wMonth,
		localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond);
}

// Appends a single line to the deletion log. The file is opened in append mode (so existing
// records are never overwritten) and flushed before returning, so that the record is durable even
// if the application subsequently crashes.
void AppendLogLine(const std::wstring &message)
{
	std::ofstream file(std::filesystem::path(GetDeletionLogPath()),
		std::ios::binary | std::ios::app);

	if (!file.is_open())
	{
		return;
	}

	// If the file has just been created, write a UTF-8 BOM so that text editors detect the
	// encoding correctly.
	file.seekp(0, std::ios::end);

	if (file.tellp() == 0)
	{
		constexpr unsigned char utf8Bom[] = { 0xEF, 0xBB, 0xBF };
		file.write(reinterpret_cast<const char *>(utf8Bom), sizeof(utf8Bom));
	}

	auto utf8Message = wstrToUtf8Str(message);
	file.write(utf8Message.data(), static_cast<std::streamsize>(utf8Message.size()));
	file.write("\r\n", 2);
	file.flush();
}

// Records that an item is about to be deleted. This is logged before the deletion is attempted,
// so that a crash cannot leave a deleted item unaccounted for.
void LogDeletionAttempt(const std::wstring &path, const FILETIME &expiryTime)
{
	AppendLogLine(std::format(L"[{}] 有效期到期，准备删除: {} (到期时间: {})", GetCurrentTimeString(), path,
		LifespanHelper::FormatExpiryTime(expiryTime)));
}

void LogDeletionResult(const std::wstring &path, HRESULT result)
{
	if (SUCCEEDED(result))
	{
		AppendLogLine(std::format(L"[{}] 有效期到期，删除成功 (已移入回收站): {}", GetCurrentTimeString(), path));
	}
	else
	{
		AppendLogLine(std::format(L"[{}] 有效期到期，删除失败 (错误码 0x{:08X}): {}", GetCurrentTimeString(),
			static_cast<unsigned long>(result), path));
	}
}

}

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

		// Record the deletion before it's attempted, so that there's always a durable record of
		// what was about to be removed.
		LogDeletionAttempt(entry.path, entry.expiryTime);

		FileActionHandler fileActionHandler;

		// The item is moved to the recycle bin, rather than being permanently deleted. If the
		// delete operation fails (e.g. because the recycle bin isn't available for a network
		// path), the entry is left in place and will be retried on the next check. The item is
		// never permanently deleted.
		HRESULT deleteResult = fileActionHandler.DeleteFiles(MaybeGetOwnerWindow(),
			{ pidl.Raw() }, false, true);

		LogDeletionResult(entry.path, deleteResult);

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
