// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LifespanHelper.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/Helper.h"

namespace
{

// There are 10,000,000 100-nanosecond intervals in a second.
constexpr ULONGLONG INTERVALS_PER_SECOND = 10000000;
constexpr ULONGLONG SECONDS_PER_MINUTE = 60;
constexpr ULONGLONG MINUTES_PER_HOUR = 60;
constexpr ULONGLONG HOURS_PER_DAY = 24;

ULONGLONG GetRemainingIntervals(const FILETIME &expiryTime)
{
	ULARGE_INTEGER now;
	GetSystemTimeAsFileTime(reinterpret_cast<FILETIME *>(&now));

	ULARGE_INTEGER expiry;
	expiry.HighPart = expiryTime.dwHighDateTime;
	expiry.LowPart = expiryTime.dwLowDateTime;

	if (expiry.QuadPart <= now.QuadPart)
	{
		return 0;
	}

	return expiry.QuadPart - now.QuadPart;
}

}

namespace LifespanHelper
{

FILETIME GetExpiryTimeFromNow(int days)
{
	ULARGE_INTEGER now;
	GetSystemTimeAsFileTime(reinterpret_cast<FILETIME *>(&now));

	ULARGE_INTEGER expiry;
	expiry.QuadPart = now.QuadPart
		+ static_cast<ULONGLONG>(days) * HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE
			* INTERVALS_PER_SECOND;

	FILETIME expiryTime;
	expiryTime.dwHighDateTime = expiry.HighPart;
	expiryTime.dwLowDateTime = expiry.LowPart;

	return expiryTime;
}

bool IsExpired(const FILETIME &expiryTime)
{
	FILETIME now;
	GetSystemTimeAsFileTime(&now);

	return CompareFileTime(&expiryTime, &now) <= 0;
}

std::wstring FormatRemainingTime(const FILETIME &expiryTime, const ResourceLoader *resourceLoader)
{
	auto remainingIntervals = GetRemainingIntervals(expiryTime);

	if (remainingIntervals == 0)
	{
		return resourceLoader->LoadString(IDS_LIFESPAN_EXPIRED);
	}

	auto remainingSeconds = remainingIntervals / INTERVALS_PER_SECOND;

	// Each quantity is rounded up, so that the text never states that less than one unit
	// remains. For example, 5 seconds remaining will be shown as "1 分钟".
	auto remainingMinutes = (remainingSeconds + SECONDS_PER_MINUTE - 1) / SECONDS_PER_MINUTE;

	if (remainingMinutes < MINUTES_PER_HOUR)
	{
		return std::to_wstring(remainingMinutes) + L" "
			+ resourceLoader->LoadString(IDS_LIFESPAN_UNIT_MINUTES);
	}

	auto remainingHours = (remainingMinutes + MINUTES_PER_HOUR - 1) / MINUTES_PER_HOUR;

	if (remainingHours < 2 * HOURS_PER_DAY)
	{
		return std::to_wstring(remainingHours) + L" "
			+ resourceLoader->LoadString(IDS_LIFESPAN_UNIT_HOURS);
	}

	auto remainingDays = (remainingHours + HOURS_PER_DAY - 1) / HOURS_PER_DAY;

	return std::to_wstring(remainingDays) + L" "
		+ resourceLoader->LoadString(IDS_LIFESPAN_UNIT_DAYS);
}

std::wstring FormatExpiryTime(const FILETIME &expiryTime)
{
	TCHAR buffer[64];
	BOOL res = CreateFileTimeString(&expiryTime, buffer, std::size(buffer), FALSE);

	if (!res)
	{
		return {};
	}

	return buffer;
}

}
