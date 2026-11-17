// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <string>

class ResourceLoader;

namespace LifespanHelper
{

// Returns the expiry time that is the specified number of days from the current time.
FILETIME GetExpiryTimeFromNow(int days);

bool IsExpired(const FILETIME &expiryTime);

// Returns a user-friendly description of the time remaining until the expiry time (e.g.
// "3 天"). If the expiry time has already passed, a string indicating that is returned instead.
std::wstring FormatRemainingTime(const FILETIME &expiryTime, const ResourceLoader *resourceLoader);

// Returns the expiry time as a localized date/time string.
std::wstring FormatExpiryTime(const FILETIME &expiryTime);

}
