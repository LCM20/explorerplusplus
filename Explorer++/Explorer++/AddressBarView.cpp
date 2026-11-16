// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddressBarView.h"
#include "AddressBarViewDelegate.h"
#include "TestHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <Shlwapi.h>
#include <vssym32.h>
#include <vsstyle.h>

namespace
{

// Any ampersand in a menu item name needs to be escaped, so that it's not treated as an
// accelerator prefix.
std::wstring EscapeNameForMenu(const std::wstring &name)
{
	std::wstring escapedName = name;

	for (size_t pos = 0; (pos = escapedName.find(L'&', pos)) != std::wstring::npos; pos += 2)
	{
		escapedName.insert(pos, L"&");
	}

	return escapedName;
}

}

// The horizontal padding applied to the left and right of the control (at 96 DPI).
constexpr int PADDING_X = 4;

// The horizontal padding applied on each side of the text within a segment (at 96 DPI).
constexpr int TEXT_PADDING_X = 4;

// The width of the dropdown chevron that's shown next to each segment (at 96 DPI).
constexpr int CHEVRON_WIDTH = 12;

// The width of the overflow button (at 96 DPI).
constexpr int OVERFLOW_WIDTH = 16;

// The size of the icon shown for the root (desktop) segment (at 96 DPI).
constexpr int ICON_SIZE = 16;

AddressBarView::AddressBarView(ViewBase *parent, const Config *config) :
	ViewBase(CreateAddressBar(parent))
{
	AddSubclass(
		std::make_unique<WindowSubclass>(GetHWND(), std::bind_front(&AddressBarView::WndProc, this)));

	m_edit = CreateWindowEx(0, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, GetHWND(),
		nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(m_edit);

	SendMessage(m_edit, WM_SETFONT, reinterpret_cast<WPARAM>(GetEffectiveFont()), false);

	AddSubclass(std::make_unique<WindowSubclass>(m_edit,
		std::bind_front(&AddressBarView::EditSubclass, this)));

	HRESULT hr = SHAutoComplete(m_edit, SHACF_FILESYSTEM | SHACF_AUTOSUGGEST_FORCE_ON);
	DCHECK(SUCCEEDED(hr));

	m_theme = OpenThemeData(GetHWND(), L"Toolbar");

	// Constructing the font setter results in an initial WM_SETFONT message being sent to the
	// window. Therefore, this needs to be done after the subclass has been installed and the edit
	// control has been created.
	m_fontSetter = std::make_unique<MainFontSetter>(GetHWND(), config);
	m_fontSetter->fontUpdatedSignal.AddObserver(
		std::bind(&AddressBarView::OnFontOrDpiUpdated, this));

	OnFontOrDpiUpdated();
}

AddressBarView::~AddressBarView()
{
	if (m_theme)
	{
		CloseThemeData(static_cast<HTHEME>(m_theme));
	}
}

HWND AddressBarView::CreateAddressBar(ViewBase *parent)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		WNDCLASSEX windowClass = {};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.style = 0;
		windowClass.lpfnWndProc = DefWindowProc;
		windowClass.hInstance = GetModuleHandle(nullptr);
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = CLASS_NAME;
		auto res = RegisterClassEx(&windowClass);
		CHECK(res);

		classRegistered = true;
	}

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0,
		parent->GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);

	return hwnd;
}

void AddressBarView::SetDelegate(AddressBarViewDelegate *delegate)
{
	m_delegate = delegate;
}

HWND AddressBarView::GetHWND() const
{
	return ViewBase::GetHWND();
}

int AddressBarView::GetHeight() const
{
	return m_cachedHeight;
}

