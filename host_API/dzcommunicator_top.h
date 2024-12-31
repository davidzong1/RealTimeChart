#pragma once
#include "dz_sm_socket_top.h"
#include <Eigen/Dense>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <queue>
#include <sstream>
#include <vector>
using namespace dz_communicate;
std::unordered_map<std::string, SSMData> data_cache;
std::mutex file_mutex;
#define socket_or_sm false
#define IP "127.0.0.1"
#define PORT 8854
#define share_memory_key 8848
#define save_path "/home/oem/dz_log/"
class dz_hc;                        // 声明dz_hc类
static dz_hc *g_instance = nullptr; // 全局实例指针
/**
 * 使用说明：
 * -- 1. 分别修改下面的宏定义，配置使用socket还是共享内存，以及IP和端口
 * -- 2. socket_or_sm为真时，使用socket，为假时使用共享内存
 * -- 3. IP和PORT为socket的配置，share_memory_key为共享内存的配置
 * -- 4. IP为为实时可视化端的IP地址，非本机地址，配置为127.0.0.1时，实时可视化端和本机在同一台机器上
 */

class Compressor
{
private:
    static std::string SavePath;
    static std::mutex print_mutex;

    static std::string get_current_time()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm *now_tm = std::localtime(&now_time);
        char buffer[9];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", now_tm);
        return std::string(buffer);
    }

    static std::string get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm *now_tm = std::localtime(&now_time);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", now_tm);
        return std::string(buffer);
    }

    static void compress_function()
    {
        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "\033[34m[" << get_current_time() << "] Compressing data do not operate!!!!!!!\033[0m" << std::endl;
        }

        bool csv_exists = false;
        for (const auto &entry : std::filesystem::directory_iterator(SavePath))
        {
            if (entry.path().extension() == ".csv")
            {
                csv_exists = true;
                break;
            }
        }

        if (csv_exists)
        {
            std::string timestamp = get_timestamp();
            std::string zip_command = "zip -j " + SavePath + "/data_" + timestamp + ".zip " + SavePath + "/*.csv";
            std::system(zip_command.c_str());

            for (const auto &entry : std::filesystem::directory_iterator(SavePath))
            {
                if (entry.path().extension() == ".csv")
                {
                    std::filesystem::remove(entry.path());
                }
            }

            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "\033[34m"
                      << "*************************************************************************\n"
                      << "-----------------------------Compress finished-----------------------------\n"
                      << "*************************************************************************\033[0m" << std::endl;
        }
        std::quick_exit(0);
    }

public:
    static void start(const std::string &path)
    {
        SavePath = path;
        std::thread(compress_function).detach();
    }

    // 禁用构造函数
    Compressor() = delete;
};

// 类外部初始化静态成员
std::string Compressor::SavePath;
std::mutex Compressor::print_mutex;

class dz_hc
{
public:
    dz_hc(bool online_or_offline_, std::string save_path_ = save_path)
    {
        g_instance = this; // 保存实例指针
        exit_flag = 0;
        online_or_offline = online_or_offline_;
        if (!online_or_offline)
        {
            SavePath = save_path_;
            // 注册信号处理程序
            std::signal(SIGINT, dz_hc::signalHandler); // 注册Ctrl+C信号处理程序
            // 递归判断 SavePath 路径文件夹是否存在，若不存在则创建
            if (!std::filesystem::exists(SavePath))
            {
                std::filesystem::create_directories(SavePath);
            }
        }
        // 在构造函数中启动线程
        if (online_or_offline_)
            worker_thread = std::thread(&dz_hc::thread_function_online, this);
        else
            worker_thread = std::thread(&dz_hc::thread_function_offline, this);
    }
    ~dz_hc()
    {
        exit_flag = 1;
        worker_thread.join();
    }

private:
    static void signalHandler(int signum)
    {
        std::string path = SavePath;
        if (!online_or_offline)
        {
            Compressor::start(path); // 创建压缩线程
        }
        if (g_instance)
        {
            dz_hc *temp = g_instance;
            g_instance = nullptr; // 先置空，防止重入
            delete temp;          // 再删除
        }
        std::quick_exit(0); // 主程序直接退出
    }

public:
    void set_ready()
    {
        ready = true;
    }
    void send(std::string name, Eigen::VectorXd &data)
    {
        SSMData cache;
        data_mutex.lock();
        cache.name = name;
        cache.data = data;
        cache.time = time_stamp;
        auto it = name_to_index.find(name);
        if (it == name_to_index.end())
        {
            // 如果不存在，添加新的数据
            name_to_index[name] = data_cache.size();
            data_cache.push_back(cache);
            /* 离线传输，即日志模式 */
            if (!online_or_offline)
            {
                std::string filename = SavePath + name + std::string(".csv");
                file.emplace_back(); // 创建新的文件流;
                file.back().open(filename, std::ios::out | std::ios::trunc);
            }
        }
        else
        {
            // 如果存在，更新现有数据
            data_cache[it->second] = cache;
        }
        if (!ready)
            ready = true;
        data_mutex.unlock();
    }

private:
    static volatile sig_atomic_t exit_flag; // 全局变量，用于控制线程的退出
    /* 通信相关 */
    static bool online_or_offline; /* 静态化成员 */
    static std::string SavePath;
    std::vector<std::ofstream> file;
    std::mutex data_mutex;
    timespec next_time;
    bool ready = false;
    std::thread worker_thread;
    dz_communicate::dz_com com;
    double time_stamp = 0.0;
    std::vector<SSMData> data_cache;
    std::map<std::string, size_t> name_to_index; // 用于存储变量名和对应索引的映射

