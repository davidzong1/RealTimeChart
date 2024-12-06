#pragma once
#include "dz_sm_socket_top.h"
#include <chrono>
#include <Eigen/Dense>

/**
 * 使用说明：
 * -- 1. 分别修改下面的宏定义，配置使用socket还是共享内存，以及IP和端口
 * -- 2. socket_or_sm为真时，使用socket，为假时使用共享内存
 * -- 3. IP和PORT为socket的配置，share_memory_key为共享内存的配置
 * -- 4. IP为为实时可视化端的IP地址，非本机地址，配置为127.0.0.1时，实时可视化端和本机在同一台机器上
 */
class dz_hc
{
#define socket_or_sm false
#define IP "127.0.0.1"
#define PORT 8854
#define share_memory_key 8848
public:
    dz_hc()
    {
        // 在构造函数中启动线程
        worker_thread = std::thread(&dz_hc::thread_function, this);
    }
    ~dz_hc()
    {
        exit_flag = true;
        worker_thread.join();
    }
    void send(Eigen::VectorXd &data)
    {
        ready = true;
        data_mutex.lock();
        data_cache = data;
        data_mutex.unlock();
    }

private:
    std::mutex data_mutex;
    timespec next_time;
    bool ready = false;
    bool exit_flag = false;
    std::thread worker_thread;
    dz_communicate::dz_com com;
    double time_stamp = 0.0;
    Eigen::VectorXd data_cache;
    void thread_function()
    {
        Eigen::MatrixXd data_cache1;
        if (socket_or_sm)
            com.init(isclient, IP, PORT, server_mode);
        else
            com.init(low_level, share_memory_key, server_mode);
        std::cout << "\033[34m"
                  << "*************************************************************************\n"
                  << "*************************************************************************\n"
                  << "*************************************************************************\n"
                  << "------------------DZ real time visual chart init success-----------------\n"
                  << "*************************************************************************\n"
                  << "*************************************************************************\n"
                  << "*************************************************************************\033[0m" << std::endl;
        while (true)
        {
            clock_gettime(CLOCK_MONOTONIC, &next_time);
            if (exit_flag)
                break;
            next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
            next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
            if (ready)
            {
                if (data_cache1.rows() != (data_cache.size() + 1))
                {
                    data_cache1.resize(data_cache.size() + 1, 1);
                }
                data_cache1(0, 0) = time_stamp;
                for (int i = 0; i < data_cache.size(); i++)
                {
                    data_cache1(i + 1, 0) = data_cache(i);
                }
                com.write(data_cache1, socket_or_sm);
                time_stamp += 0.001;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(this->next_time), NULL);
        }
    }
#undef share_memory_key
#undef PORT
#undef IP
};