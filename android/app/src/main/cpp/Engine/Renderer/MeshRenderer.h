#pragma once
#include "../Core/Component.h"
#include "Mesh.h"
#include "Material.h"
#include <memory>

namespace Nova {

// -------------------------------------------------------
// MeshRenderer – binds a MeshData + Material to a
// GameObject for rendering.  The actual GL draw call is
// issued by the Renderer system, which reads these.
// -------------------------------------------------------
class MeshRenderer : public Component {
public:
    MeshRenderer()
        : m_gpuMeshID(0), m_gpuUploaded(false),
          m_castShadow(true), m_receiveShadow(true)
    {
        m_material = Material::Default();
    }

    std::string GetTypeName() const override { return "MeshRenderer"; }

    // --- Mesh ---
    void SetMeshData(const MeshData& data) {
        m_meshData    = data;
        m_gpuUploaded = false; // force re-upload
    }
    const MeshData& GetMeshData() const { return m_meshData; }
    MeshData&       GetMeshData()       { return m_meshData; }
    bool HasMesh() const { return m_meshData.IsValid(); }

    // --- Material ---
    void SetMaterial(const Material& mat) { m_material = mat; }
    const Material& GetMaterial() const   { return m_material; }
    Material&       GetMaterial()         { return m_material; }

    // --- GPU state (managed by Renderer) ---
    uint32_t GetGPUMeshID()     const { return m_gpuMeshID; }
    void     SetGPUMeshID(uint32_t id) { m_gpuMeshID = id; m_gpuUploaded = true; }
    bool     IsGPUUploaded()    const { return m_gpuUploaded; }
    void     MarkDirty()              { m_gpuUploaded = false; }

    // --- Shadow flags ---
    bool CastsShadow()   const { return m_castShadow; }
    bool ReceivesShadow()const { return m_receiveShadow; }
    void SetCastShadow(bool v)    { m_castShadow = v; }
    void SetReceiveShadow(bool v) { m_receiveShadow = v; }

private:
    MeshData  m_meshData;
    Material  m_material;
    uint32_t  m_gpuMeshID;
    bool      m_gpuUploaded;
    bool      m_castShadow;
    bool      m_receiveShadow;
};

} // namespace Nova
