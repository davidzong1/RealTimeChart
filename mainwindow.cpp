#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cmath>
#define skselect true
#define smselect false
void dataUpdateFcn(MainWindow *obj);
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    Data_io.setFilename("watch_output.csv");
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
    int r = QRandomGenerator::global()->bounded(0, 128); // 生成 0 到 127 之间的随机数
    int g = QRandomGenerator::global()->bounded(0, 128); // 生成 0 到 127 之间的随机数
    int b = QRandomGenerator::global()->bounded(0, 128); // 生成 0 到 127 之间的随机数
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
    customPlot->yAxis->setRange(-1, 1);
    // 启用 QCustomPlot 的交互功能
    customPlot->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    // 仅启用横纵坐标的滚轮缩放
    customPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    customPlot->installEventFilter(this); // 安装事件过滤器
    customPlot->legend->setVisible(true); // 显示图例
    // QFont legendFont = font();            // 使用默认字体
    // legendFont.setPointSize(9);           // 设置字体大小
    // customPlot->legend->setFont(legendFont);
    // customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 150))); // 设置图例背景颜色
    // // 将图例放置在左下角
    // customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignLeft);
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
    static bool data_flag = false;
    static bool cout_flag = false;
    timespec next_time;
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &next_time);
        next_time.tv_sec += (next_time.tv_nsec + 0.001 * 1e9) / 1e9;
        next_time.tv_nsec = (int)(next_time.tv_nsec + 0.001 * 1e9) % (int)1e9;
        /* 连接中断 */
        if (obj->connect_flag && !obj->exit_sem)
        {
            Eigen::MatrixXd rev_cache_cache;
            rev_cache_cache = obj->com->read(obj->select_state); // 读取数据
            if (rev_cache_cache.size() == 0)
            {
                std::cout << "Timeout occurred while waiting result." << std::endl;
                obj->exit_sem = true;
                data_flag = false;
                continue;
            }
            if (rev_cache_cache.rows() != 0 && rev_cache_cache.cols() != 0)
            {
                obj->data_rev_mutex.lock();
                obj->rev_cache = rev_cache_cache;
                if (!data_flag)
                {
                    data_flag = true;
                    obj->Data_name->clear();
                    int data_size = obj->rev_cache.rows() - 1;
                    obj->data_item.resize(data_size);
                    obj->line_color.clear();
                    for (int i = 0; i < data_size; i++)
                    {
                        obj->data_item[i] = new QTreeWidgetItem(obj->Data_name);
                        obj->data_item[i]->setFlags(obj->data_item[i]->flags() | Qt::ItemIsUserCheckable);
                        obj->data_item[i]->setCheckState(0, Qt::Unchecked);          // 0 表示第一列，Qt::Checked 表示选中打勾
                        obj->data_item[i]->setText(0, "Data " + QString::number(i)); // 设置第一列的文本
                        obj->line_color.push_back(obj->randomColor());
                    }
                }
                if (obj->data_mutex.try_lock())
                {
                    obj->data_time.push_back(std::to_string(rev_cache_cache(0, 0)));
                    obj->data.push_back(rev_cache_cache.block(1, 0, rev_cache_cache.rows() - 1, 1));
                    obj->data_mutex.unlock();
                }
                obj->data_rev_mutex.unlock();
                cout_flag = false;
                obj->data_ready = true;
            }
            else
            {
                if (!cout_flag)
                    std::cout << "Data is NULL" << std::endl;
                cout_flag = true;
            }
        }
        /* 主动中断 */
        else if (obj->host_intrupt && !obj->exit_sem)
        {
            obj->exit_sem = true;
        }
        else
        {
            data_flag = false;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
}
void MainWindow::onDataNameItemChanged(QTreeWidgetItem *item, int column)
{
    /* data_item阻塞，当data_item清除所有选中时为真 */
    if (!data_item_blocked)
    {
        int index = Data_name->indexOfTopLevelItem(item);
        if (index == -1)
        {
            std::cout << "Error: " << "Item not found" << std::endl;
            return;
        }
        if (item->checkState(column) == Qt::Checked)
        {
            show_index.push_back(index);
        }
        else
        {
            show_index.erase(std::remove(show_index.begin(), show_index.end(), index), show_index.end());
        }
        /* 离线画图 */
        if (offline_image_mode)
        {
            bool has_read_file = false;
            customPlot->clearGraphs();
            double max_vaule = 1;
            double min_value = -1;
            std::vector<Eigen::VectorXd> *data_ptr; // 指向data的指针
            has_read_file = read_doc_data.size() == 0 ? false : true;
            if (!has_read_file)
            {
                data_ptr = &data;
            }
            else
            {
                data_ptr = &read_doc_data;
            }
            while (customPlot->graphCount() < (int)show_index.size())
            {
                customPlot->addGraph();
            }
            for (std::vector<double>::size_type j = 0; j < show_index.size(); j++)
            {
                for (std::vector<Eigen::VectorXd>::size_type i = 0; i < data_ptr->size(); i++)
                {
                    if (!has_read_file)
                        customPlot->graph(j)->addData(std::stod((data_time)[i]), (*data_ptr)[i](show_index[j]));
                    else
                        customPlot->graph(j)->addData(read_data_time[i], (*data_ptr)[i](show_index[j]));
                    if (max_vaule < (*data_ptr)[i](show_index[j]))
                        max_vaule = (*data_ptr)[i](show_index[j]);
                    if (min_value > (*data_ptr)[i](show_index[j]))
                        min_value = (*data_ptr)[i](show_index[j]);
                }
                customPlot->graph(j)->setName(QString("Data %1").arg(j + 1));     // 设置图例名称
                customPlot->graph(j)->setPen(QPen(line_color[show_index[j]], 2)); // 设置线条颜色和宽度
            }
            QFont legendFont = font();                                                                    // 使用默认字体
            legendFont.setPointSize(9);                                                                   // 设置字体大小
            customPlot->legend->setFont(legendFont);                                                      // 设置图例字体
            customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignLeft); // 设置图例位置
            customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 150)));
            customPlot->rescaleAxes(true);                     // 调整坐标轴范围以适应数据
            customPlot->yAxis->setRange(min_value, max_vaule); // 保持10秒的数据
            customPlot->replot();
        }
    }
}
/**
 * @brief 周期性更新图表数据
 */
