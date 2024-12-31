#ifndef __DZ_SM_SOCKET_H__
#define __DZ_SM_SOCKET_H__

#define Debug false

typedef int socket_num;

#include "dzsem.hpp" // 引入信号量头文件
#define BOOST_ASIO_DISABLE_STD_CHRONO
#include <boost/asio.hpp>
#undef BOOST_ASIO_DISABLE_STD_CHRONO
#define isserver true
#define isclient false

#define issocket true
#define issm false
#define queue_changdu 10
////////////////////////////////////////
//          此处无需配置
////////////////////////////////////////

#define high_level true // 高层，不用动
#define low_level false // 低层，不用动

/**
 * @brief
 */
namespace dz_communicate
{
    ///////////////////////////////////////////////////////////////
    //                      套接字通信模块
    ///////////////////////////////////////////////////////////////
    namespace dzsocket
    {
        class socketgo
        {
        public:
            socketgo(void);
            ~socketgo(void);
            // 网络客户描述符
            socket_num sockClient;
            // 网络服务端描述符
            socket_num sockServer;

        private:
            void skerror(socket_num sockConn);
            void skack(socket_num sockConn);
            int waitack(socket_num sockConn);
            int socketConnectNetClient(int PORT, const char *IP);
            int socketConnectNetServer(int PORT);
            // 监听描述符(服务端临时创建用于监听的)
            socket_num server_sockfd;
            bool isServer;
            bool socket_rev_config = false;
            struct timeval rev_timeout; /* 接收定时器 */
        public:
            // 打开socket连接
            // params :	PORT	传输端口
            // return : -1		连接失败
            //			0		连接成功
            bool getIsServer();
            int socketConnectNet(bool isServer, const char *IP, int PORT);
            // return : -1		接收失败
            //			0		接收成功
            int transmit(char *Data, int data_size, socket_num sockConn);
            int receive(char **Data, int data_size, socket_num sockConn);
            // 断开socket连接
            void socketDisconnect(socket_num sockConn);
        };
    }

    /******************************************************************
     ******************************************************************
     ******************************************************************
     *                    循环队列无锁共享内存模块
     ******************************************************************
     ******************************************************************
     ******************************************************************/
    /**
     * @brief 初始化用的结构体
     */
    typedef struct
    {
        bool H2L_wrbusy;
        bool L2H_wrbusy;
        bool H2L_rdbusy;
        bool L2H_rdbusy;
        bool Init_flag;
    } sm_init, *SI;
    typedef boost::lockfree::spsc_queue<share_zone, boost::lockfree::capacity<10>> dzqueue; // 队列
    /**
     * @brief 共享内存操作类
     */
    class smp
    {
    public:
        smp() {}
        ~smp() { share_mem_destroy(); }
        // 共享身份
        bool HL_ID;
        // 共享内存指针
        dzshm_queue H2L_queue;
        dzshm_queue L2H_queue;
        // 共享内存操作
        sm_init *sm_init_ptr;
        std::string shm_name_Con;
        /* 共享信号量 */
        mapped_region *region_Con;
        shared_memory_object *shm_Con;
        // sem_t *H2L_sem_Con;
        // sem_t *L2H_sem_Con;
        dzsem share_sem;
        /* 共享内存操作 */
        int share_mem_init(bool H_or_L, key_t key);
        void share_mem_destroy();
        int SM_write(char *cache, int size);
        int SM_read(char **cache, int &data_size);

    private:
    };

    /**
     * @brief socket共享内存集成通信类
     */
    typedef class dz_communicator
    {
    public:
        dz_communicator() {};
        ~dz_communicator() { close(); };
        int dz_communicator_init(bool isServer, const char *IP, int PORT);
        int dz_communicator_init(bool isServer, const char *IP, int PORT, bool H_or_L, key_t key);
        int dz_communicator_init(bool H_or_L, key_t key);
        template <typename T>
        int write(T *data, int size_, bool socket_or_sm);
        template <typename T>
        T *read(bool socket_or_sm);
        void close()
        {
            if (socket_enable)
            {
                socketDisconnect();
                delete mysocket;
                socket_enable = false;
            }
            if (share_mem_enable)
            {
                my_share_mem->share_mem_destroy();
                delete my_share_mem;
                share_mem_enable = false;
            }
        }
        template <typename T>
        int SM_write(T *data, int size_);
        template <typename T>
        T *SM_read();
        void socketDisconnect();
        template <typename T>
        int transmit(T *data, int size);
        template <typename T>
        T *receive();

    private:
        int transmit_char(char *Data, int data_size);
        int receive_char(char **Data, int data_size);
        bool share_mem_enable = false;
        smp *my_share_mem;
        bool socket_enable = false;
        dzsocket::socketgo *mysocket;
    } dzc;
}

#endif
