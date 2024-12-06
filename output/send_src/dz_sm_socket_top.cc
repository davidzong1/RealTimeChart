#include "dz_sm_socket_top.h"

namespace dz_communicate
{
    void thread_function_pub(dz_com *obj, dz_communicator *dzc)
    {
        std::mutex pub_ready_mtx;
        std::unique_lock<std::mutex> lock(pub_ready_mtx);
        obj->run_cv.wait(lock);
        lock.unlock();
        char *pub_cache;
        int pub_cache_size;
        int state = 0;
        int state_cnt = 0;
        int cnt = 0;
        while (1)
        {
            if (obj->pub_shutdown_flag) /* 主线程类析构，子线程退出 */
            {
                return;
            }
            if (state_cnt >= 100)
                state = 0; /* 100次失败，重新获取数据 */
            if (state == 0)
            {
                if (obj->socket_pub_queue->pop(&pub_cache, pub_cache_size))
                    continue;
                state++;
            }
            if (state == 1)
            {
                if (dzc->transmit(&(pub_cache_size), sizeof(int)))
                {
                    state_cnt++;
                    continue;
                }
                state++;
            }
            if (state == 2)
            {
                if (dzc->transmit(pub_cache, pub_cache_size))
                {
                    state_cnt++;
                    continue;
                }
                state = 0;
                state_cnt = 0;
                free(pub_cache);
            }
            cnt++;
        }
    }
    void thread_function_sub(dz_com *obj, dz_communicator *dzc)
    {
        std::mutex sub_ready_mtx;
        std::unique_lock<std::mutex> lock(sub_ready_mtx);
        obj->run_cv.wait(lock);
        lock.unlock();
        int *read_cache_size_ = (int *)malloc(sizeof(int));
        char *read_cache_;
        int state = 0;
        int state_cnt = 0;
        int cnt = 0;
        while (1)
        {
            if (obj->sub_shutdown_flag) /* 主线程类析构，子线程退出 */
            {
                return;
            }
            if (state_cnt >= 100)
                state = 0; /* 100次失败，重新获取数据 */
            if (state == 0)
            {
                read_cache_size_ = dzc->receive<int>();
                if (read_cache_size_ == nullptr)
                {
                    continue;
                }
                state++;
            }
            if (state == 1)
            {
                read_cache_ = dzc->receive<char>();
                if (read_cache_ == nullptr)
                {
                    continue;
                }
                state++;
            }
            if (state == 2)
            {
                if (obj->socket_sub_queue->push(&read_cache_, *read_cache_size_))
                {
                    state_cnt++;
                    continue;
                }
                state = 0;
                state_cnt = 0;
            }
            cnt++;
        }
    }
    void thread_function_write(dz_com *obj, dz_communicator *dzc)
    {
        std::mutex write_ready_mtx;
        std::unique_lock<std::mutex> lock(write_ready_mtx);
        obj->run_cv.wait(lock);
        int state = 0;
        int state_cnt = 0;
        char *write_cache = nullptr;
        int write_cache_size;
        int cnt = 0;
        while (1)
        {
            if (obj->write_shutdown_flag) /* 主线程类析构，子线程退出 */
            {
                return;
            }
            if (state_cnt >= 100)
                state = 0; /* 100次失败，重新获取数据 */
            if (state == 0)
            {
                if (obj->sm_write_queue->pop(&write_cache, write_cache_size))
                {
                    continue;
                }
                state++;
            }
            if (state == 1)
            {
                if (dzc->SM_write(&(write_cache_size), sizeof(int)))
                {
                    state_cnt++;
                    continue;
                }
                state++;
            }
            if (state == 2)
            {
                if (dzc->SM_write(write_cache, write_cache_size))
                {
                    state_cnt++;
                    continue;
                }
                state = 0;
                state_cnt = 0;
                free(write_cache);
                write_cache = nullptr;
            }
            cnt++;
        }
    }
    void thread_function_read(dz_com *obj, dz_communicator *dzc)
    {
        std::mutex read_ready_mtx;
        std::unique_lock<std::mutex> lock(read_ready_mtx);
        obj->run_cv.wait(lock);
        int *read_cache_size_;
        char *read_cache_;
        int state = 0;
        int state_cnt = 0;
        int cnt = 0;
        while (1)
        {
            if (obj->read_shutdown_flag)
            {
                return;
            }
            if (state_cnt >= 100)
                state = 0; /* 100次失败，重新获取数据 */
            if (state == 0)
            {
                read_cache_size_ = dzc->SM_read<int>();
                if (read_cache_size_ == nullptr)
                {
                    continue;
                }
                state++;
            }
            if (state == 1)
            {
                read_cache_ = dzc->SM_read<char>();
                if (read_cache_ == nullptr)
                {
                    continue;
                }
                state++;
            }
            if (state == 2)
            {
                if (obj->sm_read_queue->push(&read_cache_, *read_cache_size_))
                {
                    state_cnt++;
                    continue;
                }
                state = 0;
                state_cnt = 0;
            }
            cnt++;
        }
    }

