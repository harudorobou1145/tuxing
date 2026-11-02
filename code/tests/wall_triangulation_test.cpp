#include <tiny_obj_loader.h>
#include <cmath>
#include <iostream>

// Isolated 79-vertex facade from tiananmen_core.obj: the notched boundary
// includes the archways. Check coverage, rather than a particular triangle order.
int main() {
    tinyobj::ObjReader original, triangulated;
    tinyobj::ObjReaderConfig config;
    config.triangulate = false;
    if (!original.ParseFromFile(WALL_FIXTURE, config)) return 1;
    config.triangulate = true;
    if (!triangulated.ParseFromFile(WALL_FIXTURE, config)) return 1;
    const auto& vertices = original.GetAttrib().vertices;
    const auto& boundary = original.GetShapes().at(0).mesh.indices;
    double polygonArea = 0;
    for (std::size_t i = 0; i < boundary.size(); ++i) {
        const auto a = 3 * boundary[i].vertex_index;
        const auto b = 3 * boundary[(i + 1) % boundary.size()].vertex_index;
        polygonArea += double(vertices[a]) * vertices[b + 1] - double(vertices[b]) * vertices[a + 1];
    }
    polygonArea = std::abs(polygonArea) / 2;
    const auto& indices = triangulated.GetShapes().at(0).mesh.indices;
    const auto cross = [](double ax, double ay, double bx, double by, double px, double py) {
        return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
    };
    double triangleArea = 0;
    bool wallCovered = false, doorwayCovered = false;
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        const auto a = 3 * indices[i].vertex_index;
        const auto b = 3 * indices[i + 1].vertex_index;
        const auto c = 3 * indices[i + 2].vertex_index;
        const double ax = vertices[a], ay = vertices[a + 1];
        const double bx = vertices[b], by = vertices[b + 1];
        const double cx = vertices[c], cy = vertices[c + 1];
        triangleArea += std::abs(cross(ax, ay, bx, by, cx, cy)) / 2;
        const auto covers = [&](double x, double y) {
            const double ab = cross(ax, ay, bx, by, x, y);
            const double bc = cross(bx, by, cx, cy, x, y);
            const double ca = cross(cx, cy, ax, ay, x, y);
            return (ab >= 0 && bc >= 0 && ca >= 0) || (ab <= 0 && bc <= 0 && ca <= 0);
        };
        wallCovered |= covers(-4000, 500);
        doorwayCovered |= covers(-1290, 200);
    }
    if (std::abs(triangleArea - polygonArea) > polygonArea * 0.00001
        || !wallCovered || doorwayCovered) {
        std::cerr << "Facade coverage failed: polygon area=" << polygonArea
                  << ", triangles=" << triangleArea << ", wall=" << wallCovered
                  << ", doorway=" << doorwayCovered << '\n';
        return 1;
    }
}
