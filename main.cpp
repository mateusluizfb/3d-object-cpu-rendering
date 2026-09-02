#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <vector>
#include <stdio.h>
#include <iostream>

int window_width = 1200;
int window_height = 800;

struct Coord {
    float x;
    float y;
    float z;
};

template <typename T>
void print(const T& arg) {
    std::cout << arg << std::endl;
}

void render_point(SDL_Renderer* renderer, Coord coord) {
    float w = 10.0f;
    float h = 10.0f;
    float x = coord.x;
    float y = coord.y;

    SDL_FRect pixel = {
        (((x + 1) / 2) * window_width) - (w / 2.0f), // normalize to coordidates where 0 is the center, minus the offset to centralize it for real
        (1 - ((y + 1) / 2)) * window_height - (h / 2.0f), // 1 - is needed so -0.5 for example "goes down", where y goes from 1 to -1 
        w,
        h
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &pixel); 
}

Coord to_2d(Coord* coord) {
    return {
        coord->x / coord->z,
        coord->y / coord->z
    };
}

int main(int arhc, char* argv[]) {
    print("Starting");

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("3D CPU Rendering test", window_width, window_height, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Event event;
    bool exit = false;

    Uint64 frameStart = -1;
    std::vector<Coord> coords = {
        { 0.5, 0.5, 1.0 },
        { -0.5, 0.5, 1.0}
    };

    // Main Loop
    while(true) {
        Uint64 ticks = SDL_GetTicks();
        float dt = (ticks - frameStart) / 1000.0;
        frameStart = ticks;

        // Update
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                exit = true;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

         

        for (size_t i = 0; i < coords.size(); i++) {
            Coord* coord = &coords[i];
            coord->z += 0.1 * dt;
            render_point(renderer, to_2d(coord));
        }
        

        if (exit) {
            break;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(33); // Force 30 FPS -> 1000 (1s) / 30 = 33.3333...
    }
}
