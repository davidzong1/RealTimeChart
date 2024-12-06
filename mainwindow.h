#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QEvent>
#include <QObject>
#include <QMouseEvent>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpacerItem>
#include <QTextEdit>
#include <QMenu>
#include <QDateTime>
#include <QColor>
#include "dziostream.h"
#include "qcustomplot.h"
#include "dz_sm_socket_top.h"
#include "IOopperator.h"
#include <Eigen/Dense>
QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    QTextEditStream *debug_stream;
    std::streambuf *oldCoutStreamBuf;
    QWidget *centralWidget;
    QHBoxLayout *Top_layout;
    QVBoxLayout *main_layout;
    QVBoxLayout *show_layout;
    QHBoxLayout *UI_layout;
    QVBoxLayout *data_layout;
    QCustomPlot *customPlot;
    QLineEdit *socket_port;        /* socket_端口输入端 */
    QLineEdit *socket_ip;          /* socket_ip输入端 */
    QLineEdit *shm_key;            /*共享内存关键字*/
    QTextEdit *debug_text;         /*调试信息显示*/
    QPushButton *connect_button;   // 连接按钮
    QPushButton *reset_button;     // 重置按钮
    QPushButton *save_button;      // 保存按钮
    QPushButton *offline_image;    // 保存按钮
    QRadioButton *socket_select;   // socket选择按钮
    QRadioButton *shm_select;      // 共享内存选择按钮
    QPushButton *read_data_button; // 读取数据按钮
    QButtonGroup *buttonGroup;
    QTreeWidget *Data_name; /* 数据显示 */
    std::vector<QTreeWidgetItem *> data_item;
    QTimer *dataTimer;      // 定时器用于定期更新图表
    QTimer *exitCheckTimer; // 定时器用于定期检查退出信号
    QPoint lastMousePos;    // 鼠标位置
    bool followKey;
    bool isDragging = false;
    QColor randomColor();
    void QCPlot_init();
    void LineEdit_init();
    void button_init();
    void Debug_init();
    void updatePlot();
    friend void dataUpdateFcn(MainWindow *obj);
    void onResetButtonClicked();
    void onConnectButtonClicked();
    void onSaveButtonClicked();
    void OfflineImagePlant();
    void onDataNameItemChanged(QTreeWidgetItem *item, int column);
    void onLoadCsvButtonClicked();
    void checkExitSemaphore();

private:
    bool connecting_flag = false;
    bool data_item_blocked = false;
    double time_axi = 0;             // x轴时间
    std::thread *data_update_thread; /* 数据更新线程 */
    Eigen::MatrixXd rev_cache;
    bool data_ready = false;
    bool offline_image_mode = false;
    bool select_state;
    bool connect_flag = false; // 连接标志
    std::vector<Eigen::VectorXd> data;
    std::vector<std::string> data_time;
    std::vector<Eigen::VectorXd> read_doc_data;
    std::vector<double> read_data_time;
    std::vector<int> show_index;
    std::vector<QColor> line_color;
    dz_communicate::dz_com *com;
    dz_io::IOoperator Data_io; /* 保存文件操作口 */
    std::mutex data_mutex;     /* 数据更新互斥锁 */
    std::mutex data_rev_mutex; /* 数据接收互斥锁 */
    void resetConnectButton();
    void updateConnectButton();
    /* 退出同步 */
    bool exit_sem = false;
    bool host_intrupt = false; /* 主动断链 */
};
#endif // MAINWINDOW_H
