#ifndef DZ_SM_SOCKET_TOP_QUEUE_HPP
#define DZ_SM_SOCKET_TOP_QUEUE_HPP
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <vector>
namespace dz_communicate
{
#define block_tim 100
    typedef boost::interprocess::interprocess_semaphore xsem_t; /* 线程信号量 */
    class dz_Info
    {
    public:
        dz_Info() : has_read(true), has_write(false), data(NULL), data_size(0), next(NULL) {}
        ~dz_Info() { free(data); }
        bool has_read;  /* 读完成标志 */
        bool has_write; /* 写完成标志 */
        char *data;     // 数据
        int data_size;  // 数据大小
        dz_Info *next;  // 指向下一个节点
    };

    class InfoCycleQueue
    {
    public:
        /**
         * @brief 构造函数
         * @param size  队列大小
         * @param ser_or_topic 选择服务模式还是话题模式，true为服务模式，false为话题模式，话题模式不阻塞，服务模式阻塞
         */
        InfoCycleQueue(int size, bool ser_or_topic)
        {
            isservice = ser_or_topic;
            if (size <= 0)
            {
                std::cerr << "InfoCycleQueue size must be greater than 0" << std::endl;
                throw std::runtime_error("An error occurred.");
            }
            info_vec.resize(size);
            this->size = size;
            for (int i = 0; i < size; i++)
            {
                info_vec[i] = new dz_Info();
                if (i == 0)
                {
                    wr_head = info_vec[i];
                    rd_head = info_vec[i];
                }
                else if (i == size - 1)
                {
                    info_vec[i]->next = info_vec[0];
                    info_vec[i - 1]->next = info_vec[i];
                }
                else
                {
                    info_vec[i - 1]->next = info_vec[i];
                }
            }
            if (ser_or_topic)
            {
                wr_sem = new xsem_t(0);
                rd_sem = new xsem_t(0);
            }
        }
        ~InfoCycleQueue()
        {
            for (int i = 0; i < size; i++)
            {
                delete info_vec[i];
            }
        };
        int push(char **data, int size)
        {
            int cnt = 0;
            /* 前方未读 */
            while (!wr_head->has_read)
            {
                /* 一秒无反应自动断开 */
                if (cnt >= 10)
                {
                    return -1;
                }
                if (!isservice)
                {
                    return -1;
                }
                else
                {
                    wr_is_block = true;
                    this->semTimeWait(block_tim, wr_sem);
                    cnt++;
                    continue;
                }
            }
            wr_is_block = false;
            if (wr_head->data != NULL)
                free(wr_head->data);
            wr_head->data = (char *)malloc(size);
            memcpy(wr_head->data, *data, size);
            wr_head->data_size = size;
            wr_head->has_write = true;
            wr_head->has_read = false;
            wr_head = wr_head->next;
            if (rd_is_block)
                rd_sem->post(); // 唤醒读线程
            return 0;
        }
        int pop(char **data, int &size)
        {
            int cnt = 0;
            while (!rd_head->has_write)
            {
                /* 一秒无反应自动断开 */
                if (cnt >= 10)
                {
                    return -1;
                }
                if (!isservice)
                {
                    size = 0;
                    return -1;
                }
                else
                {
                    rd_is_block = true;
                    this->semTimeWait(block_tim, rd_sem);
                    cnt++;
                    continue;
                }
            }
            rd_is_block = false;
            *data = (char *)malloc(rd_head->data_size);
            memcpy(*data, rd_head->data, rd_head->data_size);
            size = rd_head->data_size;
            rd_head->has_write = false;
            rd_head->has_read = true;
            rd_head = rd_head->next;
            if (wr_is_block)
                wr_sem->post(); // 唤醒写线程
            return 0;
        }

    private:
        int semTimeWait(long ms, xsem_t *sem)
        {
            long time_cache = ms;
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
        std::vector<dz_Info *> info_vec;
        dz_Info *rd_head;
        dz_Info *wr_head;
        int size;
        xsem_t *wr_sem;
        xsem_t *rd_sem;
        bool isservice; // 是否为服务模式
        bool wr_is_block = false;
        bool rd_is_block = false;
    };
#undef block_tim
}
#endif
