#pragma once

#include "Types.hpp"
#include <string>

namespace korektabar {

// Szybki formater tekstu polskiego (operuje liniowo O(N) bez regexów dla N3350).
std::wstring FormatPolishText(const std::wstring& input, const FormatterOptions& opts);

} // namespace korektabar
