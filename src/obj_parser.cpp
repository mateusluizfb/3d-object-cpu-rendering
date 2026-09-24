#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "./utils/logger.h" 

struct Coord {
    float x;
    float y;
    float z;
};

struct Faces {
    int v1;
    int v2;
    int v3;
};

struct Mesh {
    std::vector<Coord> coords;
    std::vector<Faces> faces;
};

int main(int arhc, char* argv[]) {
    std::ifstream file("./src/cube.obj");

    if (!file.is_open()) {
        print("ERROR - File not opened");
        return 1;
    }

    std::unique_ptr<Mesh> mesh_ptr = std::make_unique<Mesh>(Mesh{.coords = {}, .faces = {}}); 

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        if (line.front() == 'v') {
            print("Is vector");

            // TODO: Fill vector list
        }

        if (line.front() == 'f') {
            print("Is faces");

            // TODO: Fill faces list
        }
    }

    file.close();

    return 0;
}
