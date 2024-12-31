# environment_set.sh
sudo apt install -y libtbb-dev
sudo apt install -y xcb


# Ubuntu/Debian
sudo apt-get -y install libxcb1 libxcb1-dev libx11-xcb1 libx11-xcb-dev libxcb-keysyms1 libxcb-keysyms1-dev libxcb-image0 libxcb-image0-dev libxcb-shm0 libxcb-shm0-dev libxcb-icccm4 libxcb-icccm4-dev libxcb-sync1 libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0 libxcb-render-util0-dev libxcb-glx0-dev libxcb-xinerama0-dev

# 安装OpenCV依赖
sudo apt install -y python3-opencv
sudo apt install -y libopencv-dev

# 使用pip安装OpenCV
pip3 install opencv-python
pip3 install opencv-contrib-python

sudo apt-get install -y qtbase5-dev qttools5-dev-tools

sudo apt install -y libxcb-*-dev
sudo apt install -y libx11-xcb-dev
sudo apt install -y libxkbcommon-x11-dev
