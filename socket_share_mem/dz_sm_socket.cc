#include "dz_sm_socket.h"
/****************************************************************************************
****************************************************************************************
****************************************************************************************
                                套接字相关函数
****************************************************************************************
****************************************************************************************
*****************************************************************************************/

union c2uc
{
    char c;
    unsigned char uc;
};
namespace dz_communicate
{
    dzsocket::socketgo::socketgo(void) {}
    dzsocket::socketgo::~socketgo(void)
    {
        if (isServer)
        {
            close(this->sockServer);
            close(this->server_sockfd);
        }
        else
        {
            close(this->sockClient);
        }
    }

    // socket网络服务器端
    int dzsocket::socketgo::socketConnectNetServer(int PORT)
    {
        int enable = 1;
        this->server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (setsockopt(this->server_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        {
            perror("setsockopt(SO_REUSEADDR) failed");
            close(this->server_sockfd);
            return 1;
        }
        struct sockaddr_in server_sockaddr;
        server_sockaddr.sin_family = AF_INET;
        server_sockaddr.sin_port = htons(PORT);
        server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(this->server_sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1)
        {
            perror("bind");
            return -1;
        }

        if (listen(this->server_sockfd, 5) == -1)
        {
            perror("listen");
            return -1;
        }
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        this->sockServer = accept(this->server_sockfd, (struct sockaddr *)&client_addr, &length);
        if (this->sockServer < 0)
        {
            perror("connect");
            return -1;
        }
        else
        {
            return 0;
        }
    }
    // socket网络客户端
    int dzsocket::socketgo::socketConnectNetClient(int PORT, const char *IP)
    {
        struct sockaddr_in servaddr;
        if ((this->sockClient = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
#if Debug
            printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
#endif
            return -1;
        }
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(PORT);
        if (inet_pton(AF_INET, IP, &servaddr.sin_addr) <= 0)
        {
#if Debug
            printf("inet_pton error for %s\n", IP);
#endif
            return -1;
        }
        while (1)
        {
            if (connect(this->sockClient, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                if (errno != 111)
                {
#if Debug
                    printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
#endif
                    close(this->sockClient);
                    return -1;
                }
                else
                {
                    usleep(100000);
                    continue;
                }
            }
            else
            {
                return 0;
            }
        }
    }

    bool dzsocket::socketgo::getIsServer()
    {
        return isServer;
    }
    // socket网络连接
    /*
        * 函数功能：socket网络连接
        * 函数参数：PORT:端口号，IP:IP地址，isServer:是否为服务器
                    本地时使用本机IP地址
        * 函数返回：正常返回0，异常返回-1

    */

    int dzsocket::socketgo::socketConnectNet(bool isServer, const char *IP, int PORT)
    {
        this->isServer = isServer;
        if (this->isServer == isserver)
            return this->socketConnectNetServer(PORT);
        else
        {
            usleep(10000);
            return this->socketConnectNetClient(PORT, IP);
        }
    }
    /*************************************
     * 函数功能：关闭socket连接
     * 函数参数：socket_num:socket连接号
     * 函数返回：无
     **************************************/
    void dzsocket::socketgo::socketDisconnect(socket_num socket_num)
    {
        close(socket_num);
    }

    /*************************************
     * 函数功能：发送数据
     * 函数参数：Data:发送数据的指针，data_size:发送数据的大小，sockConn:socket连接号
     * 函数返回：正常返回0，异常返回-1
     **************************************/
    int dzsocket::socketgo::transmit(char *Data, int data_size, socket_num sockConn)
    {
        // 单次发送最大数据量
        const int max_size = 1024;
        char *data = (char *)malloc(data_size);
        memcpy(data, Data, data_size);
        // 分包记数
        int sent_byte_cnt = 0;
        while (sent_byte_cnt < data_size)
        {
            int size_to_send = data_size - sent_byte_cnt;
            if (size_to_send > max_size)
            {
                size_to_send = max_size;
            }

            int result = send(sockConn, data + sent_byte_cnt, size_to_send, 0);
            if (result < 0)
            {
#if Debug
                printf("Send Data Failed! : %s\n", strerror(errno));
#endif
                free(data);
                return -1;
            }

            sent_byte_cnt += result;
        }

        switch (waitack(sockConn))
        {
        case 0:
            free(data);
            return 0;
        case -1:
            free(data);
#if Debug
            printf("Stop!\n");
#endif
            return -1;
        default:
            free(data);
#if Debug
            printf("ACK format Error! \n");
#endif
            return -1;
        }
    }
    /*************************************
     * 函数功能：接收数据
     * 函数参数：Data:接收数据的指针，data_size:接收数据的大小，sockConn:socket连接号
     * 函数返回：正常返回0，异常返回-1
     **************************************/
    int dzsocket::socketgo::receive(char **Data, int data_size, socket_num sockConn)
    {
        // 单次接受最大数据量
        const int max_size = 1024;
        char *data = (char *)malloc(data_size);
        if (data == NULL)
        {
#if Debug
            printf("Receive data size configuration error!\n");
            printf("Memory allocation failed!\n");
#endif
            return -1;
        }
        if (!socket_rev_config)
        {
            rev_timeout.tv_sec = 0;
            rev_timeout.tv_usec = 100000;                                                     /* 定时为100ms */
            setsockopt(sockConn, SOL_SOCKET, SO_RCVTIMEO, &rev_timeout, sizeof(rev_timeout)); /* 设置度超时 */
        }
        int returnflag = 0;         // 用于指示接收是否成功的标志
        memset(data, 0, data_size); // 初始化一个名为data的结构体，假设是用于存储接收数据的全局或类成员变量
        int rec_byte_cnt = 0;
        int rec_byte_cnt_cache = 0;
        while (rec_byte_cnt < data_size)
        {
            rec_byte_cnt_cache = data_size - rec_byte_cnt;
            if (rec_byte_cnt_cache > max_size)
            {
                rec_byte_cnt_cache = max_size;
            }
            returnflag = recv(sockConn, (char *)(data + rec_byte_cnt),
                              data_size - rec_byte_cnt, 0); // 接收数据
            if (returnflag <= 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return -2;
                }
                else
                {
                    // perror("recv failed");
                    return -1;
                }
            }
            rec_byte_cnt += returnflag;
        }
        skack(sockConn);
        memcpy(*Data, data, data_size);
        free(data);
        return 0;
    }

    void dzsocket::socketgo::skack(socket_num sockConn)
    {
        char ack[3] = {'A', 'C', 'K'};
        if (send(sockConn, (char *)(ack), 3 * sizeof(char), 0) < 0)
        {
#if Debug
            printf("Server Send Ack Failed!\n");
#endif
        }
    }

    void dzsocket::socketgo::skerror(socket_num sockConn)
    {
        char ack[3] = {'E', 'R', 'R'};
        if (send(sockConn, (char *)(ack), 3 * sizeof(char), 0) < 0)
        {
#if Debug
            printf("Server Send Ack Failed!\n");
#endif
        }
    }

    int dzsocket::socketgo::waitack(socket_num sockConn)
    {
        char ack[3];
        if (recv(sockConn, (char *)(ack), 3 * sizeof(char), 0) < 0)
        {
#if Debug
            printf("Wait ack fail.\n");
#endif
            return -1;
        }
        if (strcmp(ack, "ACK") == 0)
            return 0;
        else
            return 1;
    }

    /****************************************************************************************
    ****************************************************************************************
    ****************************************************************************************
                                    共享内存相关函数
    ****************************************************************************************
    ****************************************************************************************
    *****************************************************************************************/
    /**
     * @brief 初始化共享内存
     * @param H2L_data_size 高端发送到低端的共享内存大小
     * @param L2H_data_size 低端发送到高端的共享内存大小
     * @param H_or_L        1表示高端，0表示低端
     * @param key           IPC键值
     * @return              smp* 共享内存结构体指针
     */
    int smp::share_mem_init(bool H_or_L, key_t key)
    {
        HL_ID = H_or_L;
        shm_name_Con = "DZSM_Con" + std::to_string(key);
        shm_Con = nullptr;
        // 如果为共享内存创建者则初始化标志位
        if (H_or_L == high_level)
        {
            try
            {
                shm_Con = new shared_memory_object(open_only, shm_name_Con.c_str(), read_write);
            }
            catch (const interprocess_exception &e)
            {
                delete shm_Con;
                shm_Con = new shared_memory_object(create_only, shm_name_Con.c_str(), read_write);
                // 分配共享内存大小
                shm_Con->truncate(sizeof(sm_init));
            }
        }
        else
        {
            try
            {
                shm_Con = new shared_memory_object(open_only, shm_name_Con.c_str(), read_write);
            }
            catch (const interprocess_exception &e)
            {
                if (e.get_error_code() == not_found_error)
                {
                    return -1; // 共享内存不存在
                }
            }
        }
        // 将共享内存映射到进程地址空间
        region_Con = new mapped_region(*shm_Con, read_write);
        sm_init_ptr = static_cast<sm_init *>(region_Con->get_address());
        // std::string hsem = "DZsem_H2L" + std::to_string(key);
        // std::string lsem = "DZsem_L2H" + std::to_string(key);
        // H2L_sem_Con = new sem_t(open_or_create, hsem.c_str(), 0);
        // L2H_sem_Con = new sem_t(open_or_create, lsem.c_str(), 0);
        if (H_or_L)
        {
            H2L_queue.config_by_init_shm(queue_changdu, H_or_L, key);
            L2H_queue.config_by_init_shm(queue_changdu, H_or_L, key + 1);
            /* 初始化信号量结构体 */
            share_sem.init(H_or_L, key); // 初始化信号量
            sm_init_ptr->H2L_wrbusy = false;
            sm_init_ptr->L2H_wrbusy = false;
            sm_init_ptr->H2L_rdbusy = false;
            sm_init_ptr->L2H_rdbusy = false;
            sm_init_ptr->Init_flag = true;
        }
        else
        {
            /* 等待高层就绪 */
            while (!sm_init_ptr->Init_flag)
            {
                usleep(1000);
            }
            H2L_queue.config(queue_changdu, H_or_L, key);
            L2H_queue.config(queue_changdu, H_or_L, key + 1);
            share_sem.init(H_or_L, key); // 初始化信号量
            sm_init_ptr->Init_flag = false;
        }
        share_sem.ready();
        return 0;
    }

    void smp::share_mem_destroy()
    {
        if (HL_ID == high_level)
        {
            shared_memory_object::remove(shm_name_Con.c_str());
            // delete H2L_sem_Con;
            // delete L2H_sem_Con;
        }
    }

    int smp::SM_write(char *cache, int size)
    {
        if (this->HL_ID == high_level)
        {
            /* push失败，队列已满 */
            while (!H2L_queue.push(cache, size))
            {
                sm_init_ptr->H2L_wrbusy = true;
                share_sem.wr_time_wait(1);
            }
            sm_init_ptr->H2L_wrbusy = false;
            if (sm_init_ptr->L2H_rdbusy)
            {
                share_sem.rd_post();
            }
        }
        else
        {
            while (!L2H_queue.push(cache, size))
            {
                sm_init_ptr->L2H_wrbusy = true;
                share_sem.wr_time_wait(1);
            }
            sm_init_ptr->L2H_wrbusy = false;
            if (sm_init_ptr->H2L_rdbusy)
            {
                share_sem.rd_post();
            }
        }
        return 0;
    }

    int smp::SM_read(char **cache, int &data_size)
    {
        if (this->HL_ID == high_level)
        {
            while (!L2H_queue.pop(cache, data_size))
            {
                sm_init_ptr->H2L_rdbusy = true;
                share_sem.rd_time_wait(1);
                return -1;
            }
            sm_init_ptr->H2L_rdbusy = false;
            if (sm_init_ptr->L2H_wrbusy)
            {
                share_sem.wr_post();
            }
        }
        else
        {
            while (!H2L_queue.pop(cache, data_size))
            {
                sm_init_ptr->L2H_rdbusy = true;
                share_sem.rd_time_wait(1);
                return -1;
            }
            sm_init_ptr->L2H_rdbusy = false;
            if (sm_init_ptr->H2L_wrbusy)
            {
                share_sem.wr_post();
            }
        }
        return 0;
    }

    /****************************************************************************************
    ****************************************************************************************
    ****************************************************************************************
                                    外部接口相关函数
    ****************************************************************************************
    ****************************************************************************************
    *****************************************************************************************/
    int dz_communicator::dz_communicator_init(bool isServer, const char *IP, int PORT)
    {
        if (isServer == isclient)
        {
            usleep(500000);
        }
        mysocket = new dzsocket::socketgo;
        do
        {
            if (mysocket->socketConnectNet(isServer, IP, PORT))
            {
                sleep(1);
            }
            else
            {
                socket_enable = true;
            }
            socket_enable = true;
        } while (!socket_enable);
        return 0;
    }
    int dz_communicator::dz_communicator_init(bool isServer, const char *IP, int PORT, bool H_or_L, key_t key)
    {
        int init_flag;
        my_share_mem = new smp;
        if (H_or_L == low_level)
        {
            usleep(1000);
        }
        key_t key_cache = key;
// 初始化共享内存
#if Debug
        std::cout << "\033[34mInit share memory ptr\033[0m" << std::endl;
#endif
        init_flag = my_share_mem->share_mem_init(H_or_L, key);
        if (init_flag == -1)
        {
            return -1;
        }
        share_mem_enable = true;
        if (isServer == isclient)
        {
            usleep(500000);
        }
        mysocket = new dzsocket::socketgo;
        do
        {
            if (mysocket->socketConnectNet(isServer, IP, PORT))
            {
#if Debug
                std::cout << "connect failed" << std::endl;
#endif
                sleep(1);
            }
            else
            {
#if Debug
                std::cout << "connect success" << std::endl;
#endif
                socket_enable = true;
            }
            socket_enable = true;
        } while (!socket_enable);
        return 0;
    }

    int dz_communicator::dz_communicator_init(bool H_or_L, key_t key)
    {
        my_share_mem = new smp;
        key_t key_cache = key;
// 初始化共享内存
#if Debug
        std::cout << "\033[34mInit share memory ptr\033[0m" << std::endl;
#endif
        /* 等待信号量初始化（高层初始化的） */
        while (my_share_mem->share_mem_init(H_or_L, key) == -1)
        {
            usleep(100000);
        }
        share_mem_enable = true;
        return 0;
    }
    ////////////////////////////////////////////////////////////////////////////////////
    // 套接字
    ///////////////////////////////////////////////////////////////////////////////////
    int dz_communicator::transmit_char(char *Data, int data_size)
    {
        if (!socket_enable)
        {
#if Debug
            std::cout << "socket is not enable !" << std::endl;
#endif
            return -1;
        }
        if (mysocket->getIsServer() == isserver)
            return mysocket->transmit(Data, data_size, mysocket->sockServer);
        else
            return mysocket->transmit(Data, data_size, mysocket->sockClient);
    }

    int dz_communicator::receive_char(char **Data, int data_size)
    {
        if (!socket_enable)
        {
#if Debug
            std::cout << "socket is not enable !" << std::endl;
#endif
            return -1;
        }
        if (mysocket->getIsServer() == isserver)
            return mysocket->receive(Data, data_size, mysocket->sockServer);
        else
            return mysocket->receive(Data, data_size, mysocket->sockClient);
    }

    void dz_communicator::socketDisconnect()
    {
        if (!socket_enable)
        {
#if Debug
            std::cout << "socket is not enable !" << std::endl;
#endif
            return;
        }
        if (mysocket->getIsServer() == isserver)
            mysocket->socketDisconnect(mysocket->sockServer);
        else
            mysocket->socketDisconnect(mysocket->sockClient);
    }

    template <typename T>
    int dz_communicator::write(T *data, int size_, bool socket_or_sm)
    {
        if (socket_or_sm == issocket)
        {
            return transmit(data, size_);
        }
        else
        {
            return SM_write(data, size_);
        }
    }
#define DZ_TOP_WRITE_FOR_TYPE(T) \
    template int dz_communicator::write<T>(T * data, int size_, bool socket_or_sm);
    // 使用宏定义来实例化不同类型的 write 函数
    DZ_TOP_WRITE_FOR_TYPE(int)
    DZ_TOP_WRITE_FOR_TYPE(float)
    DZ_TOP_WRITE_FOR_TYPE(double)
    DZ_TOP_WRITE_FOR_TYPE(char)
    DZ_TOP_WRITE_FOR_TYPE(unsigned char)
    DZ_TOP_WRITE_FOR_TYPE(unsigned int)
    DZ_TOP_WRITE_FOR_TYPE(unsigned short)
    DZ_TOP_WRITE_FOR_TYPE(short)
    DZ_TOP_WRITE_FOR_TYPE(long)
    DZ_TOP_WRITE_FOR_TYPE(long long)
    DZ_TOP_WRITE_FOR_TYPE(unsigned long long)
    DZ_TOP_WRITE_FOR_TYPE(bool)
#undef DZ_TOP_WRITE_FOR_TYPE

    template <typename T>
    T *dz_communicator::read(bool socket_or_sm)
    {
        if (socket_or_sm == issocket)
        {
            return receive<T>();
        }
        else
        {
            return SM_read<T>();
        }
    }
#define DZ_TOP_READ_FOR_TYPE(T) \
    template T *dz_communicator::read<T>(bool socket_or_sm);
    // 使用宏定义来实例化不同类型的 read 函数
    DZ_TOP_READ_FOR_TYPE(int)
    DZ_TOP_READ_FOR_TYPE(float)
    DZ_TOP_READ_FOR_TYPE(double)
    DZ_TOP_READ_FOR_TYPE(char)
    DZ_TOP_READ_FOR_TYPE(unsigned char)
    DZ_TOP_READ_FOR_TYPE(unsigned int)
    DZ_TOP_READ_FOR_TYPE(unsigned short)
    DZ_TOP_READ_FOR_TYPE(short)
    DZ_TOP_READ_FOR_TYPE(long)
    DZ_TOP_READ_FOR_TYPE(long long)
    DZ_TOP_READ_FOR_TYPE(unsigned long long)
    DZ_TOP_READ_FOR_TYPE(bool)
#undef DZ_TOP_READ_FOR_TYPE
    /**
     * @brief 套接字发送数据
     * @tparam T
     * @param data
     * @param size
     * @return
     */
    template <typename T>
    int dz_communicator::transmit(T *data, int size)
    {
        int sizesize = sizeof(int);
        char *bytesize = new char[sizesize];
        memcpy(bytesize, &size, sizesize);
        if (transmit_char(bytesize, sizesize))
        {
#if Debug
            std::cout << "Socket size Transmit Time out!" << std::endl;
#endif
            return -1;
        }
        char *bytes = reinterpret_cast<char *>(data);
        if (transmit_char(bytes, size))
        {
#if Debug
            std::cout << "Socket Data Transmit Time out!" << std::endl;
#endif
            return -1;
        };
        delete[] bytesize;
        return 0;
    }
#define DZ_TRANSMIT_FOR_TYPE(T) \
    template int dz_communicator::transmit<T>(T * data, int size);
    // 使用宏定义来实例化不同类型的 transmit 函数
    DZ_TRANSMIT_FOR_TYPE(int)
    DZ_TRANSMIT_FOR_TYPE(float)
    DZ_TRANSMIT_FOR_TYPE(double)
    DZ_TRANSMIT_FOR_TYPE(char)
    DZ_TRANSMIT_FOR_TYPE(unsigned char)
    DZ_TRANSMIT_FOR_TYPE(unsigned int)
    DZ_TRANSMIT_FOR_TYPE(unsigned short)
    DZ_TRANSMIT_FOR_TYPE(short)
    DZ_TRANSMIT_FOR_TYPE(long)
    DZ_TRANSMIT_FOR_TYPE(long long)
    DZ_TRANSMIT_FOR_TYPE(unsigned long long)
    DZ_TRANSMIT_FOR_TYPE(bool)
#undef DZ_TRANSMIT_FOR_TYPE
    /**
     * @brief 接收数据,接收定时100ms
     * @tparam T
     * @return
     */
    template <typename T>
    T *dz_communicator::receive()
    {
        int sizesize = sizeof(int);
        char *size_cache = new char[sizesize];
        int rd_flag = receive_char(&size_cache, sizesize);
        if (rd_flag)
        {
            if (rd_flag == -1)
#if Debug
                std::cout << "Socket size receive fail!" << std::endl;
#endif
            return nullptr;
        }
        int dataSize = *reinterpret_cast<int *>(size_cache);
        char *data = new char[dataSize];
        rd_flag = receive_char(&data, dataSize);
        if (rd_flag)
        {
            if (rd_flag == -1)
#if Debug
                std::cout << "Socket Data receive fail!" << std::endl;
#endif
            return nullptr;
        }
        T *cache = reinterpret_cast<T *>(data);
        return cache;
    }
#define DZ_RECEIVE_FOR_TYPE(T) \
    template T *dz_communicator::receive<T>();
    // 使用宏定义来实例化不同类型的 receive 函数
    DZ_RECEIVE_FOR_TYPE(int)
    DZ_RECEIVE_FOR_TYPE(float)
    DZ_RECEIVE_FOR_TYPE(double)
    DZ_RECEIVE_FOR_TYPE(char)
    DZ_RECEIVE_FOR_TYPE(unsigned char)
    DZ_RECEIVE_FOR_TYPE(unsigned int)
    DZ_RECEIVE_FOR_TYPE(unsigned short)
    DZ_RECEIVE_FOR_TYPE(short)
    DZ_RECEIVE_FOR_TYPE(long)
    DZ_RECEIVE_FOR_TYPE(long long)
    DZ_RECEIVE_FOR_TYPE(unsigned long long)
    DZ_RECEIVE_FOR_TYPE(bool)
#undef DZ_RECEIVE_FOR_TYPE
    /**
     * @brief 读数据到共享内存
     * @tparam T
     * @return
     */
    template <typename T>
    T *dz_communicator::SM_read()
    {
        int result;
        if (!share_mem_enable)
        {
            std::cout << "share memory is not enable !" << std::endl;
            return nullptr;
        }
        char *size_cache = new char[sizeof(int)];
        int size;
        if (my_share_mem->SM_read(&size_cache, size))
        {
            return nullptr;
        }
        size = *reinterpret_cast<int *>(size_cache);
        if (size <= 0)
        {
            std::cerr << "size is <= 0,which is " << size << std::endl;
            throw std::runtime_error("An error occurred.");
        }
        else if (size > 1e10)
        {
            std::cerr << "size is too big,which is " << size << std::endl;
            throw std::runtime_error("An error occurred.");
        }
        char *cache = new char[size];
        char *cache_ptr = cache;
        int cache_size; /* 缓冲区大小 */
        result = 0;
        while (1)
        {
            if (size / share_mem_size != 0)
            {
                while (my_share_mem->SM_read(&cache_ptr, cache_size))
                {
                    /* 等待十秒退出 */
                    if (result >= 10000)
                    {
#if Debug
                        std::cout << "SM_read time out! Which addr is " << (int)(cache_ptr - cache_ptr) << std::endl;
#endif
                        return nullptr;
                    }
                    else
                    {
                        result++;
                    }
                }
                if (cache_size != share_mem_size)
                {
#if Debug
                    std::cerr << "cache_size is not equal to share_mem_size,which is " << cache_size << std::endl;
#endif
                }
                result = 0;
                cache_ptr += cache_size;
                size -= cache_size;
            }
            else
            {
                if (size == 0)
                {
                    break;
                }
                else
                {
                    while (my_share_mem->SM_read(&cache_ptr, cache_size))
                    {
                        if (result >= 10000)
                        {
#if Debug
                            std::cout << "SM_read time out! Which addr is " << (int)(cache_ptr - cache_ptr) << std::endl;
#endif
                            return nullptr;
                        }
                        else
                        {
                            result++;
                        }
                    }
                    if (cache_size != size)
                    {
#if Debug
                        std::cerr << "cache_size is not equal to share_mem_size,which cache_size is " << cache_size << " ,which size is " << size << std::endl;
#endif
                    }
                    break;
                }
            }
        }
        T *data = reinterpret_cast<T *>(cache);
        return data;
    }
#define DZ_SM_READ_FOR_TYPE(T) \
    template T *dz_communicator::SM_read<T>();
    // 使用宏定义来实例化不同类型的 SM_read 函数
    DZ_SM_READ_FOR_TYPE(int)
    DZ_SM_READ_FOR_TYPE(float)
    DZ_SM_READ_FOR_TYPE(double)
    DZ_SM_READ_FOR_TYPE(char)
    DZ_SM_READ_FOR_TYPE(unsigned char)
    DZ_SM_READ_FOR_TYPE(unsigned int)
    DZ_SM_READ_FOR_TYPE(unsigned short)
    DZ_SM_READ_FOR_TYPE(short)
    DZ_SM_READ_FOR_TYPE(long)
    DZ_SM_READ_FOR_TYPE(long long)
    DZ_SM_READ_FOR_TYPE(unsigned long long)
    DZ_SM_READ_FOR_TYPE(bool)
#undef DZ_SM_READ_FOR_TYPE
    /**
     * @brief 写数据到共享内存
     * @tparam T
     * @param data
     * @param size_
     * @return
     */
    template <typename T>
    int dz_communicator::SM_write(T *data, int size_)
    {
        int size = size_;
        int success_flag;
        if (size <= 0)
        {
            std::cerr << "size is <= 0,which is " << size << std::endl;
            throw std::runtime_error("An error occurred.");
        }
        if (!share_mem_enable)
        {
            std::cout << "share memory is not enable !" << std::endl;
            return -1;
        }
        char *size_cache = reinterpret_cast<char *>(&size);
        success_flag = my_share_mem->SM_write(size_cache, sizeof(int));
        if (success_flag == -1)
        {
#if Debug
            std::cout << "SM_write Size time out! Which addr is " << std::endl;
#endif
            return -1;
        }
        char *cache = reinterpret_cast<char *>(data);
        char *cache_ptr = cache;
        int cnt = 0;
        while (1)
        {
            if (size / share_mem_size != 0)
            {

                success_flag = my_share_mem->SM_write(cache_ptr, share_mem_size);
                if (success_flag == -1)
                {
#if Debug
                    std::cout << "SM_write time out! Which addr is " << (int)(cache_ptr - cache_ptr) << std::endl;
#endif
                    return -1;
                }
                cache_ptr += share_mem_size;
                size -= share_mem_size; /* 计数减少 */
            }
            else
            {
                if (size == 0)
                {
                    break;
                }
                else
                {
                    my_share_mem->SM_write(cache_ptr, size);
                    if (success_flag == -1)
                        return -1;
                    break;
                }
            }
            cnt++;
        }
        return 0;
    }
#define DZ_SM_WRITE_FOR_TYPE(T) \
    template int dz_communicator::SM_write<T>(T * data, int size_);
    // 使用宏定义来实例化不同类型的 SM_write 函数
    DZ_SM_WRITE_FOR_TYPE(int)
    DZ_SM_WRITE_FOR_TYPE(float)
    DZ_SM_WRITE_FOR_TYPE(double)
    DZ_SM_WRITE_FOR_TYPE(char)
    DZ_SM_WRITE_FOR_TYPE(unsigned char)
    DZ_SM_WRITE_FOR_TYPE(unsigned int)
    DZ_SM_WRITE_FOR_TYPE(unsigned short)
    DZ_SM_WRITE_FOR_TYPE(short)
    DZ_SM_WRITE_FOR_TYPE(long)
    DZ_SM_WRITE_FOR_TYPE(long long)
    DZ_SM_WRITE_FOR_TYPE(unsigned long long)
    DZ_SM_WRITE_FOR_TYPE(bool)
#undef DZ_SM_WRITE_FOR_TYPE
}
