#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cmath>
#include <regex>
#define skselect true
#define smselect false
#define TIME_DIFF_MS(t0, t1) ((t1.tv_sec + t1.tv_nsec * 1e-9) - (t0.tv_sec + t0.tv_nsec * 1e-9)) * 1e3
using namespace dz_communicate;
void dataUpdateFcn(MainWindow *obj);
void clear_item(std::vector<item_tree> &a);
void clear_data_point(std::vector<std::vector<SSMData>> &a);
int getDataIndex(std::string name);
std::pair<SSMData *, int> getDataFromItem(QTreeWidgetItem *son, std::vector<SSMData> &rev_data);
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /* 初始化QCustomPlot */
    QCPlot_init();
    Debug_init();
    LineEdit_init();
    button_init();
    followKey = true;
    dataTimer = new QTimer(this);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::updatePlot);
    dataTimer->start(1); // 每1ms更新一次数据
    connect(reset_button, &QPushButton::clicked, this, &MainWindow::onResetButtonClicked);
    connect(connect_button, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    connect(save_button, &QPushButton::clicked, this, &MainWindow::onSaveButtonClicked);
    connect(offline_image, &QPushButton::clicked, this, &MainWindow::OfflineImagePlant);
    connect(Data_name, &QTreeWidget::itemChanged, this, &MainWindow::onDataNameItemChanged);
    connect(read_data_button, &QPushButton::clicked, this, &MainWindow::onLoadCsvButtonClicked);
    connect(this, &MainWindow::loadCsvCompleted, this, &MainWindow::OfflineImagePlant);
    // 初始化 exitCheckTimer
    exitCheckTimer = new QTimer(this);
    connect(exitCheckTimer, &QTimer::timeout, this, &MainWindow::checkExitSemaphore);
    data_update_thread = new std::thread(&dataUpdateFcn, this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
QColor MainWindow::randomColor()
{
    int r = QRandomGenerator::global()->bounded(0,
                                                128); // 生成 0 到 127 之间的随机数
    int g = QRandomGenerator::global()->bounded(0,
                                                128); // 生成 0 到 127 之间的随机数
    int b = QRandomGenerator::global()->bounded(0,
                                                128); // 生成 0 到 127 之间的随机数
    return QColor(r, g, b);
}
void MainWindow::QCPlot_init()
{
    // 创建一个新的 QWidget 作为 centralWidget
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    // 创建 QCustomPlot 实例
    customPlot = new QCustomPlot(this);
    // 设置一些初始配置
    customPlot->addGraph();
    customPlot->xAxis->setLabel("Time");
    customPlot->yAxis->setLabel("Value");
    // 重新缩放坐标轴
    customPlot->xAxis->rescale(true);
    customPlot->yAxis->rescale(true);
    customPlot->xAxis->setRange(0, 3);
    customPlot->yAxis->setRange(-1, 1);
    // 启用 QCustomPlot 的交互功能
    customPlot->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    // 仅启用横纵坐标的滚轮缩放
    customPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    customPlot->installEventFilter(this); // 安装事件过滤器
    customPlot->legend->setVisible(true); // 显示图例
    customPlot->setAntialiasedElements(QCP::aeNone);
    customPlot->setOpenGl(true); // 启用 OpenGL 加速
    // 重新绘制图表
    customPlot->replot();
    // 创建一个主布局管理器
    Top_layout = new QHBoxLayout(centralWidget);
    main_layout = new QVBoxLayout();
    // 创建一个垂直布局管理器
    show_layout = new QVBoxLayout();
    show_layout->addWidget(customPlot); // 将 QCustomPlot 添加到布局中
    // 如果需要，可以设置布局的边距
    show_layout->setContentsMargins(0, 0, 0, 0);
    // 将 show_layout 添加到主布局
    main_layout->addLayout(show_layout, 10);
    Data_name = new QTreeWidget(this);
    // 设置列数和标题
    Data_name->setColumnCount(1);
    Data_name->setHeaderLabels(QStringList() << "Data Name");
    data_layout = new QVBoxLayout();
    data_layout->addWidget(Data_name);
    /* 顶层layout */
    Top_layout->addLayout(main_layout, 5);
    Top_layout->addLayout(data_layout, 1);
}

void MainWindow::LineEdit_init()
{
    // 创建 QLineEdit 实例
    socket_port = new QLineEdit(this);
    socket_ip = new QLineEdit(this);
    shm_key = new QLineEdit(this);
    QLabel *label_shm = new QLabel("Share Memory Key:", this);
    QLabel *label_socket = new QLabel("Socket IP:", this);
    QLabel *label_port = new QLabel("Socket Port:", this);
    label_shm->setAlignment(Qt::AlignRight);
    label_socket->setAlignment(Qt::AlignRight);
    label_port->setAlignment(Qt::AlignRight);
    // 设置 QLineEdit 属性
    socket_port->setPlaceholderText("Please Input Port...");         // 设置占位符文本
    socket_port->setMaxLength(20);                                   // 设置最大长度
    socket_ip->setPlaceholderText("Please Input Client IP...");      // 设置占位符文本
    socket_ip->setMaxLength(20);                                     // 设置最大长度
    shm_key->setPlaceholderText("Please Input share memory key..."); // 设置占位符文本
    shm_key->setMaxLength(20);                                       // 设置最大长度
    // 将 QLabel 添加到布局中
    QVBoxLayout *text_layout = new QVBoxLayout();
    QHBoxLayout *label1_layout = new QHBoxLayout();
    QHBoxLayout *label2_layout = new QHBoxLayout();
    QHBoxLayout *label3_layout = new QHBoxLayout();
    label1_layout->setSpacing(5);
    label1_layout->addWidget(label_port, 0);
    label1_layout->addWidget(socket_port, 0);
    label2_layout->setSpacing(5);
    label2_layout->addWidget(label_socket, 0);
    label2_layout->addWidget(socket_ip, 0);
    label3_layout->setSpacing(5);
    label3_layout->addWidget(label_shm, 0);
    label3_layout->addWidget(shm_key, 0);
    text_layout->addLayout(label1_layout, 1);
    text_layout->addLayout(label2_layout, 1);
    text_layout->addLayout(label3_layout, 1);
    // 将垂直布局添加到水平布局中
    UI_layout->addLayout(text_layout); /* 添加socket布局 */
    UI_layout->setContentsMargins(20, 10, 20, 10);
    // 将 UI_layout 添加到主布局
    main_layout->addLayout(UI_layout, 1);
}

void MainWindow::button_init()
{
    // 创建 QPushButton 实例
    connect_button = new QPushButton("Connect", this);
    connect_button->setFixedSize(80, 70);
    QPalette palette = connect_button->palette();
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::Button, Qt::blue); // 使用预定义的颜色常量
    connect_button->setPalette(palette);
    save_button = new QPushButton("Save Data", this);
    save_button->setFixedSize(80, 40);
    palette.setColor(QPalette::Button, Qt::green); // 使用预定义的颜色常量
    save_button->setPalette(palette);
    reset_button = new QPushButton("Reset Range", this);
    // 创建 QRadioButton 实例
    socket_select = new QRadioButton("Socket ", this);
    shm_select = new QRadioButton("Shm ", this);
    buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(socket_select);
    buttonGroup->addButton(shm_select);
    socket_select->setChecked(true);
    // 创建垂直布局管理器
    QVBoxLayout *button_layout = new QVBoxLayout();
    button_layout->setSpacing(5);
    button_layout->addWidget(reset_button);
    button_layout->addWidget(socket_select);
    button_layout->addWidget(shm_select);
    button_layout->addWidget(connect_button);
    button_layout->addWidget(save_button);
    // 将 button_layout 添加到主布局
    UI_layout->addLayout(button_layout);
    /* 创建离线图像生成按扭 */
    offline_image = new QPushButton("Offline Image", this);
    offline_image->setFixedSize(120, 60);
    read_data_button = new QPushButton("Read Data", this);
    read_data_button->setFixedSize(120, 60);
    palette.setColor(QPalette::Button, Qt::yellow); // 使用预定义的颜色常量
    read_data_button->setPalette(palette);
    palette.setColor(QPalette::Button, Qt::green); // 使用预定义的颜色常量
    read_data_button->setPalette(palette);
    data_layout->addWidget(read_data_button);
    data_layout->addWidget(offline_image);
}
void MainWindow::Debug_init()
{
    QLabel *label_port = new QLabel("Output Log:", this);
    debug_text = new QTextEdit(this);
    debug_text->setReadOnly(true);
    debug_text->setStyleSheet("background-color: white;");
    QTextCharFormat format;
    format.setForeground(Qt::black);
    debug_text->mergeCurrentCharFormat(format);
    QTextCursor cursor = debug_text->textCursor();
    /* 行间距 */
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(100, QTextBlockFormat::ProportionalHeight);
    cursor.setBlockFormat(blockFormat);
    QVBoxLayout *debug_layout = new QVBoxLayout();
    debug_layout->addWidget(label_port);
    debug_layout->addWidget(debug_text);
    // 创建一个水平布局管理器
    UI_layout = new QHBoxLayout();
    UI_layout->addLayout(debug_layout);
    /* std::cout重定向 */
    debug_stream = new QTextEditStream(debug_text);
    oldCoutStreamBuf = std::cout.rdbuf();
    std::cout.rdbuf(debug_stream->rdbuf());
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == customPlot)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                followKey = false; // 停止跟随
                isDragging = true;
                lastMousePos = mouseEvent->pos();
                return true; // 事件已处理
            }
        }
        else if (event->type() == QEvent::MouseMove && isDragging)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            int deltaX = mouseEvent->pos().x() - lastMousePos.x();
            int deltaY = mouseEvent->pos().y() - lastMousePos.y();
            double xrange = customPlot->xAxis->range().size();
            double yrange = customPlot->yAxis->range().size();
            double pixelToCoordx = xrange / customPlot->axisRect()->width();
            double pixelToCoordy = yrange / customPlot->axisRect()->height();
            customPlot->xAxis->moveRange(-deltaX * pixelToCoordx);
            customPlot->yAxis->moveRange(deltaY * pixelToCoordy);
            customPlot->replot();
            lastMousePos = mouseEvent->pos();
            return true; // 事件已处理
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                isDragging = false;
                return true; // 事件已处理
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
void dataUpdateFcn(MainWindow *obj)
{
    bool name_set_flag = false;
    timespec next_time, time1, time2;
    long cnt = 0;
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &time1);
        clock_gettime(CLOCK_MONOTONIC, &next_time);
        next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
        next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
        /* 连接中断 */
        std::vector<SSMData> rev_cache_cache;
        if (obj->connect_flag && !obj->exit_sem)
        {
            rev_cache_cache.clear(); /* 清空缓存 */
            bool rev_flag = obj->com->squeread(rev_cache_cache,
                                               obj->select_state); // 读取数据
            if (rev_flag)                                          /* 接收成功 */
            {
                if (rev_cache_cache.size() == 0)
                {
                    name_set_flag = false;
                    obj->exit_sem = true;
                }
                else
                {
                    obj->data_rev_mutex.lock();
                    obj->rev_cache = rev_cache_cache;
                    if (!name_set_flag)
                    {
                        clear_item(obj->data_item);
                        clear_data_point(obj->data);
                        name_set_flag = true;
                    }
                    // 在dataUpdateFcn函数中修改相关代码
                    if (obj->data_mutex.try_lock()) /* 记录数据 */
                    {
                        // 检查并更新数据容器
                        for (int i = 0; i < rev_cache_cache.size(); i++)
                        {
                            std::string data_name = rev_cache_cache[i].name;
                            // 查找数据名称对应的索引
                            auto it = obj->data_index_map.find(data_name);
                            size_t index;
                            if (it == obj->data_index_map.end())
                            {
                                // 未找到对应索引,创建新容器
                                index = obj->data.size();
                                obj->data_index_map[data_name] = index;
                                obj->data.push_back(std::vector<SSMData>());
                                obj->data_item.push_back(item_tree());
                                /* 添加QT列表 */
                                QTreeWidgetItem *motheritem = new QTreeWidgetItem(obj->Data_name);
                                motheritem->setText(0, QString::fromStdString(data_name));
                                for (int j = 0; j < rev_cache_cache[i].data.size(); j++) /* 子项目 */
                                {
                                    QTreeWidgetItem *item = new QTreeWidgetItem(motheritem);
                                    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                                    item->setText(0, QString::fromStdString("Data" + std::to_string(j)));
                                    item->setCheckState(0, Qt::Unchecked);
                                    obj->data_item[i].son_item.push_back(item);
                                }
                                obj->data_item[i].mother_item = motheritem;
                                /* 默认处于折叠状态 */
                                obj->Data_name->collapseItem(obj->data_item[i].mother_item);
                            }
                            else
                            {
                                // 使用已存在的索引
                                index = it->second;
                            }
                            // 将数据添加到对应容器
                            obj->data_op_mutex.lock();
                            obj->data[index].push_back(rev_cache_cache[i]);
                            obj->data_op_mutex.unlock();
                        }
                        obj->data_mutex.unlock();
                        cnt++;
                    }
                    obj->data_rev_mutex.unlock();
                    obj->data_ready = true;
                }
            }
        }
        /* 主动中断 */
        else if (obj->host_intrupt && !obj->exit_sem)
        {
            obj->exit_sem = true;
            name_set_flag = false;
        }
        else
        {
            name_set_flag = false;
        }
        clock_gettime(CLOCK_MONOTONIC, &time2);
        // std::cout << "sm_tran Time taken: " << TIME_DIFF_MS(time1, time2) << " microseconds" << std::endl;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
}
void MainWindow::onDataNameItemChanged(QTreeWidgetItem *item, int column)
{
    /* data_item阻塞，当data_item清除所有选中时为真 */
    if (!data_item_blocked)
    {
        if (item->parent() != nullptr && item->parent()->parent() == nullptr)
        {
            QTreeWidgetItem *motherItem = item->parent(); // 获取子项目对应的母项目
            if (motherItem == nullptr)
            {
                std::cout << "Error: "
                          << "Mother item not found" << std::endl;
                return;
            }
            if (item->checkState(column) == Qt::Checked)
            {
                show_index.push_back(item);
                /* 将母项目背景变为黄色 */
                motherItem->setBackground(0, QBrush(Qt::yellow));
                // 将母项目字体变为红色
                motherItem->setForeground(0, QBrush(Qt::red));
            }
            else
            {
                show_index.erase(std::remove(show_index.begin(), show_index.end(), item), show_index.end());
                // 检查母项目的所有子项目是否都未选中，如果是，则恢复母项目的背景颜色
                bool anyChildChecked = false;
                for (int i = 0; i < motherItem->childCount(); ++i)
                {
                    if (motherItem->child(i)->checkState(column) == Qt::Checked)
                    {
                        anyChildChecked = true;
                        break;
                    }
                }

                if (!anyChildChecked)
                {
                    motherItem->setBackground(0,
                                              QBrush(Qt::NoBrush)); // 恢复母项目背景颜色
                    motherItem->setForeground(0,
                                              QBrush(Qt::NoBrush)); // 将母项目字体变为黑色
                }
            }
            /* 离线画图 */
            if (offline_image_mode)
            {
                customPlot->clearGraphs();
                double max_vaule = 1;
                double min_value = -1;
                std::vector<std::vector<SSMData>> *data_ptr; // 指向data的指针
                if (!has_offline_data)
                {
                    data_ptr = &data;
                }
                else
                {
                    data_ptr = &read_doc_data;
                }
                auto &data_ref = *data_ptr; // 引用data
                while (customPlot->graphCount() < (int)show_index.size())
                {
                    customPlot->addGraph();
                }
                data_op_mutex.lock();
                /* 显示个数循环 */
                for (std::vector<double>::size_type j = 0; j < show_index.size(); j++)
                {
                    std::string mother_name = show_index[j]->parent()->text(0).toStdString();
                    int data_index = getDataIndex(show_index[j]->text(0).toStdString());
                    /* 母项目循环 */
                    for (int i = 0; i < data_ref.size(); i++)
                    {
                        if (mother_name == data_ref[i][0].name)
                        {
                            /* 子项目选中循环 */
                            for (int z = 0; z < data_ref[i].size(); z++)
                            {
                                customPlot->graph(j)->addData(data_ref[i][z].time, data_ref[i][z].data(data_index));
                                if (max_vaule < data_ref[i][z].data(data_index))
                                    max_vaule = data_ref[i][z].data(data_index);
                                if (min_value > data_ref[i][z].data(data_index))
                                    min_value = data_ref[i][z].data(data_index);
                            }
                            customPlot->graph(j)->setName(QString::fromStdString(data_ref[i][0].name) + QString(" ") + item->text(0)); // 设置图例名称
                            customPlot->graph(j)->setPen(QPen(randomColor(), 2)); // 设置线条颜色和宽度
                        }
                    }
                }
                data_op_mutex.unlock();
                QFont legendFont = font();               // 使用默认字体
                legendFont.setPointSize(9);              // 设置字体大小
                customPlot->legend->setFont(legendFont); // 设置图例字体
                customPlot->axisRect()->insetLayout()->setInsetAlignment(0,
                                                                         Qt::AlignBottom | Qt::AlignLeft); // 设置图例位置
                customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 150)));
                customPlot->rescaleAxes(true); // 调整坐标轴范围以适应数据
                customPlot->yAxis->setRange(min_value,
                                            max_vaule); // 保持10秒的数据
                customPlot->replot();
            }
        }
    }
}
/**
 * @brief 周期性更新图表数据
 */
