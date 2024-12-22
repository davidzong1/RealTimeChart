#include "IOopperator.h"
#include <execution>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <chrono>  // 用于获取当前时间
#include <iomanip> // 用于格式化时间
#include <sstream> // 用于字符串流
using namespace dz_communicate;
namespace fs = std::filesystem;
namespace dz_io
{
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

    void IOoperator::checkPath()
    {
        if (!PATH_SET)
        {
            throw std::runtime_error("An error occurred.");
            return;
        }
    }

    void IOoperator::write(const std::string &param_name, const Eigen::MatrixXd &data)
    {
        checkPath();
        std::string full_path = path + filename;
        // 检查文件是否存在，如果不存在则创建文件
        if (!fs::exists(full_path))
        {
            std::ofstream new_file(full_path);
            if (!new_file.is_open())
            {
                return;
            }
            new_file.close();
        }

        // 读取文件内容
        std::vector<std::string> lines;
        bool param_found = false;
        int param_index = -1;

        if (fs::exists(full_path))
        {
            std::ifstream file(full_path);
            if (!file.is_open())
            {
                return;
            }

            std::string line;
            int index = 0;
            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string firstColumn;
                if (iss >> firstColumn)
                {
                    if (firstColumn == param_name)
                    {
                        param_found = true;
                        param_index = index;
                    }
                }
                lines.push_back(line);
                index++;
            }
            // 如果找到参数，清空对应行并重新写入数据
            if (param_found)
            {
                lines[param_index] = formatData(param_name, data);
            }
            else
            {
                // 如果未找到参数，追加数据
                lines.push_back(formatData(param_name, data));
            }
            file.close();
        }
        // 重新写入文件
        std::ofstream file(full_path);
        if (!file.is_open())
        {
            return;
        }
        for (const auto &line : lines)
        {
            file << line << std::endl;
        }