LRESULT AddressBarView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		OnPaint(hwnd);
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_LBUTTONDOWN:
		OnLButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_LBUTTONUP:
		OnLButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_MOUSELEAVE:
		m_trackingMouse = false;
		UpdateHotItem(HotPart::None, 0);
		break;

	case WM_CAPTURECHANGED:
		m_pressedSegmentIndex = -1;
		m_potentialDrag = false;
		InvalidateRect(hwnd, nullptr, false);
		break;

	case WM_SETFOCUS:
		if (m_delegate)
		{
			m_delegate->OnFocused();
		}
		break;

	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			EnterEditMode(true);
			return 0;
		}
		break;

	case WM_RBUTTONUP:
		if (!m_editMode)
		{
			EnterEditMode(false);
			return 0;
		}
		break;

	case WM_SETFONT:
		m_font = reinterpret_cast<HFONT>(wParam);
		SendMessage(m_edit, WM_SETFONT, wParam, MAKELPARAM(true, 0));
		OnFontOrDpiUpdated();
		return 0;

	case WM_SIZE:
		UpdateLayout();
		PositionEditControl();
		break;

	case WM_DPICHANGED_AFTERPARENT:
		OnFontOrDpiUpdated();
		break;

	case WM_THEMECHANGED:
		if (m_theme)
		{
			CloseThemeData(static_cast<HTHEME>(m_theme));
		}

		m_theme = OpenThemeData(hwnd, L"Toolbar");
		InvalidateRect(hwnd, nullptr, false);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT AddressBarView::EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (m_delegate && m_delegate->OnKeyPressed(static_cast<UINT>(wParam)))
		{
			return 0;
		}
		break;

	case WM_SETFOCUS:
		if (m_delegate)
		{
			m_delegate->OnFocused();
		}
		break;

	case WM_KILLFOCUS:
		// Once the focus moves elsewhere, edit mode is exited. Note that when the focus moves to
		// the parent (breadcrumb) window, edit mode is retained. That happens, for example, when
		// the focus accelerator is pressed while the control is already in edit mode.
		if (reinterpret_cast<HWND>(wParam) != GetHWND())
		{
			ExitEditMode();
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void AddressBarView::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	RECT clientRect;
	auto res = GetClientRect(hwnd, &clientRect);
	CHECK(res);

	int width = GetRectWidth(&clientRect);
	int height = GetRectHeight(&clientRect);

	HDC memoryDC = CreateCompatibleDC(hdc);
	wil::unique_hbitmap bitmap(CreateCompatibleBitmap(hdc, width, height));
	HGDIOBJ oldBitmap = SelectObject(memoryDC, bitmap.get());

	FillRect(memoryDC, &clientRect, GetSysColorBrush(COLOR_WINDOW));

	// Draw a border that matches the one shown around a standard edit control.
	DrawEdge(memoryDC, &clientRect, EDGE_SUNKEN, BF_RECT);

	if (!m_editMode)
	{
		HGDIOBJ oldFont = SelectObject(memoryDC, GetEffectiveFont());
		SetBkMode(memoryDC, TRANSPARENT);
		SetTextColor(memoryDC, GetSysColor(COLOR_WINDOWTEXT));

		for (size_t i = 0; i < m_segments.size(); i++)
		{
			DrawSegment(memoryDC, i);
		}

		if (m_hasOverflow)
		{
			DrawOverflowButton(memoryDC, m_overflowRect);
		}

		SelectObject(memoryDC, oldFont);
	}

	BitBlt(hdc, 0, 0, width, height, memoryDC, 0, 0, SRCCOPY);

	SelectObject(memoryDC, oldBitmap);
	DeleteDC(memoryDC);

	EndPaint(hwnd, &ps);
}

