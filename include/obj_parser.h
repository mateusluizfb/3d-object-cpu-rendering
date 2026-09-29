#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include <vector>
#include "common.h" 

struct Faces {
    int v1;
    int v2;
    int v3;
};

struct Mesh {
    std::vector<Coord> coords;
    std::vector<Faces> faces;
};

Mesh create_mesh();

#endif