        file.close();
    }

    Eigen::MatrixXd IOoperator::read(const std::string &paramName)
    {
        checkPath();
        std::ifstream file;
        file.open(path + filename);
        if (!file.is_open())
        {
            throw std::runtime_error("An error occurred.");
        }

        std::string line;
        Eigen::MatrixXd values;
        int rowIndex = -1;
        int currentIndex = 0;

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string firstColumn;
            if (iss >> firstColumn)
            {
                if (firstColumn == paramName)
                {
                    rowIndex = currentIndex;

                    // 读取矩阵的行数和列数
                    int rows, cols;
                    if (!(iss >> rows >> cols))
                    {

                        throw std::runtime_error("An error occurred.");
                    }

                    // 初始化矩阵
                    values = Eigen::MatrixXd(rows, cols);

                    // 读取矩阵数据
                    for (int i = 0; i < rows; ++i)
                    {
                        for (int j = 0; j < cols; ++j)
                        {
                            if (!(iss >> values(i, j)))
                            {
                                throw std::runtime_error("An error occurred.");
                            }
                        }
                    }

                    break;
                }
                currentIndex++;
            }
        }

        if (rowIndex == -1)
        {
            throw std::runtime_error("An error occurred.");
        }

        file.close();
        return values;
    }

    void IOoperator::del(std::string filename)
    {
        checkPath();
        std::string full_path = path + filename;
        if (fs::exists(full_path))
        {
            fs::remove(full_path);
        }
    }
    bool IOoperator::checkParam(const std::string &paramName)
    {
        checkPath();
        std::ifstream file;
        file.open(path + filename);
        if (!file.is_open())
        {
            throw std::runtime_error("An error occurred.");
        }

        std::string line;
        bool param_found = false;
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string firstColumn;
            if (iss >> firstColumn)
            {
                if (firstColumn == paramName)
                {
                    param_found = true;
                    break;
                }
            }
        }
        file.close();
        return param_found;
    }

    bool IOoperator::checkFile(const std::string &filename)
    {
        checkPath();
        std::string full_path = path + filename;
        return fs::exists(full_path);
    }

    Eigen::VectorXd stringToVector(const std::string &str)
    {
        std::istringstream iss(str);
        std::vector<double> values;
        double value;
        while (iss >> value)
        {
            values.push_back(value);
        }

        Eigen::VectorXd vec(values.size());
        for (size_t i = 0; i < values.size(); ++i)
        {
            vec(i) = values[i];
        }

        return vec;
    }

    std::string IOoperator::write(std::vector<std::string> &param_names, std::vector<Eigen::VectorXd> &data)
    {
        checkPath();
        std::string full_path;
        int cnt = 0;
        while (1)
        {
            full_path = path + filename + std::to_string(cnt);
            // 检查文件是否存在，如果不存在则创建文件
            if (!fs::exists(full_path))
            {
                std::ofstream new_file(full_path);
                if (!new_file.is_open())
                {
                    return "";
                }
                new_file.close();
                break;
            }
            else
            {
                cnt++;
            }
        }

        // 重新写入文件
        std::ofstream file(full_path);
        if (!file.is_open())
        {
            return "";
        }
        for (size_t i = 0; i < param_names.size(); i++)
        {
            file << param_names[i] << " " << vectorToString(data[i]) << std::endl;
        }
        file.close();
        return full_path;
    }

    void IOoperator::readAllParams(std::vector<Eigen::VectorXd> &data, std::vector<double> &time)
    {
        checkPath();
        std::ifstream file;
        file.open(path + filename);
        if (!file.is_open())
        {
            return;
        }
        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string firstColumn;
            if (iss >> firstColumn)
            {
                time.push_back(std::stod(firstColumn));
                data.push_back(stringToVector(line.substr(firstColumn.length() + 1))); // 去掉第一个参数名
            }
        }
        file.close();
        return;
    }
    std::string IOoperator::write(std::string &filename_, std::vector<SSMData> &data)
    {
        checkPath();
        std::string full_path;
        int cnt = 0;
        while (1)
        {
            full_path = path + filename_;
            // 检查文件是否存在，如果不存在则创建文件
            if (!fs::exists(full_path))
            {
                std::ofstream new_file(full_path);
                if (!new_file.is_open())
                {
                    return "";
                }
                new_file.close();
                break;
            }
            else
            {
                cnt++;
                usleep(100000);
            }
        }

        // 重新写入文件
        std::ofstream file(full_path);
        if (!file.is_open())
        {
            return "";
        }
        for (size_t i = 0; i < data.size(); i++)
        {
            file << std::to_string(data[i].time) << " " << vectorToString(data[i].data) << std::endl;
        }
        file.close();
        return full_path;
    }

    void IOoperator::read(std::string &filename_, std::vector<SSMData> &data)
    {
        checkPath();
        std::ifstream file;
        file.open(path + filename_);
        if (!file.is_open())
        {
            return;
        }
        std::string line;
        std::string paramname = filename_; // 读取参数名
        SSMData cache;
        // if (std::getline(file, line))
        // {
        //     paramname = line;
        // }
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string firstColumn;
            if (iss >> firstColumn)
            {
                cache.name = paramname;
                cache.time = std::stod(firstColumn);
                cache.data = stringToVector(line.substr(firstColumn.length() + 1)); // 去掉第一个参数名
                data.push_back(cache);
            }
        }
        file.close();
        return;
    }
    /**
     * @brief 将数据写入多个csv文件，并压缩为一个zip文件
     * @param data
     * @return
     */
    std::string IOoperator::Squewrite(std::vector<std::vector<SSMData>> &data)
    {
        std::vector<std::string> filenames; // 存储生成的文件名
        try
        {
            if (data.empty())
            {
                throw std::runtime_error("Empty input data");
            }

            // 预分配空间
            filenames.reserve(data.size());

            // 并行写入文件
            std::mutex filenameMutex;
            std::vector<std::thread> threads;
            const size_t threadCount = std::min(static_cast<size_t>(std::thread::hardware_concurrency()), data.size());
            std::atomic<size_t> dataIndex{0};

            auto writeFiles = [&]()
            {
                while (true)
                {
                    size_t currentIndex = dataIndex++;
                    if (currentIndex >= data.size())
                        break;

                    if (data[currentIndex].empty() || data[currentIndex][0].name.empty())
                    {
                        continue;
                    }

                    std::string filename = data[currentIndex][0].name + ".csv";
                    std::string full_path = path + filename;

                    // 使用 RAII 管理文件资源
                    std::ofstream file(full_path, std::ios::out | std::ios::binary);
                    if (!file)
                        continue;

                    // 使用缓冲区提高写入效率
                    char buffer[8192];
                    file.rdbuf()->pubsetbuf(buffer, sizeof(buffer));

                    // 批量写入数据
                    std::stringstream ss;
                    for (const auto &item : data[currentIndex])
                    {
                        ss << item.time << " " << vectorToString(item.data) << "\n";
                    }
                    file << ss.str();
                    file.close();

                    // 线程安全地添加文件名
                    std::lock_guard<std::mutex> lock(filenameMutex);
                    filenames.push_back(filename);
                }
            };

            // 启动写入线程
            for (size_t i = 0; i < threadCount; ++i)
            {
                threads.emplace_back(writeFiles);
            }

            // 等待所有写入完成
            for (auto &thread : threads)
            {
                thread.join();
            }

            if (filenames.empty())
            {
                throw std::runtime_error("No files were written");
            }

            // 生成唯一的时间戳文件名
            auto now = std::chrono::system_clock::now();
            auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              now.time_since_epoch())
                              .count();
            std::string zipName = "Data_" + std::to_string(now_ns) + ".zip";

            // 构建并执行 zip 命令
            std::stringstream zipCommand;
            zipCommand << "zip -j " << zipName;
            for (const auto &filename : filenames)
            {
                zipCommand << " " << path << filename;
            }

            // 执行压缩
            if (std::system(zipCommand.str().c_str()) != 0)
            {
                throw std::runtime_error("Failed to create zip archive");
            }

            // 使用并行算法删除临时文件
            std::for_each(std::execution::par_unseq,
                          filenames.begin(), filenames.end(),
                          [this](const std::string &filename)
                          {
                              std::filesystem::remove(path + filename);
                          });

            return zipName;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in Squewrite: " << e.what() << std::endl;
            // 清理所有临时文件
            for (const auto &filename : filenames)
            {
                std::filesystem::remove(path + filename);
            }
            throw;
        }
    }
    /**
     * @brief 读取 zip 文件中的数据
     * @param data
     */
    void IOoperator::Squeread(std::vector<std::vector<SSMData>> &data)
    {
        try
        {
            // 解压 zip 文件到临时目录
            std::string tempDir = "temp_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            if (!std::filesystem::create_directory(tempDir))
            {
                throw std::runtime_error("Failed to create temporary directory");
            }

            // 使用 RAII 确保目录清理
            struct DirCleaner
            {
                std::string dir;
                ~DirCleaner()
                {
                    std::filesystem::remove_all(dir);
                }
            } dirCleaner{tempDir};

            // 解压到临时目录
            std::string unzipCommand = "unzip -q " + path + " -d " + tempDir;
            if (std::system(unzipCommand.c_str()) != 0)
            {
                throw std::runtime_error("Failed to unzip file");
            }

            // 并行处理文件
            std::vector<std::vector<SSMData>> result;
            std::mutex resultMutex;
            std::vector<std::filesystem::path> csvFiles;

            // 收集所有 CSV 文件路径
            for (const auto &entry : std::filesystem::directory_iterator(tempDir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".csv")
                {
                    csvFiles.push_back(entry.path());
                }
            }

            // 创建线程池处理文件
            const size_t threadCount = std::min(static_cast<size_t>(std::thread::hardware_concurrency()),
                                                csvFiles.size());
            std::vector<std::thread> threads;
            std::atomic<size_t> fileIndex{0};

            auto processFiles = [&]()
            {
                while (true)
                {
                    size_t currentIndex = fileIndex++;
                    if (currentIndex >= csvFiles.size())
                        break;

                    try
                    {
                        std::vector<SSMData> fileData;
                        fileData.reserve(1000); // 预分配空间

                        std::ifstream file(csvFiles[currentIndex]);
                        if (!file.is_open())
                            continue;

                        std::string line;
                        SSMData ssm;

                        // 使用缓冲区提高读取效率
                        char buffer[4096];
                        file.rdbuf()->pubsetbuf(buffer, sizeof(buffer));

                        while (std::getline(file, line))
                        {
                            size_t spacePos = line.find(' ');
                            if (spacePos == std::string::npos)
                                continue;

                            try
                            {
                                ssm.time = std::stod(line.substr(0, spacePos));
                                ssm.data = stringToVector(line.substr(spacePos + 1));
                                fileData.push_back(ssm);
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << "Error parsing line: " << e.what() << std::endl;
                                continue;
                            }
                        }

                        if (!fileData.empty())
                        {
                            std::lock_guard<std::mutex> lock(resultMutex);
                            result.push_back(std::move(fileData));
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error processing file: " << e.what() << std::endl;
                    }
                }
            };

            // 启动线程
            for (size_t i = 0; i < threadCount; ++i)
            {
                threads.emplace_back(processFiles);
            }

            // 等待所有线程完成
            for (auto &thread : threads)
            {
                thread.join();
            }

            // 移动结果到输出参数
            data = std::move(result);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in Squeread: " << e.what() << std::endl;
            data.clear();
        }
    }
}