#pragma once

#include "dz_sm_socket_top.h"
#include <Eigen/Dense>
#include <iostream>
#include <string>
namespace dz_io
{
#define PARAM_PATH "./"
    class IOoperator
    {
    public:
        IOoperator() : path(std::string(PARAM_PATH))
        {
        }
        ~IOoperator()
        {
        }
        IOoperator(std::string path_) : path(path_)
        {
            PATH_SET = true;
        }
        void setPath(std::string path_)
        {
            path = path_;
        }
        IOoperator(std::string path_, std::string filename_)
        {
            path = path_ + "/";
            filename = filename_;
            PATH_SET = true;
            FILENAME_SET = true;
        }
        void setFilename(std::string filename_)
        {
            this->filename = filename_;
            this->PATH_SET = true;
        }
        void write(const std::string &param_name, const Eigen::MatrixXd &data);
        std::string write(std::vector<std::string> &param_names, std::vector<Eigen::VectorXd> &data);
        Eigen::MatrixXd read(const std::string &paramName);
        std::string write(std::string &filename, std::vector<dz_communicate::SSMData> &data);
        void read(std::string &filename_, std::vector<dz_communicate::SSMData> &data);
        void readAllParams(std::vector<Eigen::VectorXd> &data, std::vector<double> &time);
        void Squeread(std::vector<std::vector<dz_communicate::SSMData>> &data);
        std::string Squewrite(std::vector<std::vector<dz_communicate::SSMData>> &data);
        bool checkParam(const std::string &paramName);
        bool checkFile(const std::string &filename);
        void del(std::string filename);
        int get_progress()
        {
            return progress;
        };

    private:
        int progress = 0;
        bool worker_thread;
        std::thread *worker;
        void checkPath(); // check if path is set
        bool FILENAME_SET = false;
        bool PATH_SET = false;
        std::string filename;
        std::string path;
    };
} // namespace dz_io
