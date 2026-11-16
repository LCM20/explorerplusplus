// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct IXMLDOMDocument;
struct IXMLDOMNode;

class NotesModel;

namespace NotesXmlStorage
{

void Load(IXMLDOMNode *rootNode, NotesModel *model);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const NotesModel *model);

}
