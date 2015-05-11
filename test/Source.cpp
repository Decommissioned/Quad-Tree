#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <thread>

#pragma comment(lib, "sdl2.lib")
#pragma comment(lib, "sdl2main.lib")
#pragma comment(lib, "opengl32.lib")

const int width = 800;
const int height = 600;
unsigned int *backbuffer = new unsigned int[width * height];

int mx;
int my;

static bool quit = false;

// #define SMART_ALLOCATOR_DIAGNOSTICS

#include "../src/QuadTreeRenderer.h"
#include "../src/SmartPoolAllocator.h"

#include <vector>

//////////////////////////////////////////////////////////////////////////

struct vec_p
{
        vec2 position;

        const vec2& Position() const
        {
                return position;
        }

        void Position(const vec2& pos)
        {
                position = pos;
        }
};

using alloc = orc::SmartPoolAllocator < vec_p >;
orc::QuadTreeRenderer<vec_p, alloc>* tree;

int depth = -1;
bool insert = true;

void render()
{
        const int width = 49;
        orc::AABB ms(vec2(mx - width, my - width), vec2(mx + width, my + width));
        ms.Render(backbuffer, 0xFFFFFFFF);

        tree->Render(backbuffer, depth);

        auto points = tree->Query(ms);
        for (auto ptr : points)
        {
                orc::util::Render(ptr->Position(), backbuffer, 0xFF0000FF);
        }


        backbuffer[400 * 800 + 350] = 0xFF0000FF;
}

int main(int argc, char**argv)
{
        orc::MemoryPool* pool = orc::MakeMemoryPool(100 * 1024 * 1024);
        alloc a(pool);
        tree = new orc::QuadTreeRenderer<vec_p, alloc>(orc::AABB({0.0f, 0.0f}, {800.0f, 600.0f}), a);

        const int spacing = 50;
        for (int x = 1; x < (800 - spacing); x+=spacing)
        {
                for (int y = 1; y < (600 - spacing); y+= spacing)
                {
                        tree->Insert(vec_p {vec2{x, y}});
                }
        }

        SDL_Window* wnd = SDL_CreateWindow("Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);

        unsigned int frameID = 0;

        std::thread o([&]() {

                unsigned int last = 0;

                while (!quit)
                {
                        std::cout << (frameID - last) << std::endl;
                        last = frameID;

                        auto aabb = tree->Region();
                        vec2 tr = aabb.TopRight();
                        vec2 bl = aabb.BottomLeft();
                        std::cout << "Region: (" << bl.x << ", " << bl.y << "); (" << tr.x << ", " << tr.y << ") Depth = " << depth << std::endl;

                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }

        });

        std::thread t([&]() {
        
                auto context = SDL_GL_CreateContext(wnd);
                size_t size = width * height * sizeof(unsigned int);
                
                while (!quit)
                {
                        memset(backbuffer, 0, size);
                        render();
                        glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, backbuffer);

                        SDL_GL_SwapWindow(wnd);
                        frameID++;
                }
                
                SDL_GL_DeleteContext(context);
        });

        SDL_ShowCursor(0);
        
        SDL_Event e;
        while (!quit)
        {
                SDL_WaitEvent(&e);
                switch (e.type)
                {
                case SDL_QUIT: quit = true;
                        break;
                case SDL_MOUSEMOTION:
                        mx = e.motion.x;
                        my = 599 - e.motion.y;
                        break;
                case SDL_MOUSEBUTTONDOWN:
                        if (e.button.button == 1)
                        {
                                if (insert)
                                {
                                        vec_p item;
                                        item.position.x = e.button.x;
                                        item.position.y = 599 - e.button.y;
                                        tree->Insert(item);
                                }
                        }
                        else if (e.button.button == 3)
                        {
                                insert = !insert;
                        }
                case SDL_KEYDOWN:
                        if (e.key.keysym.sym == 'a' && depth > -1) depth--;
                        if (e.key.keysym.sym == 'b') depth++;
                        break;
                }
        }
        
        t.join();
        o.join();

        delete tree;

        SDL_DestroyWindow(wnd);

        return 0;
}