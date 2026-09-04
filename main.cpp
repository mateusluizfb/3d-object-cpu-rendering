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
#include <vector>
#include <stdio.h>
#include <iostream>

int window_width = 1200;
int window_height = 800;
float pixel_h = 10.0f;
float pixel_w = 10.0f;

struct Coord {
    float x;
    float y;
    float z;
};

template <typename T>
void print(const T& arg) {
    std::cout << arg << std::endl;
}

Coord to_2d(Coord coord) {
    return {
        coord.x / coord.z,
        coord.y / coord.z
    };
}

float to_cartesian_x(float x) {
    return (((x + 1) / 2) * window_width) ; // normalize to coordidates where 0 is the center
}

float to_cartesian_y (float y) {
    return (1 - ((y + 1) / 2)) * window_height; // 1 - is needed so -0.5 for example "goes down", where y goes from 1 to -1
}

void render_point(SDL_Renderer* renderer, Coord coord) {
    float x = coord.x;
    float y = coord.y;

    SDL_FRect pixel = {
        to_cartesian_x(x) - (pixel_w / 2.0f),
        to_cartesian_y(y) - (pixel_h / 2.0f), 
        pixel_w,
        pixel_h
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &pixel); 
}

void render_points(SDL_Renderer* renderer, std::vector<Coord>* coords) {
     size_t size = coords->size();

    for (size_t i = 0; i < size; i++) {
        Coord coord = (*coords)[i];
        render_point(renderer, to_2d(coord));
    }
}

void render_lines(SDL_Renderer* renderer, std::vector<Coord>* coords) {
    size_t size = coords->size();
    SDL_FPoint points[size];

    for (size_t i = 0; i < size; i++) {
        Coord coord_2d = to_2d((*coords)[i]);

        float x = to_cartesian_x(coord_2d.x);
        float y = to_cartesian_y(coord_2d.y);

        points[i] = { x, y };
    }

    bool result = SDL_RenderLines(renderer, points, (int) size);  

    if (!result){
        print("Error while drawing lines");
    }
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
        // Front Face Loop
        { 0.5f,  0.5f, 1.0f},
        {-0.5f,  0.5f, 1.0f},
        {-0.5f, -0.5f, 1.0f},
        { 0.5f, -0.5f, 1.0f},
        { 0.5f,  0.5f, 1.0f}, // Close front face

        // Connect Front to Back (Top-Right Edge)
        { 0.5f,  0.5f, 1.5f},

        // Back Face Loop
        {-0.5f,  0.5f, 1.5f},
        {-0.5f, -0.5f, 1.5f},
        { 0.5f, -0.5f, 1.5f},
        { 0.5f,  0.5f, 1.5f}, // Close back face

        // Remaining 3 Connecting Edges
        { 0.5f, -0.5f, 1.5f}, // Move to Back Bottom-Right
        { 0.5f, -0.5f, 1.0f}, // Connect to Front Bottom-Right
        {-0.5f, -0.5f, 1.0f}, // Move to Front Bottom-Left
        {-0.5f, -0.5f, 1.5f}, // Connect to Back Bottom-Left
        {-0.5f,  0.5f, 1.5f}, // Move to Back Top-Left
        {-0.5f,  0.5f, 1.0f}  // Connect to Front Top-Left
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

        for (size_t i = 0; i < coords.size(); i++) {
            Coord* coord_pointer = &coords[i];
            coord_pointer->z += 0.1 * dt;
        }


        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
   
        render_points(renderer, &coords);
        render_lines(renderer, &coords); 

        if (exit) {
            break;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(60); // Force 60 FPS -> 1000 (1s) / 60 = 66.6666...
    }
}
