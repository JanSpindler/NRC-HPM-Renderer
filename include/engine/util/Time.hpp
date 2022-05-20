#pragma once

#include <chrono>

namespace en
{
    class Time
    {
    public:
        static void Update();
        static double GetDeltaTime();
        static uint32_t GetFps();
        static uint64_t GetTimeStamp();

    private:
        static std::chrono::time_point<std::chrono::high_resolution_clock> m_Last;
        static double m_DeltaTime;
    };
}
