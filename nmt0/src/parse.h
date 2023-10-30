#pragma once

std::string_view EatBlank(std::string_view sv);
std::string_view EatSpace(std::string_view sv);
std::string_view EatWhileNot(std::string_view sv, std::string_view chs);
std::optional<std::pair<std::string_view, std::string_view>> TryEatCSymbol(std::string_view sv);
std::optional<std::string_view> TryEatPrefix(std::string_view sv, std::string_view prefix);