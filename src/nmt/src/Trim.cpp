#include "pch.h"

#include "Trim.h"

std::string_view Trim(std::string_view sv) {
    while (!sv.empty() && isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && isspace(sv.back())) {
        sv.remove_suffix(1);
    }
    return sv;
}