void MainWindow::updatePlot()
{
    double max_vaule = 1;
    double min_value = -1;
    Eigen::VectorXd cache;
    std::vector<double> show_data;
    if (data_ready && connect_flag)
    {
        data_rev_mutex.lock();
        time_axi = rev_cache(0, 0);
        cache = rev_cache.block(1, 0, rev_cache.rows() - 1, 1);
        data_ready = false;
        data_rev_mutex.unlock();
        for (int i = 0; i < cache.size(); i++)
        {
            if (max_vaule < cache(i))
                max_vaule = cache(i);
            if (min_value > cache(i))
                min_value = cache(i);
        }
        if (followKey)
        {
            for (auto i = 0; i < show_index.size(); i++)
            {
                show_data.push_back(cache(show_index[i]));
            }
            while (customPlot->graphCount() < (int)show_data.size())
            {
                customPlot->addGraph();
            }
            for (std::vector<double>::size_type i = 0; i < show_data.size(); i++)
            {
                customPlot->graph(i)->addData(time_axi, show_data[i]);
                customPlot->graph(i)->rescaleValueAxis(true);
                customPlot->graph(i)->setName(QString("Data %1").arg(i + 1));     // 设置图例名称
                customPlot->graph(i)->setPen(QPen(line_color[show_index[i]], 2)); // 设置线条颜色和宽度
                                                                                  // 限制每条曲线只记录5000个点
                if (customPlot->graph(i)->data()->size() > 5000)
                {
                    auto it = customPlot->graph(i)->data()->at(customPlot->graph(i)->data()->size() - 5000);
                    customPlot->graph(i)->data()->removeBefore(it->key);
                }
            }
            QFont legendFont = font();                                                                    // 使用默认字体
            legendFont.setPointSize(9);                                                                   // 设置字体大小
            customPlot->legend->setFont(legendFont);                                                      // 设置图例字体
            customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignLeft); // 设置图例位置
            customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 150)));
            if (time_axi < 3)
            {
                customPlot->xAxis->setRange(0, 3); // 保持10秒的数据
            }
            else
            {
                customPlot->xAxis->setRange(time_axi - 3, time_axi);
            }
            customPlot->yAxis->setRange(min_value, max_vaule); // 保持10秒的数据
            customPlot->replot();
        }
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
            data.clear();
            data_time.clear();
            QPalette palette = connect_button->palette();
            palette.setColor(QPalette::Button, Qt::yellow); // 使用预定义的颜色常量
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
        time_axi = 0;              // 清除所有轴
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
    std::string name;
    data_mutex.lock();
    name = Data_io.write(data_time, data);
    data_mutex.unlock();
    if (name != "")
        std::cout << name << " save success" << std::endl;
    else
        std::cout << "save failed" << std::endl;
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
        time_axi = 0;              // 清除所有轴
        data_item_blocked = true;
        for (auto i = 0; i < data_item.size(); i++) /* 清除所有选项 */
        {
            data_item[i]->setCheckState(0, Qt::Unchecked); // 0 表示第一列，Qt::Checked 表示选中打勾
        }
        data_item_blocked = false;
        show_index.clear();
        if (read_doc_data.size() != 0)
        {
            /* 显示子项目 */
            Data_name->clear();
            int data_size = read_doc_data[0].rows();
            data_item.resize(data_size);
            line_color.clear();
            for (int i = 0; i < data_size; i++)
            {
                data_item[i] = new QTreeWidgetItem(Data_name);
                data_item[i]->setFlags(data_item[i]->flags() | Qt::ItemIsUserCheckable);
                data_item[i]->setCheckState(0, Qt::Unchecked);          // 0 表示第一列，Qt::Checked 表示选中打勾
                data_item[i]->setText(0, "Data " + QString::number(i)); // 设置第一列的文本
                line_color.push_back(randomColor());
            }
        }
        std::cout << "Offline Image Mode" << std::endl;
        offline_image_mode = true;
    }
    else if (connect_flag)
    {
        std::cout << "Please Disconnect" << std::endl;
        return;
    }
}
