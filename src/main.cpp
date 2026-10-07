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
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <stdio.h>
#include <algorithm>

#include "common.h" 
#include "obj_parser.h" 
#include "logger.h" 

double PI = 3.141592;

int window_width = 1200;
int window_height = 800;
float pixel_h = 10.0f;
float pixel_w = 10.0f;

SDL_Color color_red = {255, 0, 0,};
SDL_Color color_green = {0, 255, 0};
SDL_Color color_blue = {0, 0, 255};

Coord light_source = Coord{1.0, 0.5, 0.8};

Coord to_2d(Coord coord) {
    return {
        coord.x / coord.z,
        coord.y / coord.z,
        coord.z,
    };
}

float to_cartesian_x(float x) {
    return (((x + 1) / 2) * window_width) ; // transform coordinates from NDC to the SDL coordinate system, where the top-left-edge is 0
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

void render_points(SDL_Renderer* renderer, Mesh mesh) {
    std::vector<Coord> coords = mesh.coords;

    size_t size = coords.size();

    for (size_t i = 0; i < size; i++) {
        Coord coord = coords[i];
        render_point(renderer, to_2d(coord));
    }
}

void render_lines(SDL_Renderer* renderer, Mesh mesh) {
    std::vector<Coord> coords = {};

    // To use SDL_RenderLines, we need an extra coord
    // that tells the api where is the last connecting point
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


    size_t size = coords.size();
    SDL_FPoint points[size];

    for (size_t i = 0; i < size; i++) {
        Coord coord_2d = to_2d(coords[i]);

        float x = to_cartesian_x(coord_2d.x);
        float y = to_cartesian_y(coord_2d.y);

        points[i] = { x, y };
    }

    bool result = SDL_RenderLines(renderer, points, (int) size);  

    if (!result){
        print("Error while drawing lines");
    }
}

void render_light_lines(SDL_Renderer* renderer, Coord coord) {
    Coord light_source = Coord{1.0, 0.5, 1.0};

    Coord lg_coord_2d = to_cartesian(to_2d(light_source));
    Coord target_coord_2d = to_cartesian(to_2d(coord));

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderLine(
        renderer,
        lg_coord_2d.x,
        lg_coord_2d.y,
        target_coord_2d.x,
        target_coord_2d.y
    );
}

Coord normalize(Coord v) {
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    
    if (length == 0.0f) return Coord{0, 0, 0};
    
    return Coord{v.x / length, v.y / length, v.z / length};
}

float get_facing_light_dir(Coord v1, Coord v2, Coord v3) {
    Coord center = Coord{
        (v1.x + v2.x + v3.x) / 3,
        (v1.y + v2.y + v3.y) / 3,
        (v1.z + v2.z + v3.z) / 3,
    }; 

    // A = v2 - v1
    float A_x = v2.x - v1.x;
    float A_y = v2.y - v1.y;
    float A_z = v2.z - v1.z;

    // B = v3 - v1
    float B_x = v3.x - v1.x;
    float B_y = v3.y - v1.y;
    float B_z = v3.z - v1.z;

    // A X B = (A_y B_z - A_z B_y), (A_z B_x - A_x B_z), (A_x B_y - A_y B_x)
    Coord normal = normalize(Coord{
        (A_y * B_z) - (A_z * B_y),
        (A_z * B_x) - (A_x * B_z),
        (A_x * B_y) - (A_y * B_x)
    });

    Coord light_direction = normalize(Coord{
        light_source.x - center.x,
        light_source.y - center.y,
        light_source.z - center.z
    });

    float result = (normal.x * light_direction.x) + (normal.y * light_direction.y) + (normal.z * light_direction.z);

    return result;
}

void render_flat_bottom_triangle(SDL_Renderer* renderer, Coord p0, Coord p1, Coord p2, SDL_Color color) {
    // TODO: I still don't understand the need of this, its a interpolation function afaik:
    float inv_slope1 = (p1.x - p0.x) / (p1.y - p0.y);
    float inv_slope2 = (p2.x - p0.x) / (p2.y - p0.y);

    float cur_x1 = p0.x;
    float cur_x2 = p0.x;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255); // blue

    for (int scan_line = p0.y; scan_line <= p1.y; scan_line++) {
        int start = (int) std::min(cur_x1, cur_x2);
        int end   = (int) std::max(cur_x1, cur_x2);

        SDL_RenderLine(renderer, start, scan_line, end, scan_line);

        cur_x1 += inv_slope1;
        cur_x2 += inv_slope2;
    }
}

