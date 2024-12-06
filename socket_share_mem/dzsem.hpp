#ifndef __DZSEM_HPP__
#define __DZSEM_HPP__
#include <iostream>
#include <vector>
#include <chrono>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

////////////////////////////////////////
//          此处配置共享内存
////////////////////////////////////////
#define share_mem_size 4000  // 共享内存长度
#define select_msec_or_sec 0 // 选择阻塞时间单位，1为秒，0为毫秒
#define block_tim 100        // 阻塞时间,注意，毫秒等待不能超过1000，若超过1000，会自动转换为秒，使用前配置

namespace dz_communicate
{
    using namespace boost::interprocess;
    typedef named_semaphore sem_t; /* 进程信号量 */

    /**
     * @brief 共享内存结构体(循环链表)
     */

    typedef struct share_zone
    {
        int data_size;             /* 数据长度 */
        char data[share_mem_size]; /* 数据 */
        size_t next_offset;        /* 下一个节点 */
        bool has_write;            /* 是否有已写操作 */
        bool has_read;             /* 是否有已读操作 */
    } share_zone, *SZ;

    class dzsem
    {
    public:
        dzsem() {}
        ~dzsem()
        {
            delete H2L_wr;
            delete L2H_wr;
            delete L2H_rd;
            delete H2L_rd;
            if (this->is_server)
            {
                sem_t::remove(sem_name.c_str());
                sem_t::remove(sem_name1.c_str());
                sem_t::remove(sem_name2.c_str());
                sem_t::remove(sem_name3.c_str());
            }
        }
        void init(bool is_server, key_t key)
        {
            this->is_server = is_server;
            sem_name = "dzsem_H2L_wr_" + std::to_string(key);
            sem_name1 = "dzsem_L2H_wr_" + std::to_string(key);
            sem_name2 = "dzsem_H2L_rd_" + std::to_string(key);
            sem_name3 = "dzsem_L2H_rd_" + std::to_string(key);
            ready_sem_name = "dzsem_ready_" + std::to_string(key);
            if (is_server)
            {
                try
                {
                    named_semaphore::remove(ready_sem_name.c_str());
                }
                catch (interprocess_exception &ex)
                {
                }
                ready_sem = new sem_t(open_or_create, ready_sem_name.c_str(), 0);
                try
                {
                    named_semaphore::remove(sem_name.c_str());
                }
                catch (interprocess_exception &ex)
                {
                }
                H2L_wr = new sem_t(open_or_create, sem_name.c_str(), 0);
                try
                {
                    named_semaphore::remove(sem_name1.c_str());
                }
                catch (interprocess_exception &ex)
                {
                }
                L2H_wr = new sem_t(open_or_create, sem_name1.c_str(), 0);
                try
                {
                    named_semaphore::remove(sem_name2.c_str());
                }
                catch (interprocess_exception &ex)
                {
                }
                H2L_rd = new sem_t(open_or_create, sem_name2.c_str(), 0);
                try
                {
                    named_semaphore::remove(sem_name3.c_str());
                }
                catch (interprocess_exception &ex)
                {
                }
                L2H_rd = new sem_t(open_or_create, sem_name3.c_str(), 0);
            }
            else
            {
                H2L_wr = new sem_t(open_only, sem_name.c_str());
                L2H_wr = new sem_t(open_only, sem_name1.c_str());
                H2L_rd = new sem_t(open_only, sem_name2.c_str());
                L2H_rd = new sem_t(open_only, sem_name3.c_str());
                ready_sem = new sem_t(open_only, ready_sem_name.c_str());
            }
        }
        void ready()
        {
            if (this->is_server)
            {
                ready_sem->wait();
            }
            else
            {
                ready_sem->post();
            }
        }
        /**
         * @brief 信号量等待
         */
        void wr_wait()
        {
            if (!is_server) /* 客户端 */
            {
                H2L_wr->wait();
            }
            else
            {
                L2H_wr->wait();
            }
        }
        void rd_wait()
        {
            if (!is_server) /* 客户端 */
            {
                H2L_rd->wait();
            }
            else
            {
                L2H_rd->wait();
            }
        }
        /**
         * @brief 信号量定时等待，输入时间单位为毫秒
         * @param time
         * @return
         */
        int wr_time_wait(long time)
        {

            if (!is_server)
            {
                long time_cache = time;
                long timesec = 0;
                while (time_cache >= 1000)
                {
                    time_cache -= 1000;
                    timesec++;
                }
                boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
                boost::posix_time::ptime ts = now + boost::posix_time::milliseconds(time_cache) + boost::posix_time::seconds(timesec);
                bool result = H2L_wr->timed_wait(ts);
                if (!result)
                {
                    return -1;
                }
            }
            else
            {
                long time_cache = time;
                long timesec = 0;
                while (time_cache >= 1000)
                {
                    time_cache -= 1000;
                    timesec++;
                }
                boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
                boost::posix_time::ptime ts = now + boost::posix_time::milliseconds(time_cache) + boost::posix_time::seconds(timesec);
                bool result = L2H_wr->timed_wait(ts);
                if (!result)
                {
                    return -1;
                }
            }
            return 0;
        }
        int rd_time_wait(long time)
        {

            if (!is_server)
            {
                long time_cache = time;
                long timesec = 0;
                while (time_cache >= 1000)
                {
                    time_cache -= 1000;
                    timesec++;
                }
                boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
                boost::posix_time::ptime ts = now + boost::posix_time::milliseconds(time_cache) + boost::posix_time::seconds(timesec);
                bool result = H2L_rd->timed_wait(ts);
                if (!result)
                {
                    return -1;
                }
            }
            else
            {
                long time_cache = time;
                long timesec = 0;
                while (time_cache >= 1000)
                {
                    time_cache -= 1000;
                    timesec++;
                }
                boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
                boost::posix_time::ptime ts = now + boost::posix_time::milliseconds(time_cache) + boost::posix_time::seconds(timesec);
                bool result = L2H_rd->timed_wait(ts);
                if (!result)
                {
                    return -1;
                }
            }
            return 0;
        }
        void wr_post()
        {
            if (is_server)
            {
                H2L_wr->post();
            }
            else
            {
                L2H_wr->post();
            }
        }
        void rd_post()
        {
            if (is_server)
            {
                H2L_rd->post();
            }
            else
            {
                L2H_rd->post();
            }
        }

