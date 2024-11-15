#pragma once

#include <functional>

#include "new_entity_id.h"

template<>
struct std::hash<ne::EntityId>
{
    std::size_t operator()(const ne::EntityId& id) const noexcept
    {
        std::size_t h = std::hash<int>{}(id._id);
        return h;        
    }
};