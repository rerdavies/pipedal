#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <pthread.h>
#include <sched.h>

namespace pipedal
{
    class CacheMissCounter
    {
    private:
        int fd_l1;
        int fd_l2;
        bool affinity_set = false;
        cpu_set_t original_cpu_set;

        static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                                    int cpu, int group_fd, unsigned long flags)
        {
            return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
        }

        void setup_counter(int &fd, uint64_t config, const std::string &name)
        {
            struct perf_event_attr pe;
            memset(&pe, 0, sizeof(struct perf_event_attr));
            pe.type = PERF_TYPE_HW_CACHE;
            pe.size = sizeof(struct perf_event_attr);
            pe.config = config;
            pe.disabled = 1;
            pe.exclude_kernel = 1;
            pe.exclude_hv = 1;

            fd = perf_event_open(&pe, 0, -1, -1, 0);
        }

    public:
        CacheMissCounter() : fd_l1(-1), fd_l2(-1)
        {
            uint64_t l1_config = (PERF_COUNT_HW_CACHE_L1D << 0) |
                                 (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                                 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
            uint64_t l2_config = (PERF_COUNT_HW_CACHE_LL << 0) |
                                 (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                                 (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

            setup_counter(fd_l1, l1_config, "L1");
            setup_counter(fd_l2, l2_config, "L2");

            int32_t cpu = 1;
            if (cpu >= 0)
            {
                if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &original_cpu_set) != 0)
                {
                    throw std::runtime_error("Failed to get thread affinity");
                }

                cpu_set_t cpu_set;
                CPU_ZERO(&cpu_set);
                CPU_SET(cpu, &cpu_set);
                if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set) != 0)
                {
                    throw std::runtime_error("Failed to set thread affinity");
                }
                affinity_set = true;
            }
        }

        ~CacheMissCounter()
        {
            if (fd_l1 != -1)
                close(fd_l1);
            if (fd_l2 != -1)
                close(fd_l2);
            if (affinity_set)
            {
                pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &original_cpu_set);
            }
        }

        void start()
        {
            ioctl(fd_l1, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd_l2, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd_l1, PERF_EVENT_IOC_ENABLE, 0);
            ioctl(fd_l2, PERF_EVENT_IOC_ENABLE, 0);
        }

        void stop()
        {
            ioctl(fd_l1, PERF_EVENT_IOC_DISABLE, 0);
            ioctl(fd_l2, PERF_EVENT_IOC_DISABLE, 0);
        }

        long long get_l1_misses()
        {
            long long count;
            read(fd_l1, &count, sizeof(long long));
            return count;
        }

        long long get_l2_misses()
        {
            if (fd_l2 == -1)
            {
                return -1;
            }
            long long count;
            read(fd_l2, &count, sizeof(long long));
            return count;
        }
    };

    // // Example usage
    // int main() {
    //     try {
    //         CacheMissCounter counter;

    //         counter.start();

    //         // Your code to measure goes here
    //         // For example, let's do some memory operations
    //         volatile int* array = new int[1000000];
    //         for (int i = 0; i < 1000000; i++) {
    //             array[i] = i;
    //         }
    //         delete[] array;

    //         counter.stop();

    //         std::cout << "L1 cache misses: " << counter.get_l1_misses() << std::endl;
    //         std::cout << "L2 cache misses: " << counter.get_l2_misses() << std::endl;

    //     } catch (const std::exception& e) {
    //         std::cerr << "Error: " << e.what() << std::endl;
    //         return 1;
    //     }

    //     return 0;
    // }
}