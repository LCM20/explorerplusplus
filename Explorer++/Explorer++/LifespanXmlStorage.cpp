// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LifespanModel.h"
#include "LifespanXmlStorage.h"
#include "../Helper/XMLSettings.h"

namespace LifespanXmlStorage
{

namespace
{

const wchar_t LIFESPANS_NODE_NAME[] = L"Lifespans";

const wchar_t SETTING_PATH[] = L"Path";
const wchar_t SETTING_EXPIRY_TIME[] = L"ExpiryTime";

std::unique_ptr<LifespanEntry> LoadLifespan(IXMLDOMNode *parentNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = parentNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::wstring path;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_PATH, path);

	if (FAILED(hr))
	{
		return nullptr;
	}

	auto lifespanEntry = std::make_unique<LifespanEntry>();
	lifespanEntry->path = path;

	// The expiry time is essential; an entry without it is ignored, since there's no sensible
	// fallback value.
	if (!XMLSettings::ReadDateTime(attributeMap.get(), SETTING_EXPIRY_TIME,
			lifespanEntry->expiryTime))
	{
		return nullptr;
	}

	return lifespanEntry;
}

void LoadFromNode(IXMLDOMNode *parentNode, LifespanModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr_nothrow(L"./Lifespan");

	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = parentNode->selectNodes(queryString.get(), &childNodes);

	if (FAILED(hr))
	{
		return;
	}

	while (childNodes->nextNode(&childNode) == S_OK)
	{
		auto lifespanEntry = LoadLifespan(childNode.get());

		if (!lifespanEntry)
		{
			continue;
		}

		model->RestoreLifespan(std::move(*lifespanEntry));
	}
}

void SaveLifespan(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const LifespanEntry &lifespanEntry)
{
	wil::com_ptr_nothrow<IXMLDOMElement> lifespanNode;
	XMLSettings::CreateElementNode(xmlDocument, &lifespanNode, parentNode, _T("Lifespan"), _T(""));
	XMLSettings::AddAttributeToNode(xmlDocument, lifespanNode.get(), SETTING_PATH,
		lifespanEntry.path.c_str());
	XMLSettings::SaveDateTime(xmlDocument, lifespanNode.get(), SETTING_EXPIRY_TIME,
		lifespanEntry.expiryTime);
}

}

void Load(IXMLDOMNode *rootNode, LifespanModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> lifespansNode;
	auto queryString = wil::make_bstr_nothrow(LIFESPANS_NODE_NAME);
	HRESULT hr = rootNode->selectSingleNode(queryString.get(), &lifespansNode);

	if (hr != S_OK)
	{
		return;
	}

	model->RemoveAll();

	LoadFromNode(lifespansNode.get(), model);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const LifespanModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMElement> lifespansNode;
	auto nodeName = wil::make_bstr_nothrow(LIFESPANS_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(nodeName.get(), &lifespansNode);

	if (FAILED(hr))
	{
		return;
	}

	for (const auto &lifespanEntry : model->GetAllLifespans())
	{
		SaveLifespan(xmlDocument, lifespansNode.get(), lifespanEntry);
	}

	XMLSettings::AppendChildToParent(lifespansNode.get(), rootNode);
}

}
