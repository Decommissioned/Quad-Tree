#ifndef _QUADTREE_H
#define _QUADTREE_H

#include "config.h"
#include "AABB.h"

#include <memory>
#include <vector>

// TODO: Remove operations
// TODO: Optimize

#pragma inline_recursion(on)

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

namespace ORC_NAMESPACE
{

        template <typename allocator_type = std::allocator<vec2>>
        class QuadTree final
        {

                static const unsigned char max_depth = 16;
                const size_t node_capacity;

                enum
                {
                        NORTHEAST = 3, SOUTHEAST = 2, SOUTHWEST = 0, NORTHWEST = 1
                };

                struct QuadTreeNode
                {
                        unsigned char depth;
                        QuadTreeNode* parent;
                        QuadTreeNode* children;
                        AABB region;
                        size_t size;
                        size_t capacity;
                        vec2* content;
                };

                using alloc_t = std::allocator_traits < allocator_type > ;
                using NodeAlloc = typename alloc_t::template rebind_alloc < QuadTreeNode > ;
                using VecAlloc = typename alloc_t::template rebind_alloc < vec2 > ;

                NodeAlloc node_alloc;
                VecAlloc vec_alloc;

                QuadTreeNode root;

                static unsigned int partition(const vec2* center, const vec2* point)
                {
                        return ((point->x > center->x) << 1) | (point->y > center->y);
                }

                static void inc_depth(QuadTreeNode* node)
                {
                        node->depth++;
                        if (node->children != nullptr)
                        {
                                for (size_t k = 0; k < 4; ++k) inc_depth(&node->children[k]);
                        }
                }

                void insert(QuadTreeNode* node, const vec2* point)
                {
                        if (node->children != nullptr) // internal node, descend
                        {
                                vec2 center = node->region.Center();
                                unsigned int child_index = partition(&center, point);
                                insert(&node->children[child_index], point);
                        }
                        else // leaf node
                        {
                                node->content[node->size] = *point;
                                node->size = node->size + 1;
                                if (node->size == node->capacity) // partitioning or reallocation required
                                {
                                        if (node->depth < max_depth) buy(node);
                                        else // if maximum depth has been reached, simply reallocate memory for the node
                                        {
                                                node->capacity += node_capacity;
                                                vec2* new_content = vec_alloc.allocate(node->capacity, node);
                                                std::memcpy(new_content, node->content, sizeof(vec2) * node->size);
                                                vec_alloc.deallocate(node->content, node->size);
                                                node->content = new_content;
                                        }
                                }
                        }
                }

                void buy(QuadTreeNode* parent)
                {
                        // Initializing children nodes
                        parent->children = node_alloc.allocate(4, parent);
                        for (size_t k = 0; k < 4; ++k)
                        {
                                parent->children[k].parent = parent;
                                parent->children[k].children = nullptr;
                                parent->children[k].size = 0;
                                parent->children[k].content = vec_alloc.allocate(node_capacity, &parent->children[k]);
                                parent->children[k].depth = parent->depth + 1;
                                parent->children[k].capacity = node_capacity;
                        }

                        vec2 center = parent->region.Center();
                        parent->children[SOUTHWEST].region = AABB(parent->region.BottomLeft(), center);
                        parent->children[SOUTHEAST].region = AABB(parent->region.BottomRight(), center);
                        parent->children[NORTHWEST].region = AABB(parent->region.TopLeft(), center);
                        parent->children[NORTHEAST].region = AABB(parent->region.TopRight(), center);

                        // Copying existing points to the correct child
                        for (vec2* point = parent->content; point != parent->content + parent->size; ++point)
                        {
                                unsigned int child_index = partition(&center, point);
                                insert(&parent->children[child_index], point);
                        }

                        // Finally, clean up the parent node
                        vec_alloc.deallocate(parent->content, node_capacity);
                        parent->size = 0;
                        parent->content = nullptr;
                }

