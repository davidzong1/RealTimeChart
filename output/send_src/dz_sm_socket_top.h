#ifndef __DZ_SM_SOCKET_TOP_H
#define __DZ_SM_SOCKET_TOP_H
#include "dz_sm_socket.h"
#include "dz_sm_socket_top_queue.hpp"
#include <thread>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>
#define EigenOn

#ifdef EigenOn
#include <Eigen/Dense>
#endif
#define socket_tran true
#define sm_tran false
#define server_mode true
#define topic_mode false

namespace dz_communicate
{
#define queue_size 10
        class dz_com
        {
        public:
                dz_com() {};
                ~dz_com();
                dz_com(bool isServer, const char *IP, int PORT, bool ser_or_topic) { this->init(isServer, IP, PORT, ser_or_topic); };
                dz_com(bool isServer, const char *IP, int PORT, bool H_or_L, key_t key, bool ser_or_topic) { this->init(isServer, IP, PORT, H_or_L, key, ser_or_topic); };
                dz_com(bool H_or_L, key_t key, bool ser_or_topic) { this->init(H_or_L, key, ser_or_topic); };
                void init(bool isServer, const char *IP, int PORT, bool ser_or_topic);
                void init(bool isServer, const char *IP, int PORT, bool H_or_L, key_t key, bool ser_or_topic);
                void init(bool H_or_L, key_t key, bool ser_or_topic);
                template <typename T>
                int write(T *data, int size, bool socket_or_sm);
                template <typename T>
                std::pair<T *, int> read(bool socket_or_sm);
#ifdef EigenOn
                void write(Eigen::MatrixXd &data, bool socket_or_sm);
                Eigen::MatrixXd read(bool socket_or_sm);
#endif

        protected:
                bool socket_read_update = false;
                bool sm_read_update = false;
                bool sm_write_ready_flag = true;
                bool socket_write_ready_flag = true;
                bool block_read_flag = false;
                std::condition_variable run_cv;
                bool ser_or_topic;
                InfoCycleQueue *socket_pub_queue;
                InfoCycleQueue *socket_sub_queue;
                InfoCycleQueue *sm_write_queue;
                InfoCycleQueue *sm_read_queue;

                char *topic_socket_sub_data_cache = nullptr;
                char *topic_socket_pub_data_cache = nullptr;
                char *topic_sm_sub_data_cache = nullptr;
                char *topic_sm_pub_data_cache = nullptr;
                int topic_socket_sub_data_cache_size = 0;
                int topic_socket_pub_data_cache_size = 0;
                int topic_sm_sub_data_cache_size = 0;
                int topic_sm_pub_data_cache_size = 0;

        private:
                friend void thread_function_read(dz_com *obj, dz_communicator *dzc);  // 读取线程函数
                friend void thread_function_write(dz_com *obj, dz_communicator *dzc); // 写入线程函数
                friend void thread_function_pub(dz_com *obj, dz_communicator *dzc);   // 发布线程函数
                friend void thread_function_sub(dz_com *obj, dz_communicator *dzc);   // 订阅线程函数
                bool Init_flag = false;
                dz_communicator *dzc_read;
                dz_communicator *dzc_write;
                std::thread *thrd_sub;
                std::thread *thrd_pub;
                std::thread *thrd_write;
                std::thread *thrd_read;
                bool socket_on = false;
                bool sm_on = false;
                bool pub_shutdown_flag = false;
                bool sub_shutdown_flag = false;
                bool write_shutdown_flag = false;
                bool read_shutdown_flag = false;
        };
}
#endif