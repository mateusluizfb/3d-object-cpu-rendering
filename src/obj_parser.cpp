#include <sstream>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <string>

#include "obj_parser.h"
#include "logger.h" 

std::vector<std::string> parse_line (std::string line) {
    std::vector<std::string> word_list = {};

    std::stringstream ss(line);
    std::string word;

    while (ss >> word) {
        word_list.push_back(word);
    }

    return word_list;
} 


Mesh create_mesh() {
    std::ifstream file("./src/cube.obj");

    if (!file.is_open()) {
        throw std::runtime_error("Obj file not opened");
    }

    Mesh mesh_ptr = Mesh{.coords = {}, .faces = {}}; 

    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> word_list = parse_line(line);


        if (word_list[0] == "v") {
            float x = std::stof(word_list[1]);
            float y = std::stof(word_list[2]);
            float z = std::stof(word_list[3]);

            mesh_ptr.coords.push_back(Coord{x, y, z});
            continue;
        }

        if (word_list[0] == "f") {

            int v1 = std::stoi(word_list[1]);
            int v2 = std::stoi(word_list[2]);
            int v3 = std::stoi(word_list[3]);

            mesh_ptr.faces.push_back(Faces{v1, v2, v3});
            continue;
        }
    }

    file.close();

    return mesh_ptr;
}
