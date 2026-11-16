// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "RebarBandView.h"
#include "ViewBase.h"
#include "../Helper/Pidl.h"
#include <wil/resource.h>
#include <optional>
#include <string>
#include <vector>

class AddressBarViewDelegate;
struct Config;

// A breadcrumb-style address bar. The path of the current directory is split into a set of
// segments (one for each ancestor folder, starting at the desktop). Clicking a segment navigates
// to that folder, while clicking the chevron next to a segment displays a dropdown menu
// containing the subfolders of that segment. Clicking in the empty area to the right of the last
// segment (or focusing the control via its accelerator) switches the control into edit mode, in
// which a path can be entered directly.
class AddressBarView : public ViewBase, public RebarBandView
{
public:
	AddressBarView(ViewBase *parent, const Config *config);
	~AddressBarView();

	void SetDelegate(AddressBarViewDelegate *delegate);

	// RebarBandView
	HWND GetHWND() const override;
	int GetHeight() const override;

	void Focus();
	std::wstring GetText() const;
	bool IsTextModified() const;
	void SelectAllText();
	void UpdateTextAndIcon(const std::optional<std::wstring> &optionalText, int iconIndex,
		const PidlAbsolute *optionalPidl = nullptr);
	void RevertText();
	void ShowDropDown();

	AddressBarViewDelegate *GetDelegateForTesting();
	void SetTextForTesting(const std::wstring &text);

private:
	static inline const WCHAR CLASS_NAME[] = L"Explorer++ AddressBar";

	enum class HotPart
	{
		None,
		Button,
		Dropdown,
		Overflow
	};

	struct Segment
	{
		PidlAbsolute pidl;
		std::wstring name;
		int textWidth = 0;
		RECT buttonRect = {};
		RECT dropdownRect = {};
		bool hidden = false;
	};

	static HWND CreateAddressBar(ViewBase *parent);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnPaint(HWND hwnd);
	void DrawSegment(HDC hdc, size_t segmentIndex);
	void DrawSegmentBackground(HDC hdc, const RECT &rect, bool pressed);
	void DrawChevron(HDC hdc, const RECT &rect, bool pointDown);
	void DrawOverflowButton(HDC hdc, const RECT &rect);
	void OnMouseMove(const POINT &pt);
	void OnLButtonDown(const POINT &pt);
	void OnLButtonUp(const POINT &pt);
	void OnFontOrDpiUpdated();

	HotPart HitTest(const POINT &pt, size_t &segmentIndex) const;
	void UpdateHotItem(HotPart part, size_t segmentIndex);
	void UpdateLayout();
	void RebuildSegments(const PidlAbsolute &pidl);
	int CalculateHeight() const;
	int ScaleValue(int value) const;
	HFONT GetEffectiveFont() const;

	void EnterEditMode(bool selectAll);
	void ExitEditMode();
	void PositionEditControl();
	void ShowSegmentDropdown(size_t segmentIndex);
	void ShowOverflowMenu();
	std::vector<std::pair<PidlAbsolute, std::wstring>> EnumerateChildFolders(
		const PidlAbsolute &pidl) const;

	AddressBarViewDelegate *m_delegate = nullptr;
	HWND m_edit = nullptr;
	HFONT m_font = nullptr;

	// An HTHEME. This is stored as a HANDLE so that this header doesn't need to include
	// uxtheme.h. The handle is closed when the view is destroyed.
	HANDLE m_theme = nullptr;
	wil::unique_hicon m_desktopIcon;
	std::wstring m_currentText;
	std::vector<Segment> m_segments;
	int m_iconIndex = -1;
	bool m_editMode = false;
	bool m_hasOverflow = false;
	RECT m_overflowRect = {};
	HotPart m_hotPart = HotPart::None;
	size_t m_hotSegmentIndex = 0;
	int m_pressedSegmentIndex = -1;
	int m_openDropdownSegmentIndex = -1;
	POINT m_dragStartPoint = {};
	bool m_potentialDrag = false;
	bool m_trackingMouse = false;
	int m_cachedHeight = 0;

	std::unique_ptr<MainFontSetter> m_fontSetter;
};
