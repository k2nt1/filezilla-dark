#include "filezilla.h"
#include "dark_theme.h"
#include <wx/combobox.h>
#include <wx/eventfilter.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/statbox.h>
#include <wx/toolbar.h>
#include <memory>
#include <unordered_map>
#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#if wxCHECK_VERSION(3, 3, 4)
#include <wx/msw/darkmode.h>
#endif
#endif

namespace fztheme {
namespace {
wxColour DarkColour(Role role)
{
	switch (role) {
	case Role::Background: return wxColour(0x18, 0x18, 0x18);
	case Role::Surface: return wxColour(0x20, 0x20, 0x20);
	case Role::Field: return wxColour(0x26, 0x26, 0x26);
	case Role::Border: return wxColour(0x38, 0x38, 0x38);
	case Role::Text: return wxColour(0xe8, 0xe8, 0xe8);
	case Role::SecondaryText:
	case Role::DisabledText: return wxColour(0xaa, 0xaa, 0xaa);
	case Role::Selection: return wxColour(0x28, 0x4f, 0x78);
	case Role::Hover: return wxColour(0x30, 0x30, 0x30);
	case Role::Accent:
	case Role::Command: return wxColour(0x37, 0x94, 0xff);
	case Role::Error: return wxColour(0xff, 0x85, 0x85);
	case Role::Success: return wxColour(0x86, 0xd3, 0x9b);
	case Role::Progress: return wxColour(0x30, 0x5e, 0x40);
	case Role::Trace: return wxColour(0xbf, 0xa3, 0xe8);
	case Role::Listing: return wxColour(0xd7, 0xb9, 0x86);
	case Role::ComparisonDifferent: return wxColour(0x58, 0x2c, 0x32);
	case Role::ComparisonMissing: return wxColour(0x50, 0x43, 0x21);
	case Role::ComparisonNewer: return wxColour(0x26, 0x48, 0x32);
	}
	return wxColour(0xe8, 0xe8, 0xe8);
}
#if defined(__WXMSW__) && wxCHECK_VERSION(3, 3, 4)
class NativePalette final : public wxDarkModeSettings
{
public:
	wxColour GetColour(wxSystemColour index) override
	{
		// Returning no override makes wxMSW fall through to GetSysColor().
		// ForceDark can remain active while Windows High Contrast is enabled.
		if (IsHighContrast()) { return wxColour(); }
		switch (index) {
		case wxSYS_COLOUR_BACKGROUND:
		case wxSYS_COLOUR_APPWORKSPACE:
		case wxSYS_COLOUR_SCROLLBAR: return DarkColour(Role::Background);
		case wxSYS_COLOUR_WINDOW: return DarkColour(Role::Field);
		case wxSYS_COLOUR_BTNFACE:
		case wxSYS_COLOUR_LISTBOX: return DarkColour(Role::Surface);
		case wxSYS_COLOUR_WINDOWTEXT:
		case wxSYS_COLOUR_BTNTEXT:
		case wxSYS_COLOUR_LISTBOXTEXT:
		case wxSYS_COLOUR_CAPTIONTEXT:
		case wxSYS_COLOUR_INFOTEXT: return DarkColour(Role::Text);
		case wxSYS_COLOUR_GRAYTEXT:
		case wxSYS_COLOUR_INACTIVECAPTIONTEXT: return DarkColour(Role::DisabledText);
		case wxSYS_COLOUR_HIGHLIGHT: return DarkColour(Role::Selection);
		case wxSYS_COLOUR_HOTLIGHT: return DarkColour(Role::Accent);
		case wxSYS_COLOUR_HIGHLIGHTTEXT:
		case wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT: return DarkColour(Role::Text);
		case wxSYS_COLOUR_INFOBK: return DarkColour(Role::Surface);
		case wxSYS_COLOUR_WINDOWFRAME:
		case wxSYS_COLOUR_BTNSHADOW:
		case wxSYS_COLOUR_3DDKSHADOW:
		case wxSYS_COLOUR_ACTIVEBORDER:
		case wxSYS_COLOUR_INACTIVEBORDER: return DarkColour(Role::Border);
		case wxSYS_COLOUR_BTNHIGHLIGHT:
		case wxSYS_COLOUR_3DLIGHT: return DarkColour(Role::Hover);
		case wxSYS_COLOUR_ACTIVECAPTION:
		case wxSYS_COLOUR_INACTIVECAPTION:
		case wxSYS_COLOUR_GRADIENTACTIVECAPTION:
		case wxSYS_COLOUR_GRADIENTINACTIVECAPTION: return DarkColour(Role::Background);
		case wxSYS_COLOUR_MENU:
		case wxSYS_COLOUR_MENUBAR: return DarkColour(Role::Surface);
		case wxSYS_COLOUR_MENUTEXT: return DarkColour(Role::Text);
		case wxSYS_COLOUR_MENUHILIGHT: return DarkColour(Role::Hover);
		default: return wxDarkModeSettings::GetColour(index);
		}
	}
	wxColour GetMenuColour(wxMenuColour which) override
	{
		if (IsHighContrast()) {
			switch (which) {
			case wxMenuColour::StandardFg: return wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
			case wxMenuColour::StandardBg: return wxSystemSettings::GetColour(wxSYS_COLOUR_MENU);
			case wxMenuColour::DisabledFg: return wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
			case wxMenuColour::HotBg: return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
			}
		}
		switch (which) {
		case wxMenuColour::StandardFg: return DarkColour(Role::Text);
		case wxMenuColour::StandardBg: return DarkColour(Role::Surface);
		case wxMenuColour::DisabledFg: return DarkColour(Role::DisabledText);
		case wxMenuColour::HotBg: return DarkColour(Role::Hover);
		}
		return DarkColour(Role::Surface);
	}
	wxPen GetBorderPen() override
	{
		return wxPen(IsHighContrast() ? wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT) : DarkColour(Role::Border));
	}
};
#endif

struct OwnedColours
{
	bool background{};
	bool foreground{};
	wxColour backgroundValue;
	wxColour foregroundValue;
};
std::unordered_map<wxWindow*, OwnedColours> ownedColours;
bool filterInstalled{};
bool refreshPending{};
unsigned int paletteGeneration{};
int startupPreference{};
bool highContrastActive{};

void UpdateNativeAppearance()
{
#if defined(__WXMSW__) && wxCHECK_VERSION(3, 3, 4)
	auto mode = wxApp::DarkMode_Always;
	if (IsHighContrast() || startupPreference == 1) { mode = wxApp::DarkMode_Never; }
	else if (startupPreference == 2) { mode = wxApp::DarkMode_Auto; }
	auto settings = std::make_unique<NativePalette>();
	// wx3.3.4 takes ownership after successful initialization. On failure
	// it returns before adopting the settings pointer.
	if (wxTheApp->MSWEnableDarkMode(mode, settings.get())) { settings.release(); }
	else { wxLogWarning("Windows appearance initialization failed; some controls may retain their default colours."); }
#elif wxCHECK_VERSION(3, 3, 0)
	auto mode = IsHighContrast() || startupPreference == 1 ? wxApp::Appearance::Light
		: startupPreference == 0 ? wxApp::Appearance::Dark : wxApp::Appearance::System;
	if (wxTheApp->SetAppearance(mode) != wxApp::AppearanceResult::Ok) { wxLogWarning("Requested application appearance could not be enabled."); }
#endif
}

void ScheduleApply()
{
	if (!filterInstalled || refreshPending || !wxTheApp) { return; }
	refreshPending = true;
	auto const generation = paletteGeneration;
	wxTheApp->CallAfter([generation]() {
		// OnExit and failed startup can leave queued app callbacks. A new
		// Initialize() must not make callbacks from an older lifecycle valid.
		if (!filterInstalled || generation != paletteGeneration) { return; }
		refreshPending = false;
		bool const highContrast = IsHighContrast();
		if (highContrast != highContrastActive) {
			highContrastActive = highContrast;
			// Suspend native dark owner drawing for accessibility themes and
			// restore the selected startup mode when High Contrast is closed.
			UpdateNativeAppearance();
		}
		for (auto window : wxTopLevelWindows) {
			if (!window->IsBeingDeleted()) { Apply(window); }
		}
	});
}

// Native procedures, input and accessibility remain owned by wxWidgets. Only
// public colour APIs are called, on this application's windows exclusively.
class WindowPaletteFilter final : public wxEventFilter
{
public:
	int FilterEvent(wxEvent& event) override
	{
		if (event.GetEventType() == wxEVT_DESTROY) {
			ownedColours.erase(static_cast<wxWindowDestroyEvent&>(event).GetWindow());
		}
		else if (event.GetEventType() == wxEVT_CREATE ||
			(event.GetEventType() == wxEVT_SHOW && static_cast<wxShowEvent const&>(event).IsShown()) ||
			event.GetEventType() == wxEVT_SYS_COLOUR_CHANGED) {
			// wxMSW updates native colours and propagates the system event to
			// children first. Apply or remove our defaults after that finishes.
			ScheduleApply();
		}
		return Event_Skip;
	}
};
WindowPaletteFilter paletteFilter;
}