void AddressBarView::DrawSegment(HDC hdc, size_t segmentIndex)
{
	const auto &segment = m_segments[segmentIndex];

	if (segment.hidden)
	{
		return;
	}

	RECT segmentRect = segment.buttonRect;
	segmentRect.right = segment.dropdownRect.right;

	bool hot = m_hotSegmentIndex == segmentIndex
		&& (m_hotPart == HotPart::Button || m_hotPart == HotPart::Dropdown);
	bool pressed = hot && m_pressedSegmentIndex == static_cast<int>(segmentIndex)
		&& m_hotPart == HotPart::Button;
	bool dropdownOpen = m_openDropdownSegmentIndex == static_cast<int>(segmentIndex);

	if (pressed || dropdownOpen)
	{
		DrawSegmentBackground(hdc, segmentRect, true);
	}
	else if (hot)
	{
		DrawSegmentBackground(hdc, segmentRect, false);
	}

	if (segmentIndex == 0)
	{
		// The first segment represents the desktop and is shown as an icon.
		if (!m_desktopIcon)
		{
			SHFILEINFO fileInfo = {};

			if (SHGetFileInfo(reinterpret_cast<PCWSTR>(segment.pidl.Raw()), 0, &fileInfo,
					sizeof(fileInfo), SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON)
				&& fileInfo.hIcon)
			{
				m_desktopIcon.reset(fileInfo.hIcon);
			}
		}

		if (m_desktopIcon)
		{
			int iconSize = ScaleValue(ICON_SIZE);
			int x = segment.buttonRect.left
				+ (GetRectWidth(&segment.buttonRect) - iconSize) / 2;
			int y = segment.buttonRect.top
				+ (GetRectHeight(&segment.buttonRect) - iconSize) / 2;
			DrawIconEx(hdc, x, y, m_desktopIcon.get(), iconSize, iconSize, 0, nullptr, DI_NORMAL);
		}
	}
	else
	{
		RECT textRect = segment.buttonRect;
		textRect.left += ScaleValue(TEXT_PADDING_X);
		DrawText(hdc, segment.name.c_str(), static_cast<int>(segment.name.length()), &textRect,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	}

	DrawChevron(hdc, segment.dropdownRect, hot || dropdownOpen);
}

void AddressBarView::DrawSegmentBackground(HDC hdc, const RECT &rect, bool pressed)
{
	if (m_theme)
	{
		DrawThemeBackground(static_cast<HTHEME>(m_theme), hdc, TP_BUTTON,
			pressed ? TS_PRESSED : TS_HOT, &rect, nullptr);
	}
	else
	{
		DrawEdge(hdc, const_cast<RECT *>(&rect), pressed ? BDR_SUNKENOUTER : BDR_RAISEDINNER,
			BF_RECT);
	}
}

void AddressBarView::DrawChevron(HDC hdc, const RECT &rect, bool pointDown)
{
	int centerX = rect.left + GetRectWidth(&rect) / 2;
	int centerY = rect.top + GetRectHeight(&rect) / 2;

	wil::unique_hpen pen(CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWTEXT)));
	HGDIOBJ oldPen = SelectObject(hdc, pen.get());

	if (pointDown)
	{
		MoveToEx(hdc, centerX - 3, centerY - 1, nullptr);
		LineTo(hdc, centerX, centerY + 2);
		LineTo(hdc, centerX + 3, centerY - 1);
	}
	else
	{
		MoveToEx(hdc, centerX - 1, centerY - 3, nullptr);
		LineTo(hdc, centerX + 2, centerY);
		LineTo(hdc, centerX - 1, centerY + 3);
	}

	SelectObject(hdc, oldPen);
}

void AddressBarView::DrawOverflowButton(HDC hdc, const RECT &rect)
{
	if (m_hotPart == HotPart::Overflow)
	{
		DrawSegmentBackground(hdc, rect, false);
	}

	// The overflow button is drawn as a double chevron ("<<").
	int centerX = rect.left + GetRectWidth(&rect) / 2;
	int centerY = rect.top + GetRectHeight(&rect) / 2;

	wil::unique_hpen pen(CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWTEXT)));
	HGDIOBJ oldPen = SelectObject(hdc, pen.get());

	for (int offset = -2; offset <= 1; offset += 3)
	{
		MoveToEx(hdc, centerX + offset + 2, centerY - 3, nullptr);
		LineTo(hdc, centerX + offset - 1, centerY);
		LineTo(hdc, centerX + offset + 2, centerY + 3);
	}

	SelectObject(hdc, oldPen);
}