    private:
        std::string sem_name;
        std::string sem_name1;
        std::string sem_name2;
        std::string sem_name3;
        std::string ready_sem_name;
        sem_t *H2L_wr;
        sem_t *L2H_wr;
        sem_t *H2L_rd;
        sem_t *L2H_rd;
        sem_t *ready_sem;
        bool is_server = true;
    };

    inline int sem_time_wait(long time, sem_t *sem)
    {
        long time_cache = time;
        long timesec = 0;
        while (time_cache >= 1000)
        {
            time_cache -= 1000;
            timesec++;
        }
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
        boost::posix_time::ptime ts = now + boost::posix_time::milliseconds(time_cache) + boost::posix_time::seconds(timesec);
        bool result = sem->timed_wait(ts);
        if (!result)
        {
            return -1;
        }
        return 0;
    }

    class dzshm_queue
    {
    public:
        dzshm_queue() {};
        ~dzshm_queue();
        void config_by_init_shm(int array_size, bool H_or_L, int key);
        void config(int array_size, bool H_or_L, int key);
        bool push(char *data, int size);
        bool pop(char **data, int &size);

    private:
        void get_next_pos();
        std::vector<std::string> shm_name;
        std::vector<mapped_region> regions_cache;
        std::vector<shared_memory_object *> shm_ptr_cache;
        std::vector<share_zone *> shm_chain_ptr_cache;
        bool H_or_L;
        bool config_flag = false;
        int queue_size;
        int current_pos = 0;
    };

}
#endif