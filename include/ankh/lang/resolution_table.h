#pragma once

#include <unordered_map>

#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>

namespace ankh::lang {

struct ResolutionTable
{
    std::unordered_map<const Expression*, size_t> expr_hops;
    std::unordered_map<const Statement*, size_t> stmt_hops;
};

}