    void thread_function_online()
    {
        std::vector<SSMData> data_cache1;
        if (socket_or_sm)
            com.init(isclient, IP, PORT, server_mode);
        else
            com.init(low_level, share_memory_key, server_mode);
        std::cout << "\033[34m"
                  << "*************************************************************************\n"
                  << "*************************************************************************\n"
                  << "------------------DZ real time visual chart init success-----------------\n"
                  << "*************************************************************************\n"
                  << "*************************************************************************\033[0m" << std::endl;
        while (true)
        {
            clock_gettime(CLOCK_MONOTONIC, &next_time);
            if (exit_flag)
                break;
            /* 更新时间 */
            next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
            next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
            if (ready)
            {
                /* 更新数据 */
                data_mutex.lock();
                data_cache1 = data_cache;
                data_mutex.unlock();
                /* 发送数据 */
                com.squewrite(data_cache1, socket_or_sm);
                /* 更新时间戳 */
                time_stamp += 0.001;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(this->next_time), NULL);
        }
    }
    /* 不关闭文件在线更新 */
    void writeDataNoClose(std::ofstream &file, const SSMData &data, long long cnt)
    {
        if (file.is_open())
        {
            // 写入前检查文件状态
            if (!file.good())
            {
                std::cerr << "\033[31mFile stream not in good state before writing\033[0m" << std::endl;
                return;
            }
            file << std::to_string(data.time) << " " << vectorToString(data.data) << std::endl;
            file.flush(); // 强制刷新缓冲区
            // 写入后检查文件状态
            if (!file.good())
            {
                std::cerr << "\033[31mError writing to file: ";
                if (file.fail())
                    std::cerr << "formatting error ";
                if (file.bad())
                    std::cerr << "fatal error ";
                if (file.eof())
                    std::cerr << "unexpected EOF ";
                std::cerr << "\033[0m" << std::endl;
            }
        }
        else
        {
            std::cerr << "\033[31mFile stream not open before writing\033[0m" << std::endl;
        }
    }

    void thread_function_offline()
    {
        long long cnt = 0;
        std::vector<SSMData> data_cache1;
        std::cout << "\033[34m"
                  << "*************************************************************************\n"
                  << "*************************************************************************\n"
                  << "----------------------------DZ log init success--------------------------\n"
                  << "*************************************************************************\n"
                  << "*************************************************************************\033[0m" << std::endl;
        while (1)
        {
            clock_gettime(CLOCK_MONOTONIC, &next_time);
            if (exit_flag)
                break;
            /* 更新时间 */
            next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
            next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
            if (ready)
            {
                data_mutex.lock();
                data_cache1 = data_cache;
                data_mutex.unlock();
                for (int i = 0; i < file.size(); i++)
                {
                    writeDataNoClose(file[i], data_cache1[i], cnt);
                }
                /* 更新时间戳 */
                time_stamp += 0.001;
                cnt++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(this->next_time), NULL);
        }
        std::cout << std::endl;
        for (int i = 0; i < file.size(); i++)
        {
            if (file[i].is_open())
            {
                // 确保所有数据都写入磁盘
                file[i].flush();

                // 检查文件状态
                if (!file[i].good())
                {
                    std::cerr << "\033[31mFile " << i << " has errors before closing\033[0m" << std::endl;
                }

                // 关闭文件
                file[i].close();
                // 验证文件是否创建
                std::string filename = SavePath + data_cache[i].name + ".csv";
                if (!std::filesystem::exists(filename))
                {
                    std::cerr << "\033[31mFile not created: " << filename << "\033[0m" << std::endl;
                }
            }
        }
        // 获取当前时间并格式化为字符串
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm *now_tm = std::localtime(&now_time);
        std::ostringstream oss;
        oss << std::put_time(now_tm, "%Y%m%d_%H%M%S");
        std::string nowdate = oss.str();
        // 检查是否存在 .csv 文件
        bool csv_exists = false;
        for (const auto &entry : std::filesystem::directory_iterator(SavePath))
        {
            if (entry.path().extension() == ".csv")
            {
                csv_exists = true;
                break;
            }
        }

        if (!csv_exists)
        {
            std::cout << "No .csv files found in " << SavePath << std::endl;
            return;
        }
        // 压缩 SavePath 路径下的所有 .csv 文件到一个 .zip 压缩包
        std::string zip_command = "zip -j " + SavePath + "/data_" + nowdate + ".zip " + SavePath + "/*.csv";
        std::system(zip_command.c_str());

        // 删除所有 .csv 文件
        for (const auto &entry : std::filesystem::directory_iterator(SavePath))
        {
            if (entry.path().extension() == ".csv")
            {
                std::filesystem::remove(entry.path());
            }
        }
    }

    /**
     * @brief 将Eigen::VectorXd转换为字符串
     * @param vec
     * @return
     */
    std::string vectorToString(const Eigen::VectorXd &vec)
    {
        std::ostringstream oss;
        for (int i = 0; i < vec.size(); ++i)
        {
            oss << vec(i);
            if (i < vec.size() - 1)
            {
                oss << " ";
            }
        }
        return oss.str();
    }
    /**
     * @brief 流形式输入矩阵数据
     * @param param_name
     * @param data
     * @return
     */
    std::string formatData(const std::string &param_name, const Eigen::MatrixXd &data)
    {
        std::ostringstream oss;
        oss << param_name << " " << data.rows() << " " << data.cols() << " ";
        for (int i = 0; i < data.rows(); i++)
        {
            for (int j = 0; j < data.cols(); j++)
            {
                oss << data(i, j) << " ";
            }
        }
        return oss.str();
    }
};
// 在类外部初始化静态成员
volatile sig_atomic_t dz_hc::exit_flag = 0;
std::string dz_hc::SavePath = save_path;
bool dz_hc::online_or_offline = false; // true为在线，false为离线

#undef share_memory_key
#undef PORT
#undef IP
#undef save_path