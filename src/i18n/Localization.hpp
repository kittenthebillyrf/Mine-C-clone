#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mc::i18n {
struct Language {
    std::string_view code;
    std::string_view native_name;
};

const std::vector<Language>& languages();
bool is_supported(std::string_view code);
std::string text(std::string_view key, std::string_view language);
}
