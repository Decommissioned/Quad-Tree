#ifndef _QUADTREE_RENDERER_H
#define _QUADTREE_RENDERER_H

#include "QuadTree.h"

namespace ORC_NAMESPACE
{

        static unsigned int COLOR_TABLE[] = {

                0xFFFFFFFF,
                0xFF00FFFF,
                0xFFFF00FF,
                0xFFFFFF00,
                0xFF0000FF,
                0xFF00FF00,
                0xFFFF0000,
                0xFF007FFF,
                0xFFFFFFFF,
                0xFF00FFFF,
                0xFFFF00FF,
                0xFFFFFF00,
                0xFF0000FF,
                0xFF00FF00,
                0xFFFF0000,
                0xFF007FFF
        };

        template <typename type_p, typename allocator_type = std::allocator<type_p>>
        class QuadTreeRenderer final : public QuadTree < type_p, allocator_type >
        {
                void render(unsigned int* buffer, const QuadTreeNode* node, int depth) const
                {
                        if (node->children != nullptr) // internal node
                        {
                                for (size_t k = 0; k < 4; ++k) render(buffer, &node->children[k], depth);
                        }
                        else // leaf node
                        {
                                for (size_t k = 0; k < node->size; ++k)
                                {
                                        vec2 point = node->content[k].Position();
                                        int x = (int) ceilf(point.x);
                                        int y = (int) ceilf(point.y);
                                        if (x >= 0 && x < 800 && y >= 0 && y < 600)
                                        {
                                                buffer[y * 800 + (x - 1)] = 0xFFFFFFFF;
                                                buffer[y * 800 + (x)] = 0xFFFFFFFF;
                                                buffer[y * 800 + (x + 1)] = 0xFFFFFFFF;
                                                buffer[(y - 1) * 800 + x] = 0xFFFFFFFF;
                                                buffer[(y + 1) * 800 + x] = 0xFFFFFFFF;
                                        }
                                }
                        }
                        if (depth == -1 || node->depth == depth)
                                node->region.Render(buffer, COLOR_TABLE[node->depth]);
                }

        public:

                explicit QuadTreeRenderer(const AABB& region) : QuadTree(region, 10U)
                {}

                explicit QuadTreeRenderer(const AABB& region, allocator_type& allocator) : QuadTree(region, allocator, 10U)
                {}

                void Render(unsigned int* buffer, int depth) const
                {
                        render(buffer, &root, depth);
                }
        };

};

#endif // _QUADTREE_RENDERER_H
