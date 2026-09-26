#include <vector>

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

Mesh create_mesh();
