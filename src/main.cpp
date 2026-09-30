#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include <stdio.h>

#include "common.h" 
#include "obj_parser.h" 
#include "logger.h" 

double PI = 3.141592;

int window_width = 1200;
int window_height = 800;
float pixel_h = 10.0f;
float pixel_w = 10.0f;

Coord to_2d(Coord coord) {
    return {
        coord.x / coord.z,
        coord.y / coord.z,
        coord.z,
    };
}

float to_cartesian_x(float x) {
    return (((x + 1) / 2) * window_width) ; // transform coordinates from NDCto the SDL coorisnate system, where the top-left-edge is 0
}

float to_cartesian_y (float y) {
    // transform coordinates from NDC to the SDL coordinate system, where the top-left-edge is 0

    return (1 - ((y + 1) / 2)) * window_height; // 1 - is needed so -0.5 for example "goes down", where y goes from 1 to -1
}

Coord to_cartesian(Coord coord) {
    return {
        to_cartesian_x(coord.x),
        to_cartesian_y(coord.y),
        0
    };
}

void render_point(SDL_Renderer* renderer, Coord coord) {
    float x = coord.x;
    float y = coord.y;
    float w = pixel_w - (coord.z - 1);
    float h = pixel_h - (coord.z - 1);

    SDL_FRect pixel = {
        to_cartesian_x(x) - (w / 2.0f),
        to_cartesian_y(y) - (h / 2.0f), 
        w,
        h
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

void render_flat_bottom_triangle(SDL_Renderer* renderer, Coord p0, Coord p1, Coord p2) {
    // TODO: I still don't understand the need of this:
    float inv_slope1 = (p1.x - p0.x) / (p1.y - p0.y);
    float inv_slope2 = (p2.x - p0.x) / (p2.y - p0.y);

    float cur_x1 = p0.x;
    float cur_x2 = p0.x;

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

    for (int scan_line = p0.y; scan_line <= p1.y; scan_line++) {
        int start = (int) std::min(cur_x1, cur_x2);
        int end   = (int) std::max(cur_x1, cur_x2);

        SDL_RenderLine(renderer, start, scan_line, end, scan_line);

        cur_x1 += inv_slope1;
        cur_x2 += inv_slope2;
    }
}

void render_flat_top_triangle(SDL_Renderer* renderer, Coord p0, Coord p1, Coord p2) {
    // TODO: I still don't understand the need of this:
    float inv_slope1 = (p2.x - p0.x) / (p2.y - p0.y);
    float inv_slope2 = (p2.x - p1.x) / (p2.y - p1.y);

    float cur_x1 = p2.x;
    float cur_x2 = p2.x;

    print("coord 0: " + std::to_string(p0.x));
    print("coord 1: " + std::to_string(p1.x));
    print("coord 2: " + std::to_string(p2.x));

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    for (int scan_line = p2.y; scan_line >= p0.y; scan_line--) {
        int start = (int) std::min(cur_x1, cur_x2);
        int end   = (int) std::max(cur_x1, cur_x2);

        SDL_RenderLine(renderer, start, scan_line, end, scan_line);
        print("start: " + std::to_string(start));
        print("end: " + std::to_string(end));
        print("scan_line: " + std::to_string(scan_line));
        cur_x1 -= inv_slope1;
        cur_x2 -= inv_slope2;
    }
}

void render_triangles(SDL_Renderer* renderer, Mesh mesh) {
    print("render triangles");

    for (size_t i = 0; i < mesh.faces.size(); i++) {
        Coord v1 = mesh.coords[mesh.faces[i].v1 - 1];
        Coord v2 = mesh.coords[mesh.faces[i].v2 - 1];
        Coord v3 = mesh.coords[mesh.faces[i].v3 - 1];

        Coord p0 = to_cartesian(to_2d(v1));
        Coord p1 = to_cartesian(to_2d(v2));
        Coord p2 = to_cartesian(to_2d(v3));

        if (p0.y > p1.y) std::swap(p0, p1);
        if (p0.y > p2.y) std::swap(p0, p2);
        if (p1.y > p2.y) std::swap(p1, p2);

        if (p0.y == p2.y) return; // Ignore flat triangles

        if (p1.y == p2.y) {
            render_flat_bottom_triangle(renderer, p0, p1, p2);

            continue;
        }

        if (p0.y == p1.y) {
            render_flat_top_triangle(renderer, p0, p1, p2);

            continue;
        }
        // split triangle to form a flat-bottom and flat-top triangles
        // then fill them
        
        // linear interpolation along the long edge:
        float x3 = p0.x + ((p1.y - p0.y) / (p2.y - p0.y)) * (p2.x - p0.x);

        Coord p3 = { x3, p1.y };

        render_flat_bottom_triangle(renderer, p0, p1, p3);
        render_flat_top_triangle(renderer, p1, p3, p2);
    }
}

Coord get_center(const std::vector<Coord>& coords) {
    float min_x = coords[0].x; 
    float min_y = coords[0].y; 
    float min_z = coords[0].z;

    float max_x = coords[0].x;
    float max_y = coords[0].y;
    float max_z = coords[0].z;

    for (const auto& c : coords) {
        if (c.x < min_x) min_x = c.x;
        if (c.x > max_x) max_x = c.x;
        if (c.y < min_y) min_y = c.y;
        if (c.y > max_y) max_y = c.y;
        if (c.z < min_z) min_z = c.z;
        if (c.z > max_z) max_z = c.z;
    }

    return {
        (min_x + max_x) / 2.0f,
        (min_y + max_y) / 2.0f,
        (min_z + max_z) / 2.0f
    };
}

void update_rotate(float dt, Coord center, Coord* coord_pointer) {
    double rotation_rate = (0.8 * dt);

    float x = (coord_pointer->x - center.x) * std::cos(rotation_rate) - (coord_pointer->z - center.z) * std::sin(rotation_rate);
    float z = (coord_pointer->x - center.x) * std::sin(rotation_rate) + (coord_pointer->z - center.z) * std::cos(rotation_rate);

    coord_pointer->x = x + center.x;
    coord_pointer->z = z + center.z;
}

void update_move_back(float dt, Coord* coord_pointer) {
    coord_pointer->z += 0.5 * dt; 
}

std::vector<Coord> mesh_to_coords(Mesh mesh) {
    std::vector<Coord> coords = {};

    for (size_t i = 0; i < mesh.faces.size(); i++) {
        Coord v1 = mesh.coords[mesh.faces[i].v1 - 1];
        Coord v2 = mesh.coords[mesh.faces[i].v2 - 1];
        Coord v3 = mesh.coords[mesh.faces[i].v3 - 1];

        coords.push_back(v1);
        coords.push_back(v2);
        coords.push_back(v3);
        coords.push_back(v1);

        // TODO: There are overlapping edges, from overlapping triangles
    }

    return coords;
} 

int main(int arhc, char* argv[]) {
    print("Starting");

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("3D CPU Rendering test", window_width, window_height, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Event event;
    bool exit = false;

    Uint64 frameStart = -1;

    Mesh mesh = create_mesh(); 
    std::vector<Coord> coords = mesh_to_coords(mesh);

    // Main Loop
    while(true) {
        Uint64 ticks = SDL_GetTicks();
        float dt = (ticks - frameStart) / 1000.0;
        frameStart = ticks;

        // Update
        Coord center = get_center(coords);

        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                exit = true;
            }
        }

        for (size_t i = 0; i < coords.size(); i++) {
            Coord* coord_pointer = &coords[i];
            
            // rotate
            update_rotate(dt, center, coord_pointer); 

            // move back
            update_move_back(dt, coord_pointer);
        }


        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
  
        render_triangles(renderer, mesh);
        render_points(renderer, &coords);
        render_lines(renderer, &coords);

        if (exit) {
            break;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(60); // Force 60 FPS -> 1000 (1s) / 60 = 66.6666...
    }
}
