#pragma once

#include <iostream>
#include <string>
#include <Eigen/Dense>
namespace dz_io
{
#define PARAM_PATH "./"
    class IOoperator
    {
    public:
        IOoperator() : path(std::string(PARAM_PATH)) {}
        ~IOoperator() {}
        IOoperator(std::string path_) : path(path_) { PATH_SET = true; }
        void setPath(std::string path_) { path = path_; }
        void setFilename(std::string filename_)
        {
            this->filename = filename_;
            this->PATH_SET = true;
        }
        void write(const std::string &param_name, const Eigen::MatrixXd &data);
        std::string write(std::vector<std::string> &param_names, std::vector<Eigen::VectorXd> &data);
        Eigen::MatrixXd read(const std::string &paramName);
        void readAllParams(std::vector<Eigen::VectorXd> &data, std::vector<double> &time);
        bool checkParam(const std::string &paramName);
        bool checkFile(const std::string &filename);
        void del(std::string filename);

    private:
        void checkPath(); // check if path is set
        bool PATH_SET = false;
        std::string filename;
        std::string path;
    };
}
