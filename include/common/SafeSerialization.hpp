#ifndef BANKING_SYSTEM_COMMON_SAFE_SERIALIZATION_HPP
#define BANKING_SYSTEM_COMMON_SAFE_SERIALIZATION_HPP

#include <string>
#include <vector>

namespace BankingSystem::RecordFormat {

inline std::string escape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '|': out += "\\p"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

inline bool split(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    std::string field;
    bool escaped = false;
    for (char c : line) {
        if (escaped) {
            switch (c) {
                case 'p': field.push_back('|'); break;
                case 'n': field.push_back('\n'); break;
                case 'r': field.push_back('\r'); break;
                default: field.push_back(c); break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '|') {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(c);
        }
    }
    if (escaped) return false;
    fields.push_back(field);
    return true;
}

} // namespace BankingSystem::RecordFormat

#endif
