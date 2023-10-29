#pragma once

std::expected<std::optional<std::pair<std::string_view, std::vector<std::string>>>, std::string>
TryEatSpecialComment(std::string_view sv);
