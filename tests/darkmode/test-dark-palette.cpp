#include <wx/wx.h>
#include <cmath>
#include <iostream>
#include "dark_theme.h"
#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

class PaletteTestApp : public wxApp {
public:
	bool OnInit() override { return true; }
};
wxIMPLEMENT_APP_NO_MAIN(PaletteTestApp);

namespace {
int failures{};
int checks{};
void Check(bool condition, char const* message)
{
	++checks;
	std::cout << (condition ? "PASS " : "FAIL ") << message << '\n';
	if (!condition) { ++failures; }
}
double Luminance(wxColour const& colour)
{
	auto channel = [](unsigned char value) {
		double const s = value / 255.0;
		return s <= 0.04045 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
	};
	return 0.2126 * channel(colour.Red()) + 0.7152 * channel(colour.Green()) + 0.0722 * channel(colour.Blue());
}
double Contrast(wxColour const& a, wxColour const& b)
{
	double const first = Luminance(a);
	double const second = Luminance(b);
	return (std::max(first, second) + 0.05) / (std::min(first, second) + 0.05);
}
void Pump()
{
	wxTheApp->ProcessPendingEvents();
}
}

int main(int argc, char** argv)
{
	std::cout << std::unitbuf;
	wxLog::SetActiveTarget(new wxLogStderr());
	if (!wxEntryStart(argc, argv)) { return 2; }
	if (!wxTheApp->CallOnInit()) { wxEntryCleanup(); return 2; }
	fztheme::Initialize(0);
	bool const highContrast = fztheme::IsHighContrast();
	Check(fztheme::IsDark() == !highContrast, "forced app dark respects High Contrast");

	auto* frame = new wxFrame(nullptr, wxID_ANY, "Palette integration test");
	wxTheApp->SetTopWindow(frame);
	auto* defaults = new wxFrame(nullptr, wxID_ANY, "Default restoration test");
	auto* custom = new wxPanel(frame);
	wxColour const customBackground(0x13, 0x29, 0x3f);
	wxColour const customForeground(0xd9, 0xc3, 0xa1);
	custom->SetBackgroundColour(customBackground);
	custom->SetForegroundColour(customForeground);
	auto* warning = new wxStaticText(frame, wxID_ANY, "Certificate warning");
	auto const error = fztheme::Colour(fztheme::Role::Error);
	warning->SetForegroundColour(error);
	fztheme::Apply(frame);
	fztheme::Apply(defaults);
	Pump();
	Check(custom->GetBackgroundColour() == customBackground, "explicit site background survives dark apply");
	Check(custom->GetForegroundColour() == customForeground, "explicit foreground survives dark apply");
	Check(warning->GetForegroundColour() == error, "semantic warning foreground survives dark apply");
	if (!highContrast) {
		Check(defaults->GetBackgroundColour() == fztheme::Colour(fztheme::Role::Background), "default frame uses the dark background role");
		auto* laterChild = new wxPanel(defaults);
		Check(!laterChild->UseForegroundColour(), "new children do not inherit an explicit dark foreground");
		for (auto role : {fztheme::Role::Text, fztheme::Role::Error, fztheme::Role::Command,
			fztheme::Role::Success, fztheme::Role::Trace, fztheme::Role::Listing}) {
			Check(Contrast(fztheme::Colour(role), fztheme::Colour(fztheme::Role::Field)) >= 4.5,
				"dark semantic log text contrast >= 4.5:1");
		}
		for (auto role : {fztheme::Role::ComparisonDifferent, fztheme::Role::ComparisonMissing,
			fztheme::Role::ComparisonNewer, fztheme::Role::Progress}) {
			Check(Contrast(fztheme::Colour(fztheme::Role::Text), fztheme::Colour(role)) >= 4.5,
				"dark comparison/progress text contrast >= 4.5:1");
		}
	}

	// A caller may change a previously default-coloured window after Apply().
	// Leaving dark mode must retain that later caller-owned value.
	frame->SetBackgroundColour(customBackground);
	fztheme::Initialize(1);
	Pump();
	Check(!fztheme::IsDark(), "app Light mode is effective without changing Windows preferences");
	Check(frame->GetBackgroundColour() == customBackground, "later caller override survives leaving dark mode");
	Check(!defaults->UseBackgroundColour(), "leaving dark removes only helper-owned background overrides");
	Check(defaults->GetBackgroundColour() == static_cast<wxWindow*>(defaults)->GetDefaultAttributes().colBg,
		"restored background follows the native light default");
	Check(custom->GetForegroundColour() == customForeground, "explicit foreground survives light transition");
	if (!highContrast) {
		Check(Contrast(fztheme::Colour(fztheme::Role::Text), fztheme::Colour(fztheme::Role::Progress)) >= 4.5,
			"light progress text contrast >= 4.5:1");
	}

	fztheme::Initialize(0);
	Pump();
	if (!highContrast) {
		Check(defaults->GetBackgroundColour() == fztheme::Colour(fztheme::Role::Background),
			"returning to dark reapplies owned defaults");
	}
	fztheme::Initialize(2);
	Pump();
#ifdef __WXMSW__
	DWORD light = 1;
	DWORD length = sizeof(light);
	LSTATUS const read = RegGetValueW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &length);
	if (read == ERROR_SUCCESS) {
		Check(fztheme::IsDark() == (!highContrast && light == 0), "Follow System matches the read-only Windows app preference");
	}
#endif

	// Exercise callback invalidation, filter removal and repeated Shutdown().
	fztheme::Initialize(0);
	fztheme::Shutdown();
	fztheme::Shutdown();
	fztheme::Initialize(1);
	Pump();
	Check(!fztheme::IsDark(), "shutdown/reinitialize ignores callbacks from the previous generation");
	wxTheApp->SetTopWindow(nullptr);
	delete frame;
	delete defaults;
	Pump();
	fztheme::Shutdown();
	wxTheApp->OnExit();
	wxEntryCleanup();
	std::cout << "CHECKS=" << checks << " FAILURES=" << failures << '\n';
	return failures ? 1 : 0;
}
