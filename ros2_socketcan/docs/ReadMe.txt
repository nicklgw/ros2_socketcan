
sudo ip link set can0 up type can bitrate 500000
sudo ip link set can0 txqueuelen 1000
sudo ip link set up can0

sudo apt install ros-humble-can-msgs

使用这个socket_can_bridge
A ROS2 wrapper around Linux SocketCAN
https://github.com/autowarefoundation/ros2_socketcan.git

启动socket_can_bridge来负责实际的can通信，默认为can0，但此处需指定can1
ros2 launch ros2_socketcan socket_can_bridge.launch.xml interface:=can1


sudo install -Dm644 ros2_socketcan.service  /usr/lib/systemd/system/ros2_socketcan.service
systemctl enable ros2_socketcan.service
systemctl start ros2_socketcan.service
systemctl stop ros2_socketcan.service
systemctl disable ros2_socketcan.service

systemctl status ros2_socketcan.service
journalctl -n 100 -u ros2_socketcan.service

$ bloom-generate rosdebian
$ fakeroot debian/rules binary
