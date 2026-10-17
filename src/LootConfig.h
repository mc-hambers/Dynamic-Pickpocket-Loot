#pragma once
#include "Core.h"
#include <charconv>
#include <cctype>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

namespace PL {
struct LootConfigEntry {
    std::string plugin;
    std::uint32_t localID;
    Category category;
    Wealth minimum;
    bool childSafe;
    bool prisonerSafe;
    std::size_t line;
};
struct LootConfigIssue { std::size_t line; std::string message; };
struct LootConfig {
    std::vector<LootConfigEntry> entries;
    std::vector<LootConfigIssue> issues;
};
inline void upsertLootItem(std::vector<Item>& pool,const Item& value) {
    const auto found=std::find_if(pool.begin(),pool.end(),[&](const Item& item) { return item.form==value.form; });
    if (found==pool.end()) pool.push_back(value); else *found=value;
}
inline std::string_view configTrim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}
inline std::string configLower(std::string_view text) {
    std::string out(text);
    for (auto& c:out) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
inline LootConfig parseLootConfig(std::istream& input) {
    LootConfig result;
    std::optional<Category> section;
    std::string line;
    std::size_t number=0;
    while (std::getline(input,line)) {
        ++number;
        if (number==1 && line.starts_with("\xEF\xBB\xBF")) line.erase(0,3);
        auto text=configTrim(line);
        if (const auto comment=text.find_first_of(";#");comment!=text.npos) text=configTrim(text.substr(0,comment));
        if (text.empty()) continue;
        auto issue=[&](const char* message) { result.issues.push_back({number,message}); };
        if (text.front()=='[') {
            section.reset();
            if (text.back()!=']') {issue("Malformed category section");continue;}
            const auto name=configLower(configTrim(text.substr(1,text.size()-2)));
            constexpr std::array names{"food","jewelry","gem","potion","ingredient","magic","lockpick"};
            for (std::size_t i=0;i<names.size();++i) if (name==names[i]) section=static_cast<Category>(i);
            if (!section) issue("Unknown category section");
            continue;
        }
        if (!section) {issue("Item needs a valid category section");continue;}
        const auto equal=text.find('=');
        if (equal==text.npos) {issue("Expected Plugin.ext|LocalFormID = Wealth, ChildSafe, PrisonerSafe");continue;}
        const auto key=configTrim(text.substr(0,equal));
        const auto bar=key.find('|');
        if (bar==key.npos || key.find('|',bar+1)!=key.npos) {issue("Expected one plugin/local-ID separator '|'");continue;}
        const auto plugin=configTrim(key.substr(0,bar));
        const auto lower=configLower(plugin);
        if (plugin.empty() || plugin.size()>255 || plugin.find_first_of("/\\:*?\"<>")!=plugin.npos ||
            !(lower.ends_with(".esm") || lower.ends_with(".esp") || lower.ends_with(".esl"))) {
            issue("Expected an ESM, ESP, or ESL filename without a path");continue;
        }
        auto idText=configTrim(key.substr(bar+1));
        if (idText.starts_with("0x") || idText.starts_with("0X")) idText.remove_prefix(2);
        std::uint32_t id=0;
        const auto [end,error]=std::from_chars(idText.data(),idText.data()+idText.size(),id,16);
        if (idText.empty() || idText.size()>8 || error!=std::errc{} || end!=idText.data()+idText.size() || id==0 || id>0xFFFFFF) {
            issue("Use a nonzero local hexadecimal Form ID, not a load-order Form ID");continue;
        }
        std::array<std::string,3> values;
        auto rest=text.substr(equal+1);
        bool valid=true;
        for (std::size_t i=0;i<3;++i) {
            const auto comma=rest.find(',');
            if ((i<2 && comma==rest.npos) || (i==2 && comma!=rest.npos)) {valid=false;break;}
            values[i]=configLower(configTrim(rest.substr(0,comma)));
            if (i<2) rest.remove_prefix(comma+1);
        }
        if (!valid) {issue("Expected exactly Wealth, ChildSafe, PrisonerSafe");continue;}
        std::optional<Wealth> wealth;
        constexpr std::array wealthNames{"destitute","poor","common","comfortable","wealthy","noble"};
        for (std::size_t i=0;i<wealthNames.size();++i) if (values[0]==wealthNames[i]) wealth=static_cast<Wealth>(i);
        if (!wealth) {issue("Unknown minimum wealth");continue;}
        auto boolean=[](const std::string& value)->std::optional<bool> {
            if (value=="true" || value=="1") return true;
            if (value=="false" || value=="0") return false;
            return std::nullopt;
        };
        const auto child=boolean(values[1]),prisoner=boolean(values[2]);
        if (!child || !prisoner) {issue("Safety flags must be true/false or 1/0");continue;}
        result.entries.push_back({std::string(plugin),id,*section,*wealth,*child,*prisoner,number});
    }
    return result;
}
}
