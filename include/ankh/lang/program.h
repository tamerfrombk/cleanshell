#pragma once

#include <vector>
#include <string>

#include <ankh/lang/statement.h>

#include <ankh/lang/resolution_table.h>

namespace ankh::lang {

struct Program
{
    std::vector<StatementPtr> statements;
    std::vector<std::string> errors;
    ResolutionTable table;

    void add_statement(StatementPtr stmt) noexcept
    {
        statements.push_back(std::move(stmt));
    }

    void add_error(std::string&& error) noexcept
    {
        errors.push_back(std::forward<std::string>(error));
    }

    bool has_errors() const noexcept
    {
        return errors.size() > 0;
    }

    std::size_t size() const noexcept
    {
        return statements.size();
    }

    const StatementPtr& operator[](size_t i) const noexcept
    {
        return statements[i];
    }

};

}