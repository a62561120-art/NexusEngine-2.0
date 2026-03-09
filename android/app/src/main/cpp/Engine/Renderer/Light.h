#pragma once
#include "../Core/Component.h"
#include "Material.h"
#include "../Math/Vector3.h"

namespace Nova {

enum class LightType {
    Directional,
    Point,
    Spot,
    Ambient
};

// -------------------------------------------------------
// Light – represents a scene light source.
// Attach to a GameObject; direction/position come from Transform.
// -------------------------------------------------------
class Light : public Component {
public:
    Light()
        : m_type(LightType::Directional),
          m_color(Color::White()),
          m_intensity(1.0f),
          m_range(10.0f),
          m_spotAngle(30.0f),
          m_castShadows(false)
    {}

    std::string GetTypeName() const override { return "Light"; }

    void SetType(LightType t)       { m_type = t; }
    void SetColor(const Color& c)   { m_color = c; }
    void SetIntensity(float i)      { m_intensity = i; }
    void SetRange(float r)          { m_range = r; }
    void SetSpotAngle(float deg)    { m_spotAngle = deg; }
    void SetCastShadows(bool s)     { m_castShadows = s; }

    LightType   GetType()       const { return m_type; }
    Color       GetColor()      const { return m_color; }
    float       GetIntensity()  const { return m_intensity; }
    float       GetRange()      const { return m_range; }
    float       GetSpotAngle()  const { return m_spotAngle; }
    bool        CastsShadows()  const { return m_castShadows; }

    // World-space direction (for directional / spot lights)
    // Requires owner Transform
    Vector3 GetDirection() const;
    Vector3 GetPosition()  const;

    // Effective colour scaled by intensity
    Vector3 GetEffectiveColor() const {
        return { m_color.r * m_intensity,
                 m_color.g * m_intensity,
                 m_color.b * m_intensity };
    }

    // Attenuation factor for point/spot lights at given distance
    float Attenuate(float dist) const {
        if (m_range <= 0.0f) return 1.0f;
        float ratio = dist / m_range;
        return std::max(0.0f, 1.0f - ratio * ratio);
    }

private:
    LightType m_type;
    Color     m_color;
    float     m_intensity;
    float     m_range;
    float     m_spotAngle;
    bool      m_castShadows;
};

} // namespace Nova
