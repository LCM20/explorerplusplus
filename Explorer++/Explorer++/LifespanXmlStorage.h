// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct IXMLDOMDocument;
struct IXMLDOMNode;

class LifespanModel;

namespace LifespanXmlStorage
{

void Load(IXMLDOMNode *rootNode, LifespanModel *model);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const LifespanModel *model);

}
