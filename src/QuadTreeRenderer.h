#ifndef _QUADTREE_RENDERER_H
#define _QUADTREE_RENDERER_H

#include "QuadTree.h"

// TODO: overhaul this class for configurable widths and height

namespace ORC_NAMESPACE
{

        template <typename type_p, typename allocator_type = std::allocator<type_p>>
        class QuadTreeRenderer final : public QuadTree < type_p, allocator_type >
        {
                void render(unsigned int* buffer, const QuadTreeNode* node, int depth, const unsigned int* table, unsigned int count) const
                {
                        if (node->children != nullptr) // internal node
                        {
                                for (size_t k = 0; k < 4; ++k) render(buffer, &node->children[k], depth, table, count);
                        }
                        else // leaf node
                        {
                                if (depth == -2)
                                {
                                        node->region.Render(buffer, node->depth < count ? table[node->depth] : table[count - 1]);
                                        return;
                                }
                                for (size_t k = 0; k < node->size; ++k)
                                {
                                        vec2 point = node->content[k].Position();
                                        util::Render(point, buffer, 0xFFFFFFFF);
                                }
                        }
                        if (depth == -1 || node->depth == depth)
                                node->region.Render(buffer, node->depth < count ? table[node->depth] : table[count - 1]);
                }

        public:

                explicit QuadTreeRenderer(const AABB& region) : QuadTree(region, 10U)
                {}

                explicit QuadTreeRenderer(const AABB& region, allocator_type& allocator) : QuadTree(region, allocator, 10U)
                {}

                explicit QuadTreeRenderer(const AABB& region, allocator_type& allocator, unsigned int node_capacity) : QuadTree(region, allocator, node_capacity)
                {}

                void Render(unsigned int* buffer, int depth, const unsigned int* table, unsigned int count) const
                {
                        render(buffer, &root, depth, table, count);
                }

                void Render(unsigned int* buffer, int depth) const
                {
                        const static unsigned int COLOR_TABLE[] = {

                                0xFFFFFFFF, 0xFF00FFFF, 0xFFFF00FF, 0xFFFFFF00,
                                0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFF007FFF,
                                0xFFFFFFFF, 0xFF00FFFF, 0xFFFF00FF, 0xFFFFFF00,
                                0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFF007FFF
                        };
                        render(buffer, &root, depth, COLOR_TABLE, 16);
                }

                void Render(unsigned int* buffer) const
                {
                        Render(buffer, -2);
                }
        };

};

#endif // _QUADTREE_RENDERER_H
