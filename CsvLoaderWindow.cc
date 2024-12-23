#include "mainwindow.h"
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QProgressDialog>
#include <QTextStream>
#include <QTimer>

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
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Open ZIP File",
                                                    "",
                                                    "ZIP Files (*.zip*)");
    if (fileName.isEmpty())
        return;

    QFileInfo fileInfo(fileName);
    QString filePath = fileInfo.path();         // 获取文件路径
    QString baseFileName = fileInfo.fileName(); // 获取文件名(带后缀)

    // 创建进度条对话框
    QProgressDialog progress("Loading data...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setWindowTitle("Loading");
    progress.show();

    dz_io::IOoperator ioOperator(filePath.toStdString(),
                                 baseFileName.toStdString());
    ReadDataClear(); /* 清空读取文件的内存 */
    ioOperator.Squeread(read_doc_data);

    // 创建定时器监控进度
    QTimer timer;
    connect(&timer, &QTimer::timeout, [&]() {
        int currentProgress = ioOperator.get_progress();
        progress.setValue(currentProgress);

        if (currentProgress >= 100)
        {
            timer.stop();
            progress.close();
        }
    });
    timer.start(100); // 每100ms更新一次进度

    // 等待完成
    while (ioOperator.get_progress() < 100)
    {
        QApplication::processEvents();
        if (progress.wasCanceled())
        {
            return;
        }
        usleep(100000);
    }

    has_offline_data = true; /* 读取成功 */
}
