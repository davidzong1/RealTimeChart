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
    QString fileName = QFileDialog::getOpenFileName(this, "Open ZIP File", "", "ZIP Files (*.zip*)");
    if (fileName.isEmpty())
        return;

    QFileInfo fileInfo(fileName);
    QString filePath = fileInfo.path();
    QString baseFileName = fileInfo.fileName();

    std::cout << "Uncompressing zip file...,please wait..." << std::endl;

    m_progress = new QProgressDialog("Loading data...", "Cancel", 0, 100, this);
    m_progress->setWindowModality(Qt::WindowModal);
    m_progress->setWindowTitle("Loading");
    m_progress->show();

    dz_io::IOoperator ioOperator(filePath.toStdString(), baseFileName.toStdString());
    ReadDataClear();
    ioOperator.Squeread(read_doc_data);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this, &ioOperator]() {
        int currentProgress = ioOperator.get_progress();
        m_progress->setValue(currentProgress);

        if (currentProgress >= 100)
        {
            m_timer->stop();
            m_timer->deleteLater();
            m_progress->close();
            m_progress->deleteLater();
            std::cout << "Finished uncompress files!" << std::endl;
        }
    });
    m_timer->start(100);

    // 等待完成
    while (1)
    {
        QApplication::processEvents();
        if (m_progress->wasCanceled())
        {
            m_timer->stop();
            m_timer->deleteLater();
            m_progress->deleteLater();
            has_offline_data = true;
            emit loadCsvCompleted(); // 发送完成信号
            return;
        }
        usleep(100000);
    }
}