                void expand(vec2 point) // This is extremely slow, it's best to avoid adding elements outside of the region altogether
                {
                        vec2 ne;
                        vec2 sw;

                        unsigned int target;

                        switch (partition(&root.region.Center(), &point))
                        {
                        case SOUTHWEST:
                                ne = root.region.TopRight();
                                sw = 2.0f * root.region.BottomLeft() - ne;
                                target = NORTHEAST;
                                break;
                        case SOUTHEAST:
                                ne = root.region.TopLeft();
                                sw = 2.0f * root.region.BottomRight() - ne;
                                target = NORTHWEST;
                                break;
                        case NORTHEAST:
                                ne = root.region.BottomLeft();
                                sw = 2.0f * root.region.TopRight() - ne;
                                target = SOUTHWEST;
                                break;
                        case NORTHWEST:
                                ne = root.region.BottomRight();
                                sw = 2.0f * root.region.TopLeft() - ne;
                                target = NORTHEAST;
                                break;
                        }

                        QuadTreeNode* intermediates = node_alloc.allocate(4, &root.children[target]);
                        AABB region(ne, sw);

                        for (size_t k = 0; k < 4; ++k)
                        {
                                if (k == target)
                                {
                                        intermediates[k] = root;
                                        inc_depth(&intermediates[k]);
                                }
                                else
                                {
                                        intermediates[k].size = 0;
                                        intermediates[k].capacity = node_capacity;
                                        intermediates[k].content = vec_alloc.allocate(node_capacity, &intermediates[k]);
                                        intermediates[k].children = nullptr;
                                        
                                        vec2 pos;
                                        switch (k)
                                        {
                                        case SOUTHWEST: pos = region.BottomLeft(); break;
                                        case SOUTHEAST: pos = region.BottomRight(); break;
                                        case NORTHEAST: pos = region.TopRight(); break;
                                        case NORTHWEST: pos = region.TopLeft(); break;
                                        }

                                        intermediates[k].region = AABB(pos, region.Center());
                                }

                                intermediates[k].parent = &root;
                                intermediates[k].depth = root.depth + 1;
                        }

                        if (root.size > 0) vec_alloc.deallocate(root.content, root.size);
                        root.size = 0;
                        root.capacity = 0;
                        root.region = region;
                        root.children = intermediates;
                }

                void query(std::vector<vec2>& vec, const AABB& region, const QuadTreeNode* node) const
                {
                        if (node->children != nullptr) // internal node, descend
                        {
                                for (size_t k = 0; k < 4; ++k)
                                {
                                        QuadTreeNode* child = &node->children[k];
                                        if (child->region.Intersect(region))
                                                query(vec, region, child);
                                }
                        }
                        else // leaf node
                        {
                                for (size_t k = 0; k < node->size; ++k)
                                {
                                        vec2 point = node->content[k];
                                        if (region.Inside(point))
                                                vec.emplace_back(point);
                                }
                        }
                }

                void free(QuadTreeNode* node)
                {
                        if (node->children != nullptr) // internal node
                        {
                                for (size_t k = 0; k < 4; ++k) free(&node->children[k]);
                                node_alloc.deallocate(node->children, 4);
                        }
                        else // leaf node
                        {
                                vec_alloc.deallocate(node->content, node_capacity);
                        }
                }

                void render(unsigned int* buffer, const QuadTreeNode* node) const
                {
                        if (node->children != nullptr) // internal node
                        {
                                for (size_t k = 0; k < 4; ++k) render(buffer, &node->children[k]);
                        }
                        else // leaf node
                        {
                                for (size_t k = 0; k < node->size; ++k)
                                {
                                        vec2 point = node->content[k];
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
                        node->region.Render(buffer, COLOR_TABLE[node->depth]);
                }

                void init_root(const AABB& region)
                {
                        root.region = region;
                        root.size = 0;
                        root.children = nullptr;
                        root.parent = nullptr;
                        root.content = vec_alloc.allocate(node_capacity, &root);
                        root.depth = 0;
                        root.capacity = node_capacity;
                }

        public:

                explicit QuadTree(const AABB& region, const size_t capacity_hint = 10U) :
                        node_capacity(capacity_hint)
                {
                        init_root(region);

                }

                explicit QuadTree(const AABB& region, allocator_type& allocator, const size_t capacity_hint = 10U) :
                        node_capacity(capacity_hint), node_alloc(allocator), vec_alloc(allocator)
                {
                        init_root(region);
                }

                ~QuadTree()
                {
                        free(&root);
                }

                void Insert(const vec2& point)
                {
                        while (!root.region.Inside(point))
                                expand(point);

                        insert(&root, &point);
                }

                std::vector<vec2> Query(const AABB& region) const
                {
                        // TODO: create allocation heuristic
                        std::vector<vec2> vec;
                        if (root.region.Intersect(region))
                                query(vec, region, &root);
                }

                void Render(unsigned int* buffer) const
                {
                        render(buffer, &root);
                }

                const AABB& Region() const
                {
                        return root.region;
                }

        };

};

#endif // _QUADTREE_H