void MainWindow::updatePlot()
{
    static double max_vaule = 1;
    static double min_value = -1;
    static double show_time_axi = 0.0; /* 显示轴 */
    std::vector<SSMData> cache;
    SSMData *show_data;
    if (data_ready && connect_flag)
    {
        data_rev_mutex.lock();
        cache = rev_cache;
        data_ready = false;
        data_rev_mutex.unlock();
        if (followKey)
        {
            while (customPlot->graphCount() < (int)show_index.size())
            {
                customPlot->addGraph();
                line_color.push_back(randomColor()); // 随机生成颜色
            }
            for (std::vector<double>::size_type i = 0; i < show_index.size(); i++)
            {
                auto [data_ptr, index] = getDataFromItem(show_index[i], cache);
                if (data_ptr) // 数据有效，可以使用
                {
                    show_data = data_ptr;
                }
                else // 处理错误情况
                {
                    std::cout << "Data Name: " << data_ptr->name << "is not found!" << std::endl;
                }
                /* 显示上下限处理 */
                if (max_vaule < show_data->data(index))
                    max_vaule = show_data->data(index);
                if (min_value > show_data->data(index))
                    min_value = show_data->data(index);
                if (i == 0)
                    show_time_axi = show_data->time; // 设置时间轴,第一个数据为基准
                customPlot->graph(i)->addData(show_data->time, show_data->data(index));
                customPlot->graph(i)->rescaleValueAxis(true);
                customPlot->graph(i)->setName(QString::fromStdString(show_data->name) + QString("/Data%1").arg(index)); // 设置图例名称
                customPlot->graph(i)->setPen(QPen(line_color[i], 2));                                                   // 设置线条颜色和宽度
                // 限制每条曲线只记录5000个点
                if (customPlot->graph(i)->data()->size() > 5000)
                {
                    auto it = customPlot->graph(i)->data()->at(customPlot->graph(i)->data()->size() - 5000);
                    customPlot->graph(i)->data()->removeBefore(it->key);
                }
            }
            QFont legendFont = font();               // 使用默认字体
            legendFont.setPointSize(9);              // 设置字体大小
            customPlot->legend->setFont(legendFont); // 设置图例字体
            customPlot->axisRect()->insetLayout()->setInsetAlignment(0,
                                                                     Qt::AlignBottom | Qt::AlignLeft); // 设置图例位置
            customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 150)));
            if (show_time_axi < 3)
            {
                customPlot->xAxis->setRange(0, 3); // 保持10秒的数据
            }
            else
            {
                customPlot->xAxis->setRange(show_time_axi - 3, show_time_axi);
            }
            customPlot->yAxis->setRange(min_value, max_vaule); // 保持10秒的数据
            customPlot->replot();
        }
    }
    else if (!connect_flag) /* 断链清空范围 */
    {
        double max_vaule = 1;
        double min_value = -1;
    }
}

