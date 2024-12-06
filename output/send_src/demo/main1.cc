#include "dzcommunicator_top.h"
#include <iostream>
#include <thread>
#include <chrono>
#define TIME_DIFF_MS(t0, t1) ((t1.tv_sec + t1.tv_nsec * 1e-9) - (t0.tv_sec + t0.tv_nsec * 1e-9)) * 1e3
/**************************************************************
 * @brief: This is a test for dz_communicate
 * @brief：
 **************************************************************/
key_t key = 8141;
int Port = 8721;
#if 1
int main()
{
    dz_hc com;
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
        com.send(B);
        clock_gettime(CLOCK_MONOTONIC, &cal_time1);
        // std::cout << "sm_tran Time taken: " << TIME_DIFF_MS(cal_time0, cal_time1) << " microseconds" << std::endl;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
}
#endif
#if 0
dz_communicate::dz_com com1;
void threadFunction2()
{
    // com1.init(low_level, key);
    // com1.init(low_level, key, server_mode);
    com1.init(isclient, "127.0.0.1", Port, server_mode);
    Eigen::MatrixXd A(10, 1);
    Eigen::MatrixXd B(11, 1);
    timespec cal_time0, cal_time1, next_time;
    double time = 0;
    int cnt = 0;
    A << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &next_time);
        next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
        next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
        for (int i = 0; i < 10; i++)
        {
            B(i + 1) = sin(A(i) * time);
        }
        B(0) = time;
        time += 0.001;
        clock_gettime(CLOCK_MONOTONIC, &cal_time0);
        com1.write(B, socket_tran);
        // com1.write(B, sm_tran);
        clock_gettime(CLOCK_MONOTONIC, &cal_time1);

        std::cout << "sm_tran Time taken: " << TIME_DIFF_MS(cal_time0, cal_time1) << " microseconds" << std::endl;
        // std::cout << "B is:" << B.transpose() << std::endl;
        cnt++;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
}
int main()
{
    std::thread t2(threadFunction2);
    t2.join();
}
#endif
#if 0
dz_communicate::dz_com com1;
void threadFunction2()
{
    // com1.init(low_level, key);
    com1.init(isclient, "127.0.0.1", Port, low_level, key, server_mode);
    Eigen::MatrixXd A, B;
    for (int i = 0; i < 10; i++)
    {
        A = com1.read(sm_tran);
        std::cout << "Thread 1 received: \n"
                  << A << std::endl;
        B = com1.read(socket_tran);
        std::cout << "Thread 2 received: \n"
                  << B << std::endl;
    }
    for (int i = 0; i < 10; i++)
    {
        com1.write(A, sm_tran);
        com1.write(B, socket_tran);
    }
}
int main()
{
    std::thread t2(threadFunction2);
    t2.join();
}
#endif
#if 0
void threadFunction2()
{
    usleep(1000);
    dz_communicate::dzc serval;
    // serval.dz_communicator_init(low_level, key);
    serval.dz_communicator_init(isclient, "127.0.0.1", Port, low_level, key);
    // /* socket */
    auto start = std::chrono::high_resolution_clock::now();
    double *data = serval.read<double>(issocket);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Thread 2 execution time read: "
              << elapsed.count() * 1000 << " mseconds" << std::endl;
    Eigen::MatrixXd B = Eigen::Map<Eigen::MatrixXd>(data, 10, 10);
    std::cout << "Thread 2 received: \n"
              << B << std::endl;
    /* sm */
    start = std::chrono::high_resolution_clock::now();
    int a = 10;
    for (int i = 0; i < 100; i++)
    {
        serval.write(&a, sizeof(int), issm);
        serval.write(B.data(), sizeof(B.data()) * B.size(), issm);
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "Thread 2 execution time write: " << elapsed.count() * 1000 << " mseconds" << std::endl;
    serval.close();
}
int main()
{
    std::thread t2(threadFunction2);
    t2.join();
    return 0;
}
#endif
#if 0
int main()
{
    dz_communicate::dzc serval;
    serval.dz_communicator_init(isclient, "127.0.0.1", Port);
    // /* socket */
    char *data = serval.read<char>(issocket);
    std::cout << "Thread 2 received: \n"
              << data << std::endl;
}
#endif