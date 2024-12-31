#include "dzcommunicator_top.h"
#include <chrono>
#include <iostream>
#include <thread>
#define TIME_DIFF_MS(t0, t1) ((t1.tv_sec + t1.tv_nsec * 1e-9) - (t0.tv_sec + t0.tv_nsec * 1e-9)) * 1e3
/**************************************************************
 * @brief: This is a test for dz_communicate offline
 * @brief：离线模式演示
 **************************************************************/
key_t key = 8141;
int Port = 8721;
#if 1
int main()
{
    dz_hc com(false);
    double time = 0;
    timespec cal_time0, cal_time1, next_time;
    Eigen::VectorXd A(10, 1);
    Eigen::VectorXd B(10, 1);
    A << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &next_time);
        next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
        next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
        for (int i = 0; i < 10; i++)
        {
            B(i) = sin(A(i) * time);
        }
        time += 0.001;
        clock_gettime(CLOCK_MONOTONIC, &cal_time0);
        com.send("wbc", B);
        com.send("MPC", B);
        clock_gettime(CLOCK_MONOTONIC, &cal_time1);
        std::cout << "sm_tran Time taken: " << TIME_DIFF_MS(cal_time0, cal_time1) << " microseconds" << std::endl;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
    std::cout << "end" << std::endl;
}
#endif
/**************************************************************
 * @brief: This is a test for dz_communicate online
 * @brief：在线模式演示
 **************************************************************/
#if 0
int main()
{
    dz_hc com(true);
    double time = 0;
    timespec cal_time0, cal_time1, next_time;
    Eigen::VectorXd A(10, 1);
    Eigen::VectorXd B(10, 1);
    A << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &next_time);
        next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
        next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
        for (int i = 0; i < 10; i++)
        {
            B(i) = sin(A(i) * time);
        }
        time += 0.001;
        clock_gettime(CLOCK_MONOTONIC, &cal_time0);
        com.send("wbc", B);
        com.send("MPC", B);
        com.set_ready();
        clock_gettime(CLOCK_MONOTONIC, &cal_time1);
        // std::cout << "sm_tran Time taken: " << TIME_DIFF_MS(cal_time0, cal_time1) << " microseconds" << std::endl;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
}
#endif