void AddressBarView::OnMouseMove(const POINT &pt)
{
	if (m_editMode)
	{
		return;
	}

	size_t segmentIndex;
	HotPart part = HitTest(pt, segmentIndex);
	UpdateHotItem(part, segmentIndex);

	if (m_potentialDrag && m_pressedSegmentIndex >= 0)
	{
		if (std::abs(pt.x - m_dragStartPoint.x) > GetSystemMetrics(SM_CXDRAG)
			|| std::abs(pt.y - m_dragStartPoint.y) > GetSystemMetrics(SM_CYDRAG))
		{
			auto dragSegmentIndex = static_cast<size_t>(m_pressedSegmentIndex);
			m_potentialDrag = false;
			m_pressedSegmentIndex = -1;

			ReleaseCapture();
			UpdateHotItem(HotPart::None, 0);

			if (m_delegate)
			{
				// This call is modal and will only return once the drag has finished.
				m_delegate->OnBeginDrag(m_segments[dragSegmentIndex].pidl.Raw());
			}
		}
	}

	if (!m_trackingMouse)
	{
		TRACKMOUSEEVENT trackMouseEvent = { sizeof(trackMouseEvent), TME_LEAVE, GetHWND(), 0 };
		auto res = TrackMouseEvent(&trackMouseEvent);

		if (res)
		{
			m_trackingMouse = true;
		}
	}
}

void AddressBarView::OnLButtonDown(const POINT &pt)
{
	if (m_editMode)
	{
		return;
	}

	size_t segmentIndex;
	HotPart part = HitTest(pt, segmentIndex);

	switch (part)
	{
	case HotPart::Button:
		m_pressedSegmentIndex = static_cast<int>(segmentIndex);
		m_dragStartPoint = pt;
		m_potentialDrag = true;
		SetCapture(GetHWND());
		InvalidateRect(GetHWND(), nullptr, false);
		break;

	case HotPart::Dropdown:
		ShowSegmentDropdown(segmentIndex);
		break;

	case HotPart::Overflow:
		ShowOverflowMenu();
		break;

	case HotPart::None:
		EnterEditMode(true);
		break;
	}
}

void AddressBarView::OnLButtonUp(const POINT &pt)
{
	if (m_pressedSegmentIndex < 0)
	{
		return;
	}

	size_t segmentIndex;
	HotPart part = HitTest(pt, segmentIndex);

	bool activate = m_potentialDrag && part == HotPart::Button
		&& static_cast<int>(segmentIndex) == m_pressedSegmentIndex;

	PidlAbsolute selectedPidl;

	if (activate)
	{
		selectedPidl = m_segments[m_pressedSegmentIndex].pidl;
	}

	m_pressedSegmentIndex = -1;
	m_potentialDrag = false;

	ReleaseCapture();
	InvalidateRect(GetHWND(), nullptr, false);

	if (activate && m_delegate)
	{
		m_delegate->OnSegmentSelected(selectedPidl.Raw());
	}
}

AddressBarView::HotPart AddressBarView::HitTest(const POINT &pt, size_t &segmentIndex) const
{
	segmentIndex = 0;

	if (m_hasOverflow && PtInRect(&m_overflowRect, pt))
	{
		return HotPart::Overflow;
	}

	for (size_t i = 0; i < m_segments.size(); i++)
	{
		const auto &segment = m_segments[i];

		if (segment.hidden)
		{
			continue;
		}

		if (PtInRect(&segment.dropdownRect, pt))
		{
			segmentIndex = i;
			return HotPart::Dropdown;
		}

		if (PtInRect(&segment.buttonRect, pt))
		{
			segmentIndex = i;
			return HotPart::Button;
		}
	}

	return HotPart::None;
}

void AddressBarView::UpdateHotItem(HotPart part, size_t segmentIndex)
{
	if (m_hotPart == part && (part == HotPart::None || m_hotSegmentIndex == segmentIndex))
	{
		return;
	}

	m_hotPart = part;
	m_hotSegmentIndex = segmentIndex;

	InvalidateRect(GetHWND(), nullptr, false);
}

