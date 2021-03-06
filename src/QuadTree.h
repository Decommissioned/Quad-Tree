#ifndef _QUADTREE_H
#define _QUADTREE_H

#include "config.h"
#include "AABB.h"

#include <allocators>
#include <vector>

// TODO: Make better methods
// TOOD: Test and profile
// TODO: Optimize -- remove recursions -- started
// TODO: Test remove functionality

#pragma inline_recursion(on)
#pragma inline_depth(16)

namespace ORC_NAMESPACE
{

        template <typename type_p, typename allocator_type = std::allocator<type_p>>
        class QuadTree
        {

        protected:

                static const unsigned char max_depth = 16;
                const size_t node_capacity;

                enum GeoRegion
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
                        type_p* content;
                };

                using alloc_t = std::allocator_traits < allocator_type > ;
                using NodeAlloc = typename alloc_t::template rebind_alloc < QuadTreeNode > ;
                using VecAlloc = typename alloc_t::template rebind_alloc < type_p > ;

                NodeAlloc node_alloc;
                VecAlloc vec_alloc;

                QuadTreeNode root;

                static unsigned int partition(const vec2& center, const vec2& point)
                {
                        return ((point.x > center.x) << 1) | (point.y > center.y);
                }

                // Expansion heuristic: only expand if at least half of the elements will change sub quadrant
                bool should_expand(QuadTreeNode* node)
                {
                        unsigned int counter[4] = {0, 0, 0, 0};
                        vec2 center = node->region.Center();
                        for (size_t k = 0; k < node->size; ++k)
                        {
                                unsigned int index = partition(center, node->content[k].Position());
                                ++counter[index];
                        }
                        const size_t half_capacity = node_capacity / 2;
                        for (size_t k = 0; k < 4; ++k)
                        {
                                if (counter[k] >= half_capacity) return true;
                        }
                        return false;
                }

                static void inc_depth(QuadTreeNode* node)
                {
                        node->depth++;
                        if (node->children != nullptr)
                        {
                                for (size_t k = 0; k < 4; ++k) inc_depth(&node->children[k]);
                        }
                }

                type_p* insert(QuadTreeNode* node, const type_p* point)
                {
                        node->content[node->size] = *point;
                        type_p* element = &node->content[node->size];
                        node->size = node->size + 1;
                        if (node->size >= node->capacity) // partitioning or reallocation may be required
                        {
                                if ((node->depth < max_depth) && should_expand(node)) buy(node);
                                else // Simply allocate more memory for the region
                                {
                                        node->capacity += node_capacity;
                                        type_p* new_content = vec_alloc.allocate(node->capacity, node);
                                        std::memcpy(new_content, node->content, sizeof(type_p) * node->size);
                                        vec_alloc.deallocate(node->content, node->size);
                                        node->content = new_content;
                                }
                        }
                        return element;
                }

                void remove(QuadTreeNode* node, type_p* point)
                {
                        bool found = false;
                        for (size_t k = 0; k < node->size; ++k)
                        {
                                node->content[k - found] = node->content[k];
                                if (&node->content[k] == point) found = true;
                        }
                        if (found) --node->size;
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
                        for (type_p* point = parent->content; point != (parent->content + parent->size); ++point)
                        {
                                unsigned int child_index = partition(center, point->Position());
                                insert(&parent->children[child_index], point);
                        }

                        // Finally, clean up the parent node
                        vec_alloc.deallocate(parent->content, node_capacity);
                        parent->size = 0;
                        parent->content = nullptr;
                }

                static void ascend(QuadTreeNode*& node, vec2 point)
                {
                        while (node->parent != nullptr)
                        {
                                if (node->region.Inside(point)) return;
                                node = node->parent;
                        }
                }

                static void descend(QuadTreeNode*& node, vec2 point)
                {
                        while (node->children != nullptr)
                        {
                                unsigned int index = partition(node->region.Center(), point);
                                node = &node->children[index];
                        }
                }

                void expand(vec2 point) // This is extremely slow, it's best to avoid adding elements outside of the region altogether
                {
                        vec2 ne, sw;
                        unsigned int target;

                        switch (partition(root.region.Center(), point))
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
                                target = SOUTHEAST;
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
                                        case NORTHWEST: pos = region.TopLeft(); break;
                                        case NORTHEAST: pos = region.TopRight(); break;
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

                template <typename alloc = std::allocator<type_p*>>
                void query(std::vector<type_p*>& results, const AABB& region, const QuadTreeNode* node) const
                {
                        if (node->children != nullptr) // internal node, descend
                        {
                                for (size_t k = 0; k < 4; ++k)
                                {
                                        QuadTreeNode* child = &node->children[k];
                                        if (child->region.Intersect(region))
                                                query(results, region, child);
                                }
                        }
                        else // leaf node
                        {
                                for (size_t k = 0; k < node->size; ++k)
                                {
                                        type_p& item = node->content[k];
                                        if (region.Inside(item.Position()))
                                                results.emplace_back(&item);
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

                virtual ~QuadTree()
                {
                        free(&root);
                }

                type_p* Insert(const type_p& item)
                {
                        // If inserting outside of the region expansion is required
                        while (!root.region.Inside(item.Position()))
                                expand(item.Position());

                        // Descend to the appropriate leaf
                        QuadTreeNode* current = &root;
                        descend(current, item.Position());

                        // Perform the operation
                        return insert(current, &item);
                }

                template <typename alloc = std::allocator<type_p*>>
                std::vector<type_p*, alloc> Query(const AABB& region) const
                {
                        std::vector<type_p*> results;

                        if (root.region.Intersect(region))
                                query(results, region, &root);

                        return results;
                }

                void Remove(type_p* element)
                {
                        QuadTreeNode* current = &root;
                        descend(current, item.Position());
                        remove(current, element);
                }

                type_p* Move(type_p* element, const vec2& to)
                {
                        QuadTreeNode* source = &root;
                        QuadTreeNode* destination;

                        // Go to the point
                        descend(source, element->Position());

                        // Move back up until it's in range
                        destination = source;
                        ascend(destination, to);

                        // If it's inside the same region, just update its position
                        if (destination == source)
                        {
                                element->Position(to);
                                return element;
                        }

                        // Go to the leaf node containing the destination point
                        descend(destination, to);

                        // Update its position, remove from source and insert in destination
                        element->Position(to);
                        type_p* new_element = insert(destination, element);
                        remove(source, element);
                        return new_element;
                }

                const AABB& Region() const
                {
                        return root.region;
                }

        };

};

#endif // _QUADTREE_H
