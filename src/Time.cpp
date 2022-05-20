#include <engine/util/Time.hpp>

namespace en
{
    std::chrono::time_point<std::chrono::high_resolution_clock> Time::m_Last = std::chrono::high_resolution_clock::now();
    double Time::m_DeltaTime = 0.0;

    void Time::Update()
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
        std::chrono::nanoseconds delta = now - m_Last;
        m_Last = now;
        m_DeltaTime = (double)delta.count() / 1000000000.0;
    }

    double Time::GetDeltaTime()
    {
        return m_DeltaTime;
    }

    uint32_t Time::GetFps()
    {
        double invDeltaTime = 1.0 / m_DeltaTime;
        return static_cast<uint32_t>(invDeltaTime);
    }

    long Time::GetTimeStamp()
    {
        auto epoch = m_Last.time_since_epoch();
        std::chrono::microseconds value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
        return value.count();
    }
}