void AddressBarView::UpdateLayout()
{
	if (m_segments.empty())
	{
		m_hasOverflow = false;
		InvalidateRect(GetHWND(), nullptr, false);
		return;
	}

	RECT clientRect;
	auto res = GetClientRect(GetHWND(), &clientRect);
	CHECK(res);

	HDC hdc = GetDC(GetHWND());
	HGDIOBJ oldFont = SelectObject(hdc, GetEffectiveFont());

	for (auto &segment : m_segments)
	{
		segment.textWidth = 0;

		if (segment.name.empty())
		{
			continue;
		}

		SIZE size;
		GetTextExtentPoint32(hdc, segment.name.c_str(), static_cast<int>(segment.name.length()),
			&size);
		segment.textWidth = size.cx;
	}

	SelectObject(hdc, oldFont);
	ReleaseDC(GetHWND(), hdc);

	int iconSize = ScaleValue(ICON_SIZE);
	int chevronWidth = ScaleValue(CHEVRON_WIDTH);
	int textPadding = ScaleValue(TEXT_PADDING_X);

	auto getSegmentWidth = [&](size_t index)
	{
		int buttonWidth = (index == 0) ? iconSize + 2 * textPadding
									   : m_segments[index].textWidth + 2 * textPadding;
		return buttonWidth + chevronWidth;
	};

	int availableWidth = GetRectWidth(&clientRect) - 2 * ScaleValue(PADDING_X);

	for (auto &segment : m_segments)
	{
		segment.hidden = false;
	}

	m_hasOverflow = false;

	auto getTotalWidth = [&]()
	{
		int totalWidth = getSegmentWidth(0);

		if (m_hasOverflow)
		{
			totalWidth += ScaleValue(OVERFLOW_WIDTH);
		}

		for (size_t i = 1; i < m_segments.size(); i++)
		{
			if (!m_segments[i].hidden)
			{
				totalWidth += getSegmentWidth(i);
			}
		}

		return totalWidth;
	};

	// The root (desktop) segment is always shown. If there isn't enough space for all of the
	// segments, text segments are hidden from the left and can be accessed via the overflow
	// button.
	while (getTotalWidth() > availableWidth)
	{
		auto itr = std::find_if(m_segments.begin() + 1, m_segments.end(),
			[](const Segment &segment) { return !segment.hidden; });

		if (itr == m_segments.end())
		{
			break;
		}

		itr->hidden = true;
		m_hasOverflow = true;
	}

	int top = ScaleValue(2);
	int bottom = clientRect.bottom - ScaleValue(2);
	int x = clientRect.left + ScaleValue(PADDING_X);

	auto positionSegment = [&](size_t index)
	{
		auto &segment = m_segments[index];
		int buttonWidth = (index == 0) ? iconSize + 2 * textPadding
									   : segment.textWidth + 2 * textPadding;

		segment.buttonRect = { x, top, x + buttonWidth, bottom };
		segment.dropdownRect = { x + buttonWidth, top, x + buttonWidth + chevronWidth, bottom };

		x += buttonWidth + chevronWidth;
	};

	positionSegment(0);

	if (m_hasOverflow)
	{
		m_overflowRect = { x, top, x + ScaleValue(OVERFLOW_WIDTH), bottom };
		x += ScaleValue(OVERFLOW_WIDTH);
	}
	else
	{
		m_overflowRect = {};
	}

	for (size_t i = 1; i < m_segments.size(); i++)
	{
		auto &segment = m_segments[i];

		if (segment.hidden)
		{
			segment.buttonRect = {};
			segment.dropdownRect = {};
			continue;
		}

		positionSegment(i);
	}

	InvalidateRect(GetHWND(), nullptr, false);
}

void AddressBarView::RebuildSegments(const PidlAbsolute &pidl)
{
	m_segments.clear();

	std::vector<PidlAbsolute> chain;
	PidlAbsolute current(pidl);

	while (current.HasValue())
	{
		chain.push_back(current);

		if (ILIsEmpty(current.Raw()) || !current.RemoveLastItem())
		{
			break;
		}
	}

	std::reverse(chain.begin(), chain.end());

	for (const auto &prefixPidl : chain)
	{
		Segment segment;
		segment.pidl = prefixPidl;

		// The first segment represents the desktop (which has an empty pidl) and is shown as an
		// icon, rather than as text.
		if (!ILIsEmpty(prefixPidl.Raw()))
		{
			segment.name = GetDisplayNameWithFallback(prefixPidl.Raw(), SHGDN_NORMAL);
		}

		m_segments.push_back(std::move(segment));
	}

	UpdateLayout();
}

