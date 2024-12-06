#include "IOopperator.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <vector>

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

}