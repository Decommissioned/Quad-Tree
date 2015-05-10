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

#include "../src/QuadTree.h"
#include "../src/SmartPoolAllocator.h"

#include <vector>
std::vector<int> lol;

//////////////////////////////////////////////////////////////////////////

using alloc = orc::SmartPoolAllocator < vec2 > ;

orc::QuadTree<alloc>* tree;

void render()
{
        orc::AABB ms(vec2(mx - 5, my - 5), vec2(mx + 5, my + 5));
        ms.Render(backbuffer, 0xFFFF00FF);



        tree->Render(backbuffer);
}

int main(int argc, char**argv)
{
        /*const int spacing = 50;
        for (int x = 1; x < (800 - spacing); x+=spacing)
        {
                for (int y = 1; y < (600 - spacing); y+= spacing)
                {
                        tree->Insert({x, y});
                }
        }*/

        orc::MemoryPool* pool = orc::MakeMemoryPool(100 * 1024 * 1024);
        alloc a(pool);

        tree = new orc::QuadTree<alloc>(orc::AABB({0.0f, 0.0f}, {400.0f, 300.0f}), a, 3);

        SDL_Window* wnd = SDL_CreateWindow("Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
        
        unsigned int frameID = 0;

        std::thread o([&]() {

                unsigned int last = 0;

                while (!quit)
                {
                        std::cout << (frameID - last) << std::endl;
                        last = frameID;

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
                                vec2 point(e.button.x, 599 - e.button.y);
                                tree->Insert(point);
                        }
                        break;
                }
        }

        
        t.join();
        o.join();
        SDL_DestroyWindow(wnd);

        return 0;
}