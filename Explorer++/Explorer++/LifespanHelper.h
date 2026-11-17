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

// Returns a special expiry time that represents a permanent lifespan (i.e. one that never
// expires).
FILETIME GetPermanentExpiryTime();

bool IsExpired(const FILETIME &expiryTime);

// Returns true if the expiry time represents a permanent lifespan.
bool IsPermanent(const FILETIME &expiryTime);

// Returns a user-friendly description of the time remaining until the expiry time (e.g.
// "3 天"). If the expiry time has already passed, a string indicating that is returned instead.
std::wstring FormatRemainingTime(const FILETIME &expiryTime, const ResourceLoader *resourceLoader);

// Returns the expiry time as a localized date/time string.
std::wstring FormatExpiryTime(const FILETIME &expiryTime);

}
