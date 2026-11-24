#pragma once

#include "Types.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace korektabar {

// Wyświetla modalne okno opcji w trybie Dark Mode.
// Zwraca true jeśli użytkownik zatwierdził zmiany.
bool ShowSettingsDialog(HWND parentHwnd, AppConfig& inOutConfig);

} // namespace korektabar