    void dz_com::init(bool isServer, const char *IP, int PORT, bool ser_or_topic)
    {
        this->ser_or_topic = ser_or_topic;
        if (!Init_flag)
        {
            dzc_read = new dz_communicator;
            dzc_write = new dz_communicator;
            dzc_read->dz_communicator_init(isServer, IP, PORT);
            dzc_write->dz_communicator_init(!isServer, IP, PORT + 1);
            if (isServer == isserver)
            {
                thrd_pub = new std::thread(&thread_function_pub, this, dzc_write);
                thrd_sub = new std::thread(&thread_function_sub, this, dzc_read);
            }
            else
            {
                thrd_pub = new std::thread(&thread_function_pub, this, dzc_read);
                thrd_sub = new std::thread(&thread_function_sub, this, dzc_write);
            }
            socket_on = true;
            socket_pub_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            socket_sub_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            Init_flag = true;
            usleep(500000);
            run_cv.notify_all();
        }
        else
        {
            std::cerr << "\033[31mdz_com::init() already called!!!!!\033[0m" << std::endl;
            return;
        }
    }
    void dz_com::init(bool isServer, const char *IP, int PORT, bool H_or_L, key_t key, bool ser_or_topic)
    {
        this->ser_or_topic = ser_or_topic;
        if (!Init_flag)
        {
            dzc_read = new dz_communicator;
            dzc_write = new dz_communicator;
            dzc_read->dz_communicator_init(isServer, IP, PORT, H_or_L, key);
            dzc_write->dz_communicator_init(!isServer, IP, PORT + 1);
            /* 读写端互换 */
            if (isServer == isserver)
            {
                thrd_pub = new std::thread(&thread_function_pub, this, dzc_write);
                thrd_sub = new std::thread(&thread_function_sub, this, dzc_read);
            }
            else
            {
                thrd_pub = new std::thread(&thread_function_pub, this, dzc_read);
                thrd_sub = new std::thread(&thread_function_sub, this, dzc_write);
            }
            thrd_write = new std::thread(&thread_function_write, this, dzc_read);
            thrd_read = new std::thread(&thread_function_read, this, dzc_read);
            socket_on = true;
            sm_on = true;
            socket_pub_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            socket_sub_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            sm_write_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            sm_read_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            Init_flag = true;
            usleep(500000);
            run_cv.notify_all();
        }
        else
        {
            std::cerr << "\033[31mdz_com::init() already called!!!!!\033[0m" << std::endl;
            return;
        }
    }
    void dz_com::init(bool H_or_L, key_t key, bool ser_or_topic)
    {
        this->ser_or_topic = ser_or_topic;
        if (!Init_flag)
        {
            dzc_read = new dz_communicator;
            dzc_read->dz_communicator_init(H_or_L, key);
            thrd_write = new std::thread(&thread_function_write, this, dzc_read);
            thrd_read = new std::thread(&thread_function_read, this, dzc_read);
            sm_on = true;
            sm_write_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            sm_read_queue = new InfoCycleQueue(queue_size, ser_or_topic);
            Init_flag = true;
            usleep(500000);
            run_cv.notify_all();
        }
        else
        {
            std::cerr << "\033[31mdz_com::init() already called!!!!!\033[0m" << std::endl;
            return;
        }
    }

