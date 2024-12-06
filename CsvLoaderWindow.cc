#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include "mainwindow.h"

void MainWindow::onLoadCsvButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open CSV File", "", "CSV Files (*.csv*)");
    if (fileName.isEmpty())
        return;
    dz_io::IOoperator ioOperator(fileName.toStdString());
    read_doc_data.clear();
    read_data_time.clear();
    ioOperator.readAllParams(read_doc_data, read_data_time);
    std::cout << "Read CSV file successfully" << std::endl;
}
