#pragma once
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include <vector>
#include <string>

namespace Nova {

// Single vertex in a mesh
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
    Vector3 color; // vertex colour (default white)

    Vertex() : position(), normal(0,1,0), uv(), color(1,1,1) {}
    Vertex(const Vector3& pos, const Vector3& nrm, const Vector2& uv)
        : position(pos), normal(nrm), uv(uv), color(1,1,1) {}
};

// CPU-side mesh data
struct MeshData {
    std::string            name;
    std::vector<Vertex>    vertices;
    std::vector<uint32_t>  indices;

    // Recalculate flat normals from triangle data
    void RecalculateNormals() {
        // Reset
        for (auto& v : vertices) v.normal = Vector3::Zero();

        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            uint32_t i0 = indices[i], i1 = indices[i+1], i2 = indices[i+2];
            Vector3 edge1 = vertices[i1].position - vertices[i0].position;
            Vector3 edge2 = vertices[i2].position - vertices[i0].position;
            Vector3 n = edge1.Cross(edge2).Normalized();
            vertices[i0].normal = vertices[i0].normal + n;
            vertices[i1].normal = vertices[i1].normal + n;
            vertices[i2].normal = vertices[i2].normal + n;
        }
        for (auto& v : vertices) v.normal.Normalize();
    }

    int VertexCount()  const { return (int)vertices.size(); }
    int IndexCount()   const { return (int)indices.size(); }
    int TriangleCount()const { return (int)indices.size() / 3; }

    bool IsValid() const { return !vertices.empty() && !indices.empty(); }
};

// -------------------------------------------------------
// Primitive mesh factory
// -------------------------------------------------------
namespace PrimitiveMesh {

inline MeshData CreateCube(float size = 1.0f) {
    float h = size * 0.5f;
    MeshData mesh;
    mesh.name = "Cube";

    // 24 unique vertices (4 per face) to allow hard normals
    struct FaceData { Vector3 normal; Vector3 p[4]; };
    FaceData faces[6] = {
        { {0,0,1},  {{-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}} }, // front
        { {0,0,-1}, {{h,-h,-h},{-h,-h,-h},{-h,h,-h},{h,h,-h}} }, // back
        { {-1,0,0}, {{-h,-h,-h},{-h,-h,h},{-h,h,h},{-h,h,-h}} }, // left
        { {1,0,0},  {{h,-h,h},{h,-h,-h},{h,h,-h},{h,h,h}} }, // right
        { {0,1,0},  {{-h,h,h},{h,h,h},{h,h,-h},{-h,h,-h}} }, // top
        { {0,-1,0}, {{-h,-h,-h},{h,-h,-h},{h,-h,h},{-h,-h,h}} } // bottom
    };
    Vector2 uvs[4] = {{0,0},{1,0},{1,1},{0,1}};

    for (auto& f : faces) {
        uint32_t base = (uint32_t)mesh.vertices.size();
        for (int i = 0; i < 4; ++i) {
            Vertex v;
            v.position = f.p[i];
            v.normal   = f.normal;
            v.uv       = uvs[i];
            mesh.vertices.push_back(v);
        }
        // Two triangles per face
        mesh.indices.insert(mesh.indices.end(),
            {base,base+1,base+2, base,base+2,base+3});
    }
    return mesh;
}

inline MeshData CreatePlane(float width = 1.0f, float depth = 1.0f, int subX = 1, int subZ = 1) {
    MeshData mesh;
    mesh.name = "Plane";
    float hw = width * 0.5f, hd = depth * 0.5f;
    float stepX = width / subX, stepZ = depth / subZ;

    for (int z = 0; z <= subZ; ++z) {
        for (int x = 0; x <= subX; ++x) {
            Vertex v;
            v.position = { -hw + x*stepX, 0.0f, -hd + z*stepZ };
            v.normal   = { 0, 1, 0 };
            v.uv       = { (float)x/subX, (float)z/subZ };
            mesh.vertices.push_back(v);
        }
    }
    for (int z = 0; z < subZ; ++z) {
        for (int x = 0; x < subX; ++x) {
            uint32_t tl = z*(subX+1)+x, tr = tl+1;
            uint32_t bl = tl+(subX+1),  br = bl+1;
            mesh.indices.insert(mesh.indices.end(), {tl,bl,tr, tr,bl,br});
        }
    }
    return mesh;
}

inline MeshData CreateSphere(float radius = 0.5f, int stacks = 16, int slices = 16) {
    MeshData mesh;
    mesh.name = "Sphere";
    const float PI = 3.14159265f;

    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * j / slices;
            Vertex v;
            v.normal   = { std::sin(phi)*std::cos(theta),
                           std::cos(phi),
                           std::sin(phi)*std::sin(theta) };
            v.position = v.normal * radius;
            v.uv       = { (float)j/slices, (float)i/stacks };
            mesh.vertices.push_back(v);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t tl = i*(slices+1)+j, tr = tl+1;
            uint32_t bl = tl+(slices+1),  br = bl+1;
            mesh.indices.insert(mesh.indices.end(), {tl,bl,tr, tr,bl,br});
        }
    }
    return mesh;
}

inline MeshData CreateCylinder(float radius = 0.5f, float height = 1.0f, int slices = 16) {
    MeshData mesh;
    mesh.name = "Cylinder";
    const float PI = 3.14159265f;
    float h = height * 0.5f;

    // Side
    for (int i = 0; i <= slices; ++i) {
        float t = 2.0f * PI * i / slices;
        float c = std::cos(t), s = std::sin(t);
        for (int j = 0; j < 2; ++j) {
            Vertex v;
            v.position = { radius*c, (j==0)?-h:h, radius*s };
            v.normal   = { c, 0, s };
            v.uv       = { (float)i/slices, (float)j };
            mesh.vertices.push_back(v);
        }
    }
    for (int i = 0; i < slices; ++i) {
        uint32_t b = i*2;
        mesh.indices.insert(mesh.indices.end(), {b,b+2,b+1, b+1,b+2,b+3});
    }
    // Caps
    auto addCap = [&](float y, bool flip) {
        uint32_t center = (uint32_t)mesh.vertices.size();
        Vertex cv; cv.position={0,y,0}; cv.normal={0,flip?-1.0f:1.0f,0}; cv.uv={0.5f,0.5f};
        mesh.vertices.push_back(cv);
        uint32_t start = center+1;
        for (int i = 0; i <= slices; ++i) {
            float t = 2.0f*PI*i/slices;
            Vertex v; v.position={radius*std::cos(t),y,radius*std::sin(t)};
            v.normal=cv.normal; v.uv={(std::cos(t)+1)*0.5f,(std::sin(t)+1)*0.5f};
            mesh.vertices.push_back(v);
        }
        for (int i = 0; i < slices; ++i) {
            if (!flip) mesh.indices.insert(mesh.indices.end(), {center,start+i,start+i+1});
            else       mesh.indices.insert(mesh.indices.end(), {center,start+i+1,start+i});
        }
    };
    addCap(-h, true);
    addCap( h, false);
    return mesh;
}

} // namespace PrimitiveMesh
} // namespace Nova
