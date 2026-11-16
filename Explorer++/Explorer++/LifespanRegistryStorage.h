// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>

class LifespanModel;

namespace LifespanRegistryStorage
{

void Load(HKEY applicationKey, LifespanModel *model);
void Save(HKEY applicationKey, const LifespanModel *model);

}
