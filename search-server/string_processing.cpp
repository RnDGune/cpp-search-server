#include "string_processing.h"


std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.size(), str.find_first_not_of(" ")));
    const size_t pos_end = str.npos;
    while (!str.empty()) {
        size_t space_pos = str.find(' ');
        result.push_back(space_pos == pos_end ? str.substr(0, pos_end) : str.substr(0, space_pos));
        str.remove_prefix(std::min(str.size(), space_pos));
        str.remove_prefix(std::min(str.size(), str.find_first_not_of(" ")));
    }
    return result;
}