/**
 * @brief 复位槽函数
 */
void MainWindow::onResetButtonClicked()
{
    followKey = true;
}
/**
 * @brief 连接槽函数
 */
void MainWindow::onConnectButtonClicked()
{
    QAbstractButton *checkedButton = buttonGroup->checkedButton();
    if (!connect_flag)
    {
        if (!connecting_flag)
        {
            clear_data_point(data); /* 清除所有数据 */
            // data_time.clear();
            QPalette palette = connect_button->palette();
            palette.setColor(QPalette::Button,
                             Qt::yellow); // 使用预定义的颜色常量
            palette.setColor(QPalette::ButtonText, Qt::black);
            connect_button->setPalette(palette);
            connect_button->setText("Connecting");
            connecting_flag = true;
            offline_image_mode = false;
            customPlot->clearGraphs(); // 清除所有图形

            // 立即尝试连接
            if (checkedButton == socket_select)
            {
                select_state = skselect;
                std::cout << "Connecting socket" << std::endl;
                QString text = socket_ip->text(); // 获取文本框的内容
                std::string ip = text.toStdString();
                text = socket_port->text(); // 获取文本框的内容
                std::string port = text.toStdString();
                if (ip == "Please Input Client IP..." || ip == "")
                {
                    std::cout << "Please Input Client IP" << std::endl;
                    resetConnectButton();
                    return;
                }
                if (port == "Please Input Port..." || port == "")
                {
                    std::cout << "Please Input Client Port" << std::endl;
                    resetConnectButton();
                    return;
                }
                int port1 = std::stoi(port);
                com = new dz_communicate::dz_com(isserver, ip.c_str(), port1, true);
            }
            else
            {
                select_state = smselect;
                std::cout << "Connecting Share Memory" << std::endl;
                QString text = shm_key->text(); // 获取文本框的内容
                std::string name = text.toStdString();
                if (name == "Please Input share memory key..." || name == "")
                {
                    std::cout << "Please Input Share Memory key" << std::endl;
                    resetConnectButton();
                    return;
                }
                int key = std::stoi(name);
                com = new dz_communicate::dz_com(high_level, key, true);
            }
            std::cout << "Connecting success" << std::endl;
            has_offline_data = false; /* 复位离线相关操作 */
            ReadDataClear();          /* 清空读取文件的内存 */
            // 模拟连接成功
            connect_flag = true;
            updateConnectButton();
            return;
        }
    }
    else
    {
        connect_flag = false;
        host_intrupt = true;
        exitCheckTimer->start(100); // 每100ms检查一次
    }
}
void MainWindow::checkExitSemaphore()
{
    if (exit_sem)
    {
        exitCheckTimer->stop();
        delete com;
        resetConnectButton();
        exit_sem = false;
        host_intrupt = false;
    }
}
void MainWindow::resetConnectButton()
{
    QPalette palette = connect_button->palette();
    palette.setColor(QPalette::Button, Qt::blue); // 使用预定义的颜色常量
    palette.setColor(QPalette::ButtonText, Qt::black);
    connect_button->setPalette(palette);
    connect_button->setText("Connect");
    connecting_flag = false;
}

