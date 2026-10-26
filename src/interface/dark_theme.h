#ifndef FILEZILLA_INTERFACE_DARK_THEME_HEADER
#define FILEZILLA_INTERFACE_DARK_THEME_HEADER
#include <wx/colour.h>
class wxWindow;
namespace fztheme {
enum class Role {
	Background, Surface, Field, Border, Text, SecondaryText, DisabledText,
	Selection, Hover, Accent, Error, Command, Success, Trace, Listing,
	ComparisonDifferent, ComparisonMissing, ComparisonNewer, Progress
};
// 0=Dark (custom build default), 1=Light, 2=Follow System. Before UI creation.
void Initialize(int preference);
// Safe after normal exit or failed startup; does not change OS settings.
void Shutdown();
bool IsDark();
bool IsHighContrast();
wxColour Colour(Role role);
void Apply(wxWindow* window);
}
#endif
