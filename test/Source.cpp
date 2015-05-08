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

//////////////////////////////////////////////////////////////////////////

#include "../src/QuadTree.h"
static orc::QuadTree tree(orc::AABB(vec2(0.0f, 0.0f), vec2(799.0f, 599.0f)), 3);

void render()
{
        orc::AABB ms(vec2(mx - 5, my - 5), vec2(mx + 5, my + 5));
        ms.Render(backbuffer, 0xFFFF00FF);



        tree.Render(backbuffer);
}

int main(int argc, char**argv)
{
        SDL_Window* wnd = SDL_CreateWindow("Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
        
        std::thread t([&]() {
        
                auto context = SDL_GL_CreateContext(wnd);
                size_t size = width * height * sizeof(unsigned int);
                
                while (!quit)
                {
                        memset(backbuffer, 0, size);
                        render();
                        glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, backbuffer);
                        SDL_GL_SwapWindow(wnd);
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
                                tree.Insert(point);
                        }
                        break;
                }
        }
        
        t.join();
        SDL_DestroyWindow(wnd);

        return 0;
}