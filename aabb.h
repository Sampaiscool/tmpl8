#pragma once

namespace Tmpl8
{
    /*
    An Axis Aligned Bounding Box: a rectangle that is never rotated. Because
    the sides always line up with the axes, an overlap test is four
    comparisons instead of real maths, cheap enough to run for every collider
    every frame.

    x/y/w/h in world pixels, which is exactly how Tiled stores its object
    layer, so colliders copy across 1 to 1.

    (Tmpl8's own aabb in tmpl8math.h is 3D and SSE based, so it is no use here.)
    */
    struct AABB
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        // y grows DOWNWARDS on a screen, so Top() is the smaller number.
        float Left()   const { return x; }
        float Right()  const { return x + w; }
        float Top()    const { return y; }
        float Bottom() const { return y + h; }
    };

    /*
    Proves the boxes are NOT separated, which is easier than proving they
    overlap: if neither is fully left, right, above or below the other, they
    have nowhere left to be except on top of eachother.

    Touching edges deliberately count as NOT overlapping. Pushing the player
    out of a wall leaves him exactly against it, and if that counted as a hit
    he would read as stuck inside the wall and jitter.
    */
    inline bool Overlaps(const AABB& a, const AABB& b)
    {
        if (a.Right()  <= b.Left())   return false;
        if (a.Left()   >= b.Right())  return false;
        if (a.Bottom() <= b.Top())    return false;
        if (a.Top()    >= b.Bottom()) return false;
        return true;
    }

    /*
    A piece of level geometry: the rectangle plus how solid it is. The flag
    lives here and not in AABB because an AABB is pure geometry and gets used
    for things that have no such rule, like the player's own body.

    oneWay is Tiled's 'platform' boolean: it only stops you from above, so you
    can jump up through it and land on top.
    */
    struct Collider
    {
        AABB box;
        bool oneWay = false;
    };
}