bool IsHighContrast()
{
#ifdef __WXMSW__
	HIGHCONTRASTW state{};
	state.cbSize = sizeof(state);
	return SystemParametersInfoW(SPI_GETHIGHCONTRAST, state.cbSize, &state, 0) && (state.dwFlags & HCF_HIGHCONTRASTON);
#else
	return false;
#endif
}
bool IsDark() { return !IsHighContrast() && wxSystemSettings::GetAppearance().IsDark(); }

wxColour Colour(Role role)
{
	if (IsDark()) { return DarkColour(role); }
	// Never override an existing accessibility palette with custom colours.
	if (IsHighContrast()) {
		switch (role) {
		case Role::Background: case Role::Surface: case Role::Field:
		case Role::ComparisonDifferent: case Role::ComparisonMissing: case Role::ComparisonNewer:
			return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		case Role::Selection: case Role::Progress: return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
		default: return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		}
	}
	switch (role) {
	case Role::Background: case Role::Surface: case Role::Hover: return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	case Role::Field: return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	case Role::Border: return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
	case Role::Text: return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	case Role::SecondaryText: case Role::DisabledText: return wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
	case Role::Selection: return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	case Role::Accent: case Role::Command: return wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT);
	case Role::Error: return wxColour(0xb0, 0, 0);
	case Role::Success: return wxColour(0, 0x70, 0x28);
	case Role::Progress: return wxColour(0x80, 0xbf, 0x91);
	case Role::Trace: case Role::Listing: return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	case Role::ComparisonDifferent: return wxColour(255, 128, 128);
	case Role::ComparisonMissing: return wxColour(255, 255, 128);
	case Role::ComparisonNewer: return wxColour(128, 255, 128);
	}
	return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
}

