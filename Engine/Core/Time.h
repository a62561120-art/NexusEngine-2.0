#pragma once
#include <chrono>

namespace Nova {

class Time {
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    static Time& Get() {
        static Time instance;
        return instance;
    }

    void Init() {
        m_startTime   = Clock::now();
        m_lastFrame   = m_startTime;
        m_deltaTime   = 0.0f;
        m_totalTime   = 0.0f;
        m_frameCount  = 0;
        m_fps         = 0.0f;
        m_fpsAccum    = 0.0f;
        m_fpsFrames   = 0;
    }

    // Call once per frame at the start of the loop
    void Tick() {
        TimePoint now = Clock::now();
        auto duration = std::chrono::duration<float>(now - m_lastFrame);
        m_deltaTime   = duration.count();

        // Clamp delta time to avoid huge spikes (e.g. on breakpoints)
        if (m_deltaTime > m_maxDeltaTime) m_deltaTime = m_maxDeltaTime;

        m_lastFrame  = now;
        m_totalTime += m_deltaTime;
        ++m_frameCount;

        // FPS calculation (averaged over 1 second)
        m_fpsAccum  += m_deltaTime;
        ++m_fpsFrames;
        if (m_fpsAccum >= 1.0f) {
            m_fps       = static_cast<float>(m_fpsFrames) / m_fpsAccum;
            m_fpsAccum  = 0.0f;
            m_fpsFrames = 0;
        }
    }

    float DeltaTime()  const { return m_deltaTime; }
    float TotalTime()  const { return m_totalTime; }
    float FPS()        const { return m_fps; }
    int   FrameCount() const { return m_frameCount; }

    void SetMaxDeltaTime(float max) { m_maxDeltaTime = max; }
    float MaxDeltaTime() const { return m_maxDeltaTime; }

    // Fixed timestep utilities
    float FixedDeltaTime() const { return m_fixedDeltaTime; }
    void  SetFixedDeltaTime(float dt) { m_fixedDeltaTime = dt; }

    // Time since engine start in seconds
    float TimeSinceStart() const {
        auto duration = std::chrono::duration<float>(Clock::now() - m_startTime);
        return duration.count();
    }

private:
    Time()
        : m_deltaTime(0.016f), m_totalTime(0.0f), m_frameCount(0),
          m_fps(0.0f), m_fpsAccum(0.0f), m_fpsFrames(0),
          m_maxDeltaTime(0.1f), m_fixedDeltaTime(1.0f / 60.0f)
    {}

    TimePoint m_startTime;
    TimePoint m_lastFrame;
    float     m_deltaTime;
    float     m_totalTime;
    int       m_frameCount;
    float     m_fps;
    float     m_fpsAccum;
    int       m_fpsFrames;
    float     m_maxDeltaTime;
    float     m_fixedDeltaTime;
};

} // namespace Nova