    template <typename T>
    int dz_com::write(T *data, int size, bool socket_or_sm)
    {
        T *data1 = (T *)malloc(size);
        memcpy(data1, data, size);
        char *write_cache = reinterpret_cast<char *>(data1);
        int cnt = 0;
        if (ser_or_topic)
        {
            while (1)
            {
                /* 10秒失败返回 */
                if (cnt >= 10)
                {
                    return -1;
                }
                if (socket_or_sm == socket_tran)
                {
                    if (socket_pub_queue->push(&write_cache, size))
                    {
                        cnt++;
                        continue;
                    }
                    break;
                }
                else
                {
                    if (sm_write_queue->push(&write_cache, size))
                    {
                        cnt++;
                        continue;
                    }
                    break;
                }
            }
        }
        else
        {
            if (socket_or_sm == socket_tran)
            {
                if (socket_pub_queue->push(&write_cache, size))
                {
                    return 0;
                }
            }
            else
            {
                if (sm_write_queue->push(&write_cache, size))
                {
                    return 0;
                }
            }
        }
        free(data1);
        return 0;
    }
    template <typename T>
    std::pair<T *, int> dz_com::read(bool socket_or_sm)
    {
        T *data;
        char *cache;
        int read_cache_socket_size1;
        int cnt = 0;
        if (ser_or_topic)
        {
            while (1)
            {
                if (cnt >= 1)
                {
                    return std::make_pair(nullptr, 0);
                }
                if (socket_or_sm == socket_tran)
                {
                    if (socket_sub_queue->pop(&cache, read_cache_socket_size1))
                    {
                        cnt++;
                        continue;
                    }
                    break;
                }
                else
                {
                    if (sm_read_queue->pop(&cache, read_cache_socket_size1))
                    {
                        cnt++;
                        continue;
                    }
                    break;
                }
            }
        }
        else
        {
            if (socket_or_sm == socket_tran)
            {
                if (socket_sub_queue->pop(&cache, read_cache_socket_size1))
                {
                    data = (T *)malloc(topic_socket_sub_data_cache_size);
                    memcpy(data, topic_socket_sub_data_cache, topic_socket_sub_data_cache_size);
                    return std::make_pair(data, topic_socket_sub_data_cache_size);
                }
            }
            else
            {
                if (sm_read_queue->pop(&cache, read_cache_socket_size1))
                {
                    data = (T *)malloc(topic_sm_sub_data_cache_size);
                    memcpy(data, topic_sm_sub_data_cache, topic_sm_sub_data_cache_size);
                    return std::make_pair(data, topic_sm_sub_data_cache_size);
                }
            }
        }
        data = (T *)malloc(read_cache_socket_size1);
        memcpy(data, cache, read_cache_socket_size1);
        /* 话题模式提取缓存用于接收失败返回 */
        if (!ser_or_topic)
        {
            if (socket_or_sm == socket_tran)
            {
                free(topic_socket_sub_data_cache);
                topic_socket_sub_data_cache = cache;
                topic_socket_sub_data_cache_size = read_cache_socket_size1;
            }
            else
            {
                free(topic_sm_sub_data_cache);
                topic_sm_sub_data_cache = cache;
                topic_sm_sub_data_cache_size = read_cache_socket_size1;
            }
        }
        free(cache);
        return std::make_pair(data, read_cache_socket_size1);
    }
#ifdef EigenOn
    /**
     * @brief 读取Eigen::MatrixXd类型的数据,建议仅在block_read模式下使用
     * @param data
     * @param socket_or_sm
     */
    void dz_com::write(Eigen::MatrixXd &data, bool socket_or_sm)
    {
        if (!ser_or_topic)
        {
            throw std::runtime_error("Eigen::MatrixXd write is not allowed in topic mode");
        }
        int rows = data.rows();
        int cols = data.cols();
        while (1)
        {
            if (!this->write(&(rows), sizeof(int), socket_or_sm))
                break;
        }
        while (1)
        {
            if (!this->write(&(cols), sizeof(int), socket_or_sm))
                break;
        }
        while (1)
        {
            if (!this->write(data.data(), sizeof(double) * data.size(), socket_or_sm))
                break;
        }
    }
    /**
     * @brief 读取Eigen::MatrixXd类型的数据,建议仅在block_read模式下使用
     * @param socket_or_sm
     * @return
     */
    Eigen::MatrixXd dz_com::read(bool socket_or_sm)
    {
        if (!ser_or_topic)
        {
            throw std::runtime_error("Eigen::MatrixXd read is not allowed in topic mode");
        }
        Eigen::MatrixXd data;
        int size;
        int *row, *col;
        double *cache;
        std::tie(row, size) = this->read<int>(socket_or_sm);
        if (row == nullptr && size == 0)
            return data;
        std::tie(col, size) = this->read<int>(socket_or_sm);
        if (col == nullptr && size == 0)
            return data;
        std::tie(cache, size) = this->read<double>(socket_or_sm);
        if (cache == nullptr && size == 0)
            return data;
        data.resize(*row, *col);
        data = Eigen::Map<Eigen::MatrixXd>(cache, *row, *col);
        free(row);
        free(col);
        free(cache);
        return data;
    }
#endif

    dz_com::~dz_com()
    {
        /* 关闭标识符 */
        pub_shutdown_flag = true;
        sub_shutdown_flag = true;
        write_shutdown_flag = true;
        read_shutdown_flag = true;
        /* 修改阻塞条件变量 */
        if (socket_on)
        {
            thrd_pub->join();
            thrd_sub->join();
            delete thrd_pub;
            delete thrd_sub;
            delete dzc_read;
            delete dzc_write;
        }
        if (sm_on)
        {
            thrd_write->join();
            thrd_read->join();
            delete thrd_write;
            delete thrd_read;
            delete dzc_read;
        }
        /* 释放读写缓存 */
    }
}
