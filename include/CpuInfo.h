#pragma once

#include <cstddef>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <unistd.h>
#endif

class CpuInfo
{
public:
    // 获取逻辑线程数（std::thread::hardware_concurrency 的包装）
    static size_t LogicalThreads()
    {
        size_t n = std::thread::hardware_concurrency();
        return n > 0 ? n : 1;
    }

    // 获取物理核心数
    static size_t PhysicalCores()
    {
#if defined(_WIN32) || defined(_WIN64)
        DWORD len = 0;
        GetLogicalProcessorInformation(nullptr, &len);
        if (len == 0)
            return LogicalThreads();

        DWORD count = len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer =
            new SYSTEM_LOGICAL_PROCESSOR_INFORMATION[count];
        if (!GetLogicalProcessorInformation(buffer, &len))
        {
            delete[] buffer;
            return LogicalThreads();
        }

        size_t cores = 0;
        for (DWORD i = 0; i < count; ++i)
        {
            if (buffer[i].Relationship == RelationProcessorCore)
                ++cores;
        }

        delete[] buffer;
        return cores > 0 ? cores : LogicalThreads();

#elif defined(__linux__)
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        return n > 0 ? (size_t) n : LogicalThreads();
#elif defined(__APPLE__) && defined(__MACH__)
        int nm[2];
        size_t len = 4;
        uint32_t count;
        nm[0] = CTL_HW;
        nm[1] = HW_PHYSICALCPU;
        if (sysctl(nm, 2, &count, &len, nullptr, 0) == 0)
            return (size_t) count;
        return LogicalThreads();
#else
        return LogicalThreads(); // fallback
#endif
    }
};
