// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>

class NotesModel;

namespace NotesRegistryStorage
{

void Load(HKEY applicationKey, NotesModel *model);
void Save(HKEY applicationKey, const NotesModel *model);

}
