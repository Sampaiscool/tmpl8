#pragma once

namespace Tmpl8
{
    struct AABB
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        float Left()   const { return x; }
        float Right()  const { return x + w; }
        float Top()    const { return y; }
        float Bottom() const { return y + h; }
    };

    // Touching edges doesnt count as overlapping.
    inline bool Overlaps(const AABB& a, const AABB& b)
    {
        if (a.Right()  <= b.Left())   return false;
        if (a.Left()   >= b.Right())  return false;
        if (a.Bottom() <= b.Top())    return false;
        if (a.Top()    >= b.Bottom()) return false;
        return true;
    }

    // oneWay is Tiled's platform flag.
    struct Collider
    {
        AABB box;
        bool oneWay = false;
    };
}
