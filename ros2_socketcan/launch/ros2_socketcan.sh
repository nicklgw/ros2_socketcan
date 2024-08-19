#!/bin/bash

export ROS_DOMAIN_ID=5
source /opt/ros/humble/setup.bash
export ROS_LOCALHOST_ONLY=1
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

mkdir -m a=rwx -p /var/log/bzlrobot/ros2_socketcan
export ROS_LOG_DIR="/var/log/bzlrobot/ros2_socketcan"

/usr/bin/python3 /opt/ros/humble/bin/ros2 launch ros2_socketcan socket_can_bridge.launch.xml interface:=can1

