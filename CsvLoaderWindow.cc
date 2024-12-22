#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include "mainwindow.h"
void MainWindow::ReadDataClear()
{
    for (auto &doc : read_doc_data)
    {
        doc.clear();
    }
    read_doc_data.clear();
}
/* 读取离线数据文件 */
void MainWindow::onLoadCsvButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open CSV File", "", "CSV Files (*.csv*)");
    if (fileName.isEmpty())
        return;
    dz_io::IOoperator ioOperator(fileName.toStdString());
    ReadDataClear(); /* 清空读取文件的内存 */
    ioOperator.Squeread(read_doc_data);
    has_offline_data = true; /* 读取成功 */
    std::cout << "Read CSV file successfully" << std::endl;
}
