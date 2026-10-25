#include "penteboard.h"

#include <utility>

namespace cj::pente
{
    bool is_in_bounds(coord position)
    {
        return position.x < GridSize && position.y < GridSize;
    }
    
    bool is_in_bounds(signed_coord position)
    {
        return position.x >= 0 && position.y >= 0
            && std::cmp_less(position.x, GridSize)
            && std::cmp_less(position.y, GridSize);
    }
}