void render_flat_top_triangle(SDL_Renderer* renderer, Coord p0, Coord p1, Coord p2, SDL_Color color) {
    // TODO: I still don't understand the need of this:
    float inv_slope1 = (p2.x - p0.x) / (p2.y - p0.y);
    float inv_slope2 = (p2.x - p1.x) / (p2.y - p1.y);

    float cur_x1 = p2.x;
    float cur_x2 = p2.x;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255); // green

    for (int scan_line = p2.y; scan_line >= p0.y; scan_line--) {
        int start = (int) std::min(cur_x1, cur_x2);
        int end   = (int) std::max(cur_x1, cur_x2);

        SDL_RenderLine(renderer, start, scan_line, end, scan_line);
        cur_x1 -= inv_slope1;
        cur_x2 -= inv_slope2;
    }
}

void render_triangles(SDL_Renderer* renderer, Mesh mesh) {
    // z-index sorting:
    std::sort(mesh.faces.begin(), mesh.faces.end(), [mesh](Faces& a, Faces& b) {
        Coord av1 = mesh.coords[a.v1 - 1];
        Coord av2 = mesh.coords[a.v2 - 1];
        Coord av3 = mesh.coords[a.v3 - 1];

        Coord bv1 = mesh.coords[b.v1 - 1];
        Coord bv2 = mesh.coords[b.v2 - 1];
        Coord bv3 = mesh.coords[b.v3 - 1];

        float acenter = (av1.z + av2.z + av3.z) / 3;
        float bcenter = (bv1.z + bv2.z + bv3.z) / 3;

        return (acenter > bcenter);
    });


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

        if (p0.y == p2.y) continue; // Ignore flat triangles

        // linear interpolation along the long edge:
        float x3 = p0.x + ((p1.y - p0.y) / (p2.y - p0.y)) * (p2.x - p0.x);
        Coord p3 = { x3, p1.y };

        float light_dir = get_facing_light_dir(v1, v2, v3);
        uint8_t blue_color_tone = static_cast<int>(255.0 * std::max(light_dir, 0.0f));

        render_flat_bottom_triangle(renderer, p0, p1, p3, {0, 0, blue_color_tone, 255});
        render_flat_top_triangle(renderer, p1, p3, p2, {0, 0, blue_color_tone, 255});
       
        Coord center = Coord{
            (v1.x + v2.x + v3.x) / 3,
            (v1.y + v2.y + v3.y) / 3,
            (v1.z + v2.z + v3.z) / 3,
        }; 
        render_light_lines(renderer, center);
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
    double rotation_rate = (0.3 * dt);

    float x = (coord_pointer->x - center.x) * std::cos(rotation_rate) - (coord_pointer->z - center.z) * std::sin(rotation_rate);
    float z = (coord_pointer->x - center.x) * std::sin(rotation_rate) + (coord_pointer->z - center.z) * std::cos(rotation_rate);

    coord_pointer->x = x + center.x;
    coord_pointer->z = z + center.z;
}

void update_move_back(float dt, Coord* coord_pointer) {
    coord_pointer->z += 0.3 * dt; 
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
    std::vector<Coord>* mesh_coords_ptr = &mesh.coords;
    // std::vector<Coord> coords = mesh_to_coords(mesh);

    // Main Loop
    while(true) {
        Uint64 ticks = SDL_GetTicks();
        float dt = (ticks - frameStart) / 1000.0;
        frameStart = ticks;

        // Update
        Coord center = get_center(*mesh_coords_ptr);

        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                exit = true;
            }
        }

        for (size_t i = 0; i < mesh_coords_ptr->size(); i++) {
            Coord* coord_pointer = &(*mesh_coords_ptr)[i];
            
            // rotate
            update_rotate(dt, center, coord_pointer); 

            // move back
            update_move_back(dt, coord_pointer);
        }

        if (exit) {
            break;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
  
        render_triangles(renderer, mesh);
        render_points(renderer, mesh);
        render_lines(renderer, mesh);

        SDL_RenderPresent(renderer);
        SDL_Delay(30); // Force 60 FPS -> 1000 (1s) / 60 = 66.6666...
    }
}