void MainWindow::updateConnectButton()
{
    if (connect_flag) /* 刚连接，清除此前缓存 */
    {
        customPlot->clearGraphs(); // 清除所有图形
        QPalette palette = connect_button->palette();
        palette.setColor(QPalette::Button, Qt::red); // 使用预定义的颜色常量
        palette.setColor(QPalette::ButtonText, Qt::black);
        connect_button->setPalette(palette);
        connect_button->setText("Disconnect");
    }
}

/**
 * @brief 保存槽函数
 */
void MainWindow::onSaveButtonClicked()
{
    if (connect_flag)
    {
        std::cout << "Please Disconnect the connection!!!!!" << std::endl;
        return;
    }
    offline_file_name = ""; // 清空离线文件名
    std::string homeDir = std::getenv("HOME");
    // 获取当前时间并格式化
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y%m%d_%H%M%S");
    std::string timeStr = ss.str();                                    // 获取当前时间并格式化
    dz_io::IOoperator Data_io(homeDir + "/dz_log", "data_" + timeStr); /* 保存文件操作口 */
    m_progress = new QProgressDialog("Saving data...", "Cancel", 0, 100, this);
    m_progress->setWindowFlags(m_progress->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_progress->setMinimumDuration(0); // 立即显示
    m_progress->setAutoClose(false);   // 不自动关闭
    m_progress->setAutoReset(false);   // 不自动重置
    m_progress->show();
    // 创建定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this, &Data_io]() {
        int currentProgress = Data_io.get_progress();
        m_progress->setValue(currentProgress);
        if (currentProgress >= 100)
        {
            m_timer->stop();
            m_timer->deleteLater();
            m_progress->close();
            m_progress->deleteLater();
            if (offline_file_name != "")
                std::cout << "Save data to " << offline_file_name << std::endl;
            else
                std::cout << "Save data failed" << std::endl;
        }
    });

    // 启动定时器
    m_timer->start(100);

    // 开始保存数据
    data_mutex.lock();
    offline_file_name = Data_io.Squewrite(data);
    data_mutex.unlock();

    while (Data_io.get_progress() < 100)
    {
        QApplication::processEvents();
        if (m_progress->wasCanceled())
        {
            m_timer->stop();
            m_timer->deleteLater();
            m_progress->deleteLater();
            return;
        }
        QThread::msleep(50); // 使用QThread::msleep替代usleep
    }
}
/**
 * @brief 离线图像槽函数
 */