void Apply(wxWindow* window)
{
	if (!window || window->IsBeingDeleted()) { return; }
	bool changed = false;
	auto previous = ownedColours.find(window);
	if (!IsDark()) {
		if (previous != ownedColours.end()) {
			// Only undo a colour if it is still the value we applied. A later
			// site tint or other application override belongs to its caller.
			auto const& owned = previous->second;
			if (owned.background && window->GetBackgroundColour() == owned.backgroundValue) {
				changed |= window->SetBackgroundColour(wxColour());
			}
			if (owned.foreground && window->GetForegroundColour() == owned.foregroundValue) {
				changed |= window->SetForegroundColour(wxColour());
			}
			ownedColours.erase(previous);
		}
		if (changed) { window->Refresh(); }
		for (auto child : window->GetChildren()) { Apply(child); }
		return;
	}

	// Modify only native defaults. In particular, explicit site tints, rich
	// message formatting and certificate warning colours remain caller-owned.
	// SetOwn* avoids passing an explicit dark default to children created later.
	auto background = [window, &changed](Role role) {
		auto existing = ownedColours.find(window);
		bool const ours = existing != ownedColours.end() && existing->second.background &&
			window->GetBackgroundColour() == existing->second.backgroundValue;
		if (existing != ownedColours.end() && !ours) { existing->second.background = false; }
		if (!ours && window->UseBackgroundColour()) { return; }
		auto const colour = Colour(role);
		if (window->GetBackgroundColour() == colour) { return; }
		window->SetOwnBackgroundColour(colour);
		auto& owned = ownedColours[window];
		owned.background = true;
		owned.backgroundValue = colour;
		changed = true;
	};
	auto foreground = [window, &changed]() {
		auto existing = ownedColours.find(window);
		bool const ours = existing != ownedColours.end() && existing->second.foreground &&
			window->GetForegroundColour() == existing->second.foregroundValue;
		if (existing != ownedColours.end() && !ours) { existing->second.foreground = false; }
		if (!ours && window->UseForegroundColour()) { return; }
		auto const colour = Colour(Role::Text);
		if (window->GetForegroundColour() == colour) { return; }
		window->SetOwnForegroundColour(colour);
		auto& owned = ownedColours[window];
		owned.foreground = true;
		owned.foregroundValue = colour;
		changed = true;
	};
	if (dynamic_cast<wxTopLevelWindow*>(window)) {
		background(Role::Background);
		foreground();
	}
	else if (dynamic_cast<wxTextCtrl*>(window) || dynamic_cast<wxComboBox*>(window)) {
		background(Role::Field);
		// The rich message log retains per-line semantic text attributes.
		if (!(window->GetWindowStyle() & wxTE_RICH2)) { foreground(); }
	}
	else if (dynamic_cast<wxListCtrl*>(window) || dynamic_cast<wxTreeCtrl*>(window)) {
		background(Role::Surface);
		foreground();
	}
	else if (dynamic_cast<wxToolBar*>(window) || dynamic_cast<wxPanel*>(window) || dynamic_cast<wxStatusBar*>(window)) {
		background(Role::Surface);
		foreground();
	}
	auto const owned = ownedColours.find(window);
	if (owned != ownedColours.end() && !owned->second.background && !owned->second.foreground) { ownedColours.erase(owned); }
	if (changed) { window->Refresh(); }
	for (auto child : window->GetChildren()) { Apply(child); }
}

void Initialize(int preference)
{
	startupPreference = preference;
	highContrastActive = IsHighContrast();
	// wx owns the platform integration; no OS preferences are set.
	UpdateNativeAppearance();
	if (!filterInstalled) { wxEvtHandler::AddFilter(&paletteFilter); filterInstalled = true; }
	ScheduleApply();
}

void Shutdown()
{
	if (filterInstalled) {
		wxEvtHandler::RemoveFilter(&paletteFilter);
		filterInstalled = false;
	}
	++paletteGeneration;
	refreshPending = false;
	ownedColours.clear();
}
}
