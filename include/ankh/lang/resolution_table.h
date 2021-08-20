#pragma once

#include <cstddef>

#include <unordered_map>
namespace ankh::lang {

struct ResolutionTable
{
    std::unordered_map<const void*, size_t> hops;
};

}