int AddressBarView::CalculateHeight() const
{
	HDC hdc = GetDC(GetHWND());
	HGDIOBJ oldFont = SelectObject(hdc, GetEffectiveFont());

	TEXTMETRIC textMetrics;
	GetTextMetrics(hdc, &textMetrics);

	SelectObject(hdc, oldFont);
	ReleaseDC(GetHWND(), hdc);

	int contentHeight = std::max<int>(textMetrics.tmHeight, ScaleValue(ICON_SIZE));

	// The additional space here accounts for the padding above/below the content and the 1 pixel
	// border on the top/bottom of the control.
	return contentHeight + 2 * ScaleValue(3) + 2;
}

int AddressBarView::ScaleValue(int value) const
{
	return DpiCompatibility::GetInstance().ScaleValue(GetHWND(), value);
}

HFONT AddressBarView::GetEffectiveFont() const
{
	return m_font ? m_font : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
}

void AddressBarView::OnFontOrDpiUpdated()
{
	m_cachedHeight = CalculateHeight();
	UpdateLayout();
	NotifySizeRequirementsChanged();
}

void AddressBarView::EnterEditMode(bool selectAll)
{
	SetWindowText(m_edit, m_currentText.c_str());

	if (!m_editMode)
	{
		m_editMode = true;

		PositionEditControl();
		ShowWindow(m_edit, SW_SHOW);

		InvalidateRect(GetHWND(), nullptr, false);
	}

	SetFocus(m_edit);

	if (selectAll)
	{
		SelectAllText();
	}
}

void AddressBarView::ExitEditMode()
{
	if (!m_editMode)
	{
		return;
	}

	m_editMode = false;

	ShowWindow(m_edit, SW_HIDE);
	InvalidateRect(GetHWND(), nullptr, false);
}

void AddressBarView::PositionEditControl()
{
	if (!m_editMode)
	{
		return;
	}

	RECT clientRect;
	auto res = GetClientRect(GetHWND(), &clientRect);
	CHECK(res);

	// The edit control is positioned within the 1 pixel border that's drawn around the control.
	InflateRect(&clientRect, -1, -1);

	SetWindowPos(m_edit, nullptr, clientRect.left, clientRect.top, GetRectWidth(&clientRect),
		GetRectHeight(&clientRect), SWP_NOZORDER);
}

void AddressBarView::ShowSegmentDropdown(size_t segmentIndex)
{
	if (!m_delegate || segmentIndex >= m_segments.size())
	{
		return;
	}

	auto items = EnumerateChildFolders(m_segments[segmentIndex].pidl);

	if (items.empty())
	{
		return;
	}

	wil::unique_hmenu menu(CreatePopupMenu());
	CHECK(menu);

	UINT id = 1;

	for (const auto &[pidl, name] : items)
	{
		std::wstring escapedName = EscapeNameForMenu(name);

		AppendMenu(menu.get(), MF_STRING, id++, escapedName.c_str());
	}

	m_openDropdownSegmentIndex = static_cast<int>(segmentIndex);
	InvalidateRect(GetHWND(), nullptr, false);

	POINT pt = { m_segments[segmentIndex].dropdownRect.left,
		m_segments[segmentIndex].dropdownRect.bottom };
	auto res = ClientToScreen(GetHWND(), &pt);
	CHECK(res);

	UINT command = TrackPopupMenuEx(menu.get(), TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x,
		pt.y, GetHWND(), nullptr);

	m_openDropdownSegmentIndex = -1;
	InvalidateRect(GetHWND(), nullptr, false);

	if (command != 0)
	{
		m_delegate->OnSegmentSelected(items[command - 1].first.Raw());
	}
}

