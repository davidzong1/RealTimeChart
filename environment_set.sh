# environment_set.sh
# 添加Qt仓库
sudo add-apt-repository ppa:beineri/opt-qt-6.5.3-jammy
# 更新包索引
sudo apt update


# 安装Qt6
sudo apt install -y qt6-base-dev
sudo apt install -y qt6-base-private-dev

# 安装Qt6开发工具
sudo apt install -y qt6-tools-dev-tools

# 安装Qt6开发工具
sudo apt install -y qt6-tools-dev-tools

# 安装Qt6开发工具
sudo apt install -y qt6-tools-dev-tools

# 安装Qt6开发工具
sudo apt install -y qt6-tools-dev-tools

# 安装Qt6开发工具
sudo apt install -y libtbb-dev
sudo apt install -y xcb
sudo apt-get install -y libtbb-dev
sudo apt-get install -y libxcb1-dev libxcb-glx0-dev libxcb-render0-dev \
    libxcb-render-util0-dev libx11-xcb-dev libxkbcommon-x11-dev
sudo apt-get install -y qt6-base-dev qt6-base-private-dev

# Ubuntu/Debian
sudo apt-get -y install libxcb1 libxcb1-dev libx11-xcb1 libx11-xcb-dev libxcb-keysyms1 libxcb-keysyms1-dev libxcb-image0 libxcb-image0-dev libxcb-shm0 libxcb-shm0-dev libxcb-icccm4 libxcb-icccm4-dev libxcb-sync1 libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0 libxcb-render-util0-dev libxcb-glx0-dev libxcb-xinerama0-dev

# 安装OpenCV依赖
sudo apt install -y python3-opencv
sudo apt install -y libopencv-dev

# 使用pip安装OpenCV
pip3 install opencv-python
pip3 install opencv-contrib-python

sudo apt-get install -y qtbase5-dev
sudo apt-get install -y qt6-base-dev

sudo apt install -y libxcb-*-dev
sudo apt install -y libx11-xcb-dev
sudo apt install -y libxkbcommon-x11-dev

# 获取当前路径并添加到环境变量
CURRENT_PATH=$(pwd)/QT_LIB/bin
echo "export PATH="$CURRENT_PATH:\${PATH}"" >> ~/.bashrc
echo "export PATH="$usr/.local/lib/python3.8\${PATH}"" >> ~/.bashrc

# 立即生效
source ~/.bashrc