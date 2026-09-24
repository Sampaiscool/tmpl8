#pragma once

namespace Tmpl8
{
    /*
    This code:
    An AABB, short for Axis Aligned Bounding Box. It is just a rectangle: a
    corner (x,y) plus a width and a height. "Axis aligned" means it is never
    rotated, its sides always line up with the x and y axis.

    Why that matters:
    Working out whether two ROTATED boxes touch is real maths. For two axis
    aligned ones it is four comparisons, because two rectangles can only
    overlap if they overlap on BOTH axis at the same time. Cheap enough to
    run for every collider, every frame.

    These come straight out of the Tiled JSON. The object layer stores every
    collider as x/y/width/height in world pixels, wich is exactly this shape,
    so we can copy them over 1 to 1.
    */
    struct AABB
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        // The four edges, so the overlap test below reads like plain english.
        // Remember y grows DOWNWARDS on a screen, so Top() is the small one.
        float Left()   const { return x; }
        float Right()  const { return x + w; }
        float Top()    const { return y; }
        float Bottom() const { return y + h; }
    };

    /*
    This code:
    Returns true if two rectangles are really overlapping.

    The trick is to prove they DONT touch instead, wich is much easier.
    Two boxes are seperated if one is completely left of the other, or
    completely right of it, or completely above it, or completely below it.
    If none of those four are true, they have no way of avoiding eachother
    and must be overlapping.

    Note the <= and >= : two boxes that are exactly edge to edge count as NOT
    touching. That is deliberate. After we push the player out of a wall he
    ends up exactly against it, and if that counted as a hit he would be
    detected as stuck inside the wall forever and jitter.
    */
    /*
    This code:
    A piece of level geometry: the rectangle, plus how solid it is.

    Why not just put the flag inside AABB?
    Because an AABB is pure geometry and gets used for things that are not
    level geometry at all, like the players own body. A "is this one way"
    field would be meaningless there. Keeping them seperate means the maths
    stays about rectangles and the gameplay rules live one layer up.

    oneWay is the 'platform' boolean from Tiled. A normal collider blocks you
    from every side; a one way platform only ever stops you from above, so you
    can jump up THROUGH it and land on top.
    */
    struct Collider
    {
        AABB box;
        bool oneWay = false;
    };

    inline bool Overlaps(const AABB& a, const AABB& b)
    {
        if (a.Right()  <= b.Left())   return false; // a is fully left of b
        if (a.Left()   >= b.Right())  return false; // a is fully right of b
        if (a.Bottom() <= b.Top())    return false; // a is fully above b
        if (a.Top()    >= b.Bottom()) return false; // a is fully below b
        return true;                                // no escape, they overlap
    }
}
