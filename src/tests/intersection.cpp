#include "../scene.h"
#include "../intersection.h"

int main(int argc, char *argv[]) {

    RTCDevice embree_device = rtcNewDevice(nullptr);
    Camera cam;
    std::vector<Shape> shapes;
    shapes.push_back(TriangleMesh{
        {} /*default parameters for ShapeBase*/,
        {Vector3{-1, -1, -1}, Vector3{1, -1, -1}, Vector3{0, 1, -1}},
        {Vector3i{0, 1, 2}},
        {Vector3{0, 1, 0}, Vector3{0, 1, 0}, Vector3{0, 1, 0}}, // normals
        {}, // uvs
        Real(0), // total area
        TableDist1D{}});
    Scene scene(embree_device,
                Camera(),
                {}, /* materials */
                shapes,
                {}, /* lights */
                {}, /* media */
                -1, /* envmap id */
                TexturePool{},
                RenderOptions{},
                "" /* output filename */);

    Ray ray{Vector3{0, 0, 0}, Vector3{0, 0, -1}, Real(0), infinity<Real>()};
    RayDifferential ray_diff;
    std::optional<PathVertex> vertex = intersect(scene, ray, ray_diff);
    if (!vertex) {
        printf("FAIL\n");
        return 1;
    }
    if (distance(vertex->position, Vector3{0, 0, -1}) > Real(1e-3)) {
        printf("FAIL\n");
        return 1;
    }

    printf("SUCCESS\n");
    return 0;
}
