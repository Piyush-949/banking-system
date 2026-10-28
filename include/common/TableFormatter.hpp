#ifndef BANKING_SYSTEM_COMMON_TABLE_FORMATTER_HPP
#define BANKING_SYSTEM_COMMON_TABLE_FORMATTER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace BankingSystem {

class TableFormatter {
public:
    enum class Alignment {
        LEFT,
        CENTER,
        RIGHT
    };

    struct Column {
        std::string title;
        size_t width;
        Alignment alignment;
        
        Column(std::string t, size_t w = 0, Alignment align = Alignment::LEFT)
            : title(std::move(t)), width(w), alignment(align) {}
    };

private:
    std::string m_title;
    std::vector<Column> m_columns;
    std::vector<std::vector<std::string>> m_rows;
    bool m_showBorders;
    bool m_autoWidth;

public:
    explicit TableFormatter(std::string tableTitle = "", bool showBorders = true)
        : m_title(std::move(tableTitle)), m_showBorders(showBorders), m_autoWidth(true) {}

    // Add column metadata
    void addColumn(const std::string& title, size_t width = 0, Alignment alignment = Alignment::LEFT);

    // Add row data
    void addRow(const std::vector<std::string>& rowData);

    // Render table to string or output stream
    std::string render() const;
    void print(std::ostream& os = std::cout) const;

    // Reset table data
    void clear();

private:
    std::string alignText(const std::string& text, size_t width, Alignment align) const;
    std::string buildHorizontalBorder(const std::vector<size_t>& colWidths) const;
};

} // namespace BankingSystem

#endif // BANKING_SYSTEM_COMMON_TABLE_FORMATTER_HPP
