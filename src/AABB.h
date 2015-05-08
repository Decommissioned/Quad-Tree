#ifndef _AABB_H
#define _AABB_H

#include "config.h"

#include <glm/vec2.hpp>
using glm::vec2;

namespace ORC_NAMESPACE
{

        class AABB final
        {
                
                vec2 sw;
                vec2 ne;

        public:

                AABB();
                AABB(const vec2& sw, const vec2& ne);
                
                bool Intersect(const AABB& other) const;
                bool Inside(const vec2& point) const;

                void Render(unsigned int* buffer, unsigned int color) const;

                vec2 BottomLeft() const;
                vec2 BottomRight() const;
                vec2 TopLeft() const;
                vec2 TopRight() const;
                vec2 Center() const;

        };

};

#endif // _AABB_H
