#include "common/TableFormatter.hpp"

namespace BankingSystem {

void TableFormatter::addColumn(const std::string& title, size_t width, Alignment alignment) {
    m_columns.emplace_back(title, width, alignment);
}

void TableFormatter::addRow(const std::vector<std::string>& rowData) {
    m_rows.push_back(rowData);
}

void TableFormatter::clear() {
    m_columns.clear();
    m_rows.clear();
}

std::string TableFormatter::alignText(const std::string& text, size_t width, Alignment align) const {
    if (text.length() >= width) {
        return text.substr(0, width);
    }
    size_t extra = width - text.length();
    if (align == Alignment::LEFT) {
        return text + std::string(extra, ' ');
    } else if (align == Alignment::RIGHT) {
        return std::string(extra, ' ') + text;
    } else { // CENTER
        size_t left = extra / 2;
        size_t right = extra - left;
        return std::string(left, ' ') + text + std::string(right, ' ');
    }
}

std::string TableFormatter::buildHorizontalBorder(const std::vector<size_t>& colWidths) const {
    std::stringstream ss;
    ss << "+";
    for (size_t w : colWidths) {
        ss << std::string(w + 2, '-') << "+";
    }
    ss << "\n";
    return ss.str();
}

std::string TableFormatter::render() const {
    if (m_columns.empty()) return "";

    std::vector<size_t> colWidths;
    colWidths.reserve(m_columns.size());

    // Compute column widths
    for (size_t c = 0; c < m_columns.size(); ++c) {
        size_t maxLen = m_columns[c].title.length();
        if (m_autoWidth || m_columns[c].width == 0) {
            for (const auto& row : m_rows) {
                if (c < row.size()) {
                    maxLen = std::max(maxLen, row[c].length());
                }
            }
            if (m_columns[c].width > 0) {
                maxLen = std::max(maxLen, m_columns[c].width);
            }
        } else {
            maxLen = m_columns[c].width;
        }
        colWidths.push_back(maxLen);
    }

    std::stringstream ss;

    // Calculate total inner width
    size_t totalWidth = 0;
    for (size_t w : colWidths) {
        totalWidth += w + 3; // 1 border/space + w + 2 padding spaces
    }
    if (totalWidth > 0) totalWidth -= 1; // last separator offset

    // Render Table Title
    if (!m_title.empty()) {
        ss << "+" << std::string(totalWidth, '-') << "+\n";
        ss << "| " << alignText(m_title, totalWidth - 2, Alignment::CENTER) << " |\n";
    }

    // Top Border
    ss << buildHorizontalBorder(colWidths);

    // Header Row
    ss << "|";
    for (size_t c = 0; c < m_columns.size(); ++c) {
        ss << " " << alignText(m_columns[c].title, colWidths[c], m_columns[c].alignment) << " |";
    }
    ss << "\n";

    // Header Separator
    ss << buildHorizontalBorder(colWidths);

    // Data Rows
    for (const auto& row : m_rows) {
        ss << "|";
        for (size_t c = 0; c < m_columns.size(); ++c) {
            std::string cellText = (c < row.size()) ? row[c] : "";
            ss << " " << alignText(cellText, colWidths[c], m_columns[c].alignment) << " |";
        }
        ss << "\n";
    }

    // Bottom Border
    ss << buildHorizontalBorder(colWidths);

    return ss.str();
}

void TableFormatter::print(std::ostream& os) const {
    os << render();
}

} // namespace BankingSystem
