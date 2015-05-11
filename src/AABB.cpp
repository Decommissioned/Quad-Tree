#include "AABB.h"

#include <cmath>

template <typename type>
static type max(type a, type b)
{
        return a > b ? a : b;
}
template <typename type>
static type min(type a, type b)
{
        return a > b ? b : a;
}

namespace ORC_NAMESPACE
{

        AABB::AABB(const vec2& sw, const vec2& ne)
        {
                this->sw.x = min(sw.x, ne.x);
                this->sw.y = min(sw.y, ne.y);
                this->ne.x = max(sw.x, ne.x);
                this->ne.y = max(sw.y, ne.y);
        }

        AABB::AABB()
        {

        }

        bool AABB::Intersect(const AABB& other) const
        {
                return !(sw.x > other.ne.x ||
                         ne.x < other.sw.x ||
                         ne.y < other.sw.y ||
                         sw.y > other.ne.y );
        }

        bool AABB::Inside(const vec2& point) const
        {
                return point.x >= sw.x &&
                       point.x <= ne.x &&
                       point.y >= sw.y &&
                       point.y <= ne.y;
        }

        void AABB::Render(unsigned int* buffer, unsigned int color) const
        {

                int xs = (int) ceilf(sw.x);
                int xe = (int) ceilf(ne.x);
                
                int ys = (int) ceilf(sw.y);
                int ye = (int) ceilf(ne.y);

                if (ys >= 0 && ys < 600)
                {
                        for (int x = max(xs, 0); x < min(xe, 800); x++)
                                buffer[ys * 800 + x] = color;
                }

                if (ye >= 0 && ye < 600)
                {
                        for (int x = max(xs, 0); x < min(xe, 800); x++)
                                buffer[ye * 800 + x] = color;
                }

                if (xs >= 0 && xs < 800)
                {
                        for (int y = max(ys, 0); y < min(ye, 600); y++)
                                buffer[y * 800 + xs] = color;
                }

                if (xe >= 0 && xe < 800)
                {
                        for (int y = max(ys, 0); y < min(ye, 600); y++)
                                buffer[y * 800 + xe] = color;
                }
        }

        vec2 AABB::BottomLeft() const
        {
                return sw;
        }

        vec2 AABB::BottomRight() const
        {
                return vec2(ne.x, sw.y);
        }

        vec2 AABB::TopLeft() const
        {
                return vec2(sw.x, ne.y);
        }

        vec2 AABB::TopRight() const
        {
                return ne;
        }

        vec2 AABB::Center() const
        {
                return (ne + sw) * 0.5f;
        }


        void util::Render(const vec2& point, unsigned int* buffer, unsigned int color)
        {
                int x = (int) ceilf(point.x);
                int y = (int) ceilf(point.y);
                if (x >= 0 && x < 800 && y >= 0 && y < 600)
                {
                        buffer[y * 800 + (x - 1)] = color;
                        buffer[y * 800 + (x)]     = color;
                        buffer[y * 800 + (x + 1)] = color;
                        buffer[(y - 1) * 800 + x] = color;
                        buffer[(y + 1) * 800 + x] = color;
                }
        }

};
