// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NotesModel.h"
#include "NotesXmlStorage.h"
#include "../Helper/XMLSettings.h"

namespace NotesXmlStorage
{

namespace
{

const wchar_t NOTES_NODE_NAME[] = L"Notes";

const wchar_t SETTING_PATH[] = L"Path";
const wchar_t SETTING_LAST_MODIFIED[] = L"LastModified";

std::unique_ptr<NoteEntry> LoadNote(IXMLDOMNode *parentNode)
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

	std::wstring note;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), L"name", note);

	if (FAILED(hr))
	{
		return nullptr;
	}

	auto noteEntry = std::make_unique<NoteEntry>();
	noteEntry->path = path;
	noteEntry->note = note;

	// The timestamp isn't essential, so it's not treated as an error if it can't be read.
	XMLSettings::ReadDateTime(attributeMap.get(), SETTING_LAST_MODIFIED, noteEntry->lastModified);

	return noteEntry;
}

void LoadFromNode(IXMLDOMNode *parentNode, NotesModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr_nothrow(L"./Note");

	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = parentNode->selectNodes(queryString.get(), &childNodes);

	if (FAILED(hr))
	{
		return;
	}

	while (childNodes->nextNode(&childNode) == S_OK)
	{
		auto noteEntry = LoadNote(childNode.get());

		if (!noteEntry)
		{
			continue;
		}

		model->RestoreNote(std::move(*noteEntry));
	}
}

void SaveNote(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode, const NoteEntry &noteEntry)
{
	wil::com_ptr_nothrow<IXMLDOMElement> noteNode;
	XMLSettings::CreateElementNode(xmlDocument, &noteNode, parentNode, _T("Note"),
		noteEntry.note.c_str());
	XMLSettings::AddAttributeToNode(xmlDocument, noteNode.get(), SETTING_PATH,
		noteEntry.path.c_str());
	XMLSettings::SaveDateTime(xmlDocument, noteNode.get(), SETTING_LAST_MODIFIED,
		noteEntry.lastModified);
}

}

void Load(IXMLDOMNode *rootNode, NotesModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> notesNode;
	auto queryString = wil::make_bstr_nothrow(NOTES_NODE_NAME);
	HRESULT hr = rootNode->selectSingleNode(queryString.get(), &notesNode);

	if (hr != S_OK)
	{
		return;
	}

	model->RemoveAll();

	LoadFromNode(notesNode.get(), model);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const NotesModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMElement> notesNode;
	auto nodeName = wil::make_bstr_nothrow(NOTES_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(nodeName.get(), &notesNode);

	if (FAILED(hr))
	{
		return;
	}

	for (const auto &noteEntry : model->GetAllNotes())
	{
		SaveNote(xmlDocument, notesNode.get(), noteEntry);
	}

	XMLSettings::AppendChildToParent(notesNode.get(), rootNode);
}

}