void AddressBarView::ShowOverflowMenu()
{
	if (!m_delegate || !m_hasOverflow)
	{
		return;
	}

	wil::unique_hmenu menu(CreatePopupMenu());
	CHECK(menu);

	std::vector<size_t> hiddenSegmentIndices;
	UINT id = 1;

	for (size_t i = 1; i < m_segments.size(); i++)
	{
		if (!m_segments[i].hidden)
		{
			continue;
		}

		std::wstring escapedName = EscapeNameForMenu(m_segments[i].name);

		AppendMenu(menu.get(), MF_STRING, id++, escapedName.c_str());
		hiddenSegmentIndices.push_back(i);
	}

	if (hiddenSegmentIndices.empty())
	{
		return;
	}

	POINT pt = { m_overflowRect.left, m_overflowRect.bottom };
	auto res = ClientToScreen(GetHWND(), &pt);
	CHECK(res);

	UINT command = TrackPopupMenuEx(menu.get(), TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x,
		pt.y, GetHWND(), nullptr);

	if (command != 0)
	{
		m_delegate->OnSegmentSelected(m_segments[hiddenSegmentIndices[command - 1]].pidl.Raw());
	}
}

std::vector<std::pair<PidlAbsolute, std::wstring>> AddressBarView::EnumerateChildFolders(
	const PidlAbsolute &pidl) const
{
	std::vector<std::pair<PidlAbsolute, std::wstring>> items;

	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, pidl.Raw(), nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return items;
	}

	wil::com_ptr_nothrow<IEnumIDList> enumerator;
	hr = shellFolder->EnumObjects(GetHWND(), SHCONTF_FOLDERS, &enumerator);

	if (FAILED(hr) || !enumerator)
	{
		return items;
	}

	PITEMID_CHILD child = nullptr;

	while (enumerator->Next(1, &child, nullptr) == S_OK)
	{
		std::wstring name;
		hr = GetDisplayName(shellFolder.get(), child, SHGDN_NORMAL, name);

		if (SUCCEEDED(hr))
		{
			items.emplace_back(pidl + child, name);
		}

		CoTaskMemFree(child);
		child = nullptr;
	}

	return items;
}

void AddressBarView::Focus()
{
	if (!IsWindowVisible(GetHWND()))
	{
		return;
	}

	EnterEditMode(true);
}

std::wstring AddressBarView::GetText() const
{
	if (m_editMode)
	{
		return GetWindowString(m_edit);
	}

	return m_currentText;
}

bool AddressBarView::IsTextModified() const
{
	if (!m_editMode)
	{
		return false;
	}

	return SendMessage(m_edit, EM_GETMODIFY, 0, 0);
}

void AddressBarView::SelectAllText()
{
	SendMessage(m_edit, EM_SETSEL, 0, -1);
}

void AddressBarView::UpdateTextAndIcon(const std::optional<std::wstring> &optionalText,
	int iconIndex, const PidlAbsolute *optionalPidl)
{
	m_iconIndex = iconIndex;

	if (optionalText)
	{
		m_currentText = *optionalText;

		if (optionalPidl)
		{
			RebuildSegments(*optionalPidl);
		}

		// If the text was updated, it means the current directory has changed (e.g. a navigation
		// was committed, or another tab was selected). In that case, edit mode (if active) is
		// exited.
		ExitEditMode();
	}

	InvalidateRect(GetHWND(), nullptr, false);
}

void AddressBarView::RevertText()
{
	SetWindowText(m_edit, m_currentText.c_str());
}

void AddressBarView::ShowDropDown()
{
	if (!IsWindowVisible(GetHWND()) || m_segments.empty())
	{
		return;
	}

	// This shows the subfolders of the current directory, which matches the items that were
	// previously shown in the address bar dropdown.
	ShowSegmentDropdown(m_segments.size() - 1);
}

AddressBarViewDelegate *AddressBarView::GetDelegateForTesting()
{
	CHECK(IsInTest());
	return m_delegate;
}

void AddressBarView::SetTextForTesting(const std::wstring &text)
{
	CHECK(IsInTest());

	EnterEditMode(false);
	SetWindowText(m_edit, text.c_str());
	SendMessage(m_edit, EM_SETMODIFY, TRUE, 0);
}