void MainWindow::OfflineImagePlant()
{
    if (!connect_flag)
    {
        if (data.size() == 0 && read_doc_data.size() == 0)
        {
            std::cout << "No Data" << std::endl;
            return;
        }
        customPlot->clearGraphs(); // 清除所有图形
        show_index.clear();
        std::vector<std::vector<dz_communicate::SSMData>> *data_ptr;
        /* 重置列表，重置为读取文档的 */
        if (read_doc_data.size() != 0 && has_offline_data)
        {
            data_ptr = &read_doc_data;
        }
        else /* 离线显示刚在线接收的文件 */
        {
            data_ptr = &data;
        }
        data_item_blocked = true;
        line_color.clear();                         /* 清除所有颜色 */
        Data_name->clear();                         /* 清除QT列表数据 */
        clear_item(data_item);                      /* 清除容器列表数据 */
        for (auto i = 0; i < data_ptr->size(); i++) /* 清除所有打勾 */
        {
            /* 去第一个SSMData的向量初始化列表 */
            data_item.push_back(item_tree()); /* 每个data_item对应一个data */
            data_item[i].mother_item = new QTreeWidgetItem(Data_name);
            data_item[i].son_item.resize((*data_ptr)[i][0].data.size());
            for (auto j = 0; j < data_item[i].son_item.size(); j++)
            {
                /* 每个data_item对应一个向量的元素 */
                data_item[i].son_item[j] = new QTreeWidgetItem(data_item[i].mother_item);
                data_item[i].son_item[j]->setFlags(data_item[i].son_item[j]->flags() | Qt::ItemIsUserCheckable); /* 设置打勾 */
                // 0 表示第一列，Qt::Checked 表示选中打勾
                data_item[i].son_item[j]->setCheckState(0, Qt::Unchecked);
                data_item[i].son_item[j]->setText(0, "Data" + QString::number(j));
            }
            data_item[i].mother_item->setText(0, QString::fromStdString((*data_ptr)[i][0].name));
            Data_name->collapseItem(data_item[i].mother_item); // 折叠子项
        }
        data_item_blocked = false;
        std::cout << "Offline Image Mode" << std::endl;
        offline_image_mode = true;
    }
    else if (connect_flag)
    {
        std::cout << "Please Disconnect" << std::endl;
        return;
    }
}

void clear_item(std::vector<item_tree> &a)
{
    for (int i = 0; i < a.size(); i++)
    {
        a[i].son_item.clear();
    }
    a.clear();
}

void clear_data_point(std::vector<std::vector<SSMData>> &a)
{
    for (int i = 0; i < a.size(); i++)
    {
        a[i].clear();
    }
    a.clear();
}
int getDataIndex(std::string name)
{
    std::regex pattern("Data(\\d+)");
    std::smatch result;

    if (std::regex_search(name, result, pattern))
    {
        std::string number_str = result[1];
        return std::stoi(number_str);
    }
    std::cout << "Error: Invalid data name format" << name << "is not Data_***" << std::endl;
    return -1;
}
std::pair<SSMData *, int> getDataFromItem(QTreeWidgetItem *son, std::vector<SSMData> &rev_data)
{
    if (!son || !son->parent())
    {
        return std::make_pair(nullptr, -1);
    }

    std::string mother_name = son->parent()->text(0).toStdString();
    int data_index = getDataIndex(son->text(0).toStdString());

    for (auto &data : rev_data)
    {
        if (data.name == mother_name)
        {
            return std::make_pair(&data, data_index);
        }
    }

    return std::make_pair(nullptr, -1);
}
