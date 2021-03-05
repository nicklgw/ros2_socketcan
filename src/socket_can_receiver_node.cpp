// Copyright 2021 the Autoware Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Co-developed by Tier IV, Inc. and Apex.AI, Inc.

#include "ros2_socketcan/socket_can_receiver_node.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <utility>

namespace lc = rclcpp_lifecycle;
using LNI = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface;
using namespace std::chrono_literals;

namespace drivers
{
namespace socketcan
{
SocketCanReceiverNode::SocketCanReceiverNode(rclcpp::NodeOptions options)
: lc::LifecycleNode("socket_can_receiver_node", options)
{
  interface_ = this->declare_parameter("interface", "can0");
  double interval_sec = this->declare_parameter("interval_sec", 0.01);
  interval_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::duration<double>(interval_sec));

  RCLCPP_INFO(this->get_logger(), "interface: %s", interface_.c_str());
  RCLCPP_INFO(this->get_logger(), "interval(s): %f", interval_sec);
}

LNI::CallbackReturn SocketCanReceiverNode::on_configure(const lc::State & state)
{
  (void)state;

  try
  {
    receiver_ = std::make_unique<SocketCanReceiver>(interface_);
  } catch (const std::exception & ex) {
    RCLCPP_ERROR(this->get_logger(), "Error opening CAN receiver: %s - %s",
      interface_.c_str(), ex.what());
    return LNI::CallbackReturn::FAILURE;
  }

  RCLCPP_DEBUG(this->get_logger(), "Receiver successfully configured.");
  frames_pub_ = this->create_publisher<can_msgs::msg::Frame>("can_tx", 500);

  auto on_timer = std::bind(&SocketCanReceiverNode::receive, this);
  timer_ = std::make_shared<rclcpp::GenericTimer<decltype(on_timer)>>(
    this->get_clock(), interval_ns_, std::move(on_timer),
    this->get_node_base_interface()->get_context());
  this->get_node_timers_interface()->add_timer(timer_, nullptr);

  return LNI::CallbackReturn::SUCCESS;
}

LNI::CallbackReturn SocketCanReceiverNode::on_activate(const lc::State & state)
{
  (void)state;
  frames_pub_->on_activate();
  RCLCPP_DEBUG(this->get_logger(), "Receiver activated.");
  return LNI::CallbackReturn::SUCCESS;
}

LNI::CallbackReturn SocketCanReceiverNode::on_deactivate(const lc::State & state)
{
  (void)state;
  frames_pub_->on_deactivate();
  RCLCPP_DEBUG(this->get_logger(), "Receiver deactivated.");
  return LNI::CallbackReturn::SUCCESS;
}

LNI::CallbackReturn SocketCanReceiverNode::on_cleanup(const lc::State & state)
{
  (void)state;
  frames_pub_.reset();
  timer_.reset();
  RCLCPP_DEBUG(this->get_logger(), "Receiver cleaned up.");
  return LNI::CallbackReturn::SUCCESS;
}

LNI::CallbackReturn SocketCanReceiverNode::on_shutdown(const lc::State & state)
{
  (void)state;
  RCLCPP_DEBUG(this->get_logger(), "Receiver shutting down.");
  return LNI::CallbackReturn::SUCCESS;
}

void SocketCanReceiverNode::receive()
{
  CanId receive_id{};
  can_msgs::msg::Frame frame_msg(rosidl_runtime_cpp::MessageInitialization::ZERO);
  frame_msg.header.frame_id = "can";
  try {
    receive_id = receiver_->receive(frame_msg.data.data(), interval_ns_);
  } catch (const std::exception & ex) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
      "Error receiving CAN message: %s - %s",
      interface_.c_str(), ex.what());
    return;
  }
  frame_msg.header.stamp = this->now();
  frame_msg.id = receive_id.identifier();
  frame_msg.is_rtr = (receive_id.frame_type() == FrameType::REMOTE);
  frame_msg.is_extended = receive_id.is_extended();
  frame_msg.is_error = (receive_id.frame_type() == FrameType::ERROR);
  frame_msg.dlc = receive_id.length();
  frames_pub_->publish(std::move(frame_msg));
}

}  // namespace socketcan
}  // namespace drivers

RCLCPP_COMPONENTS_REGISTER_NODE(drivers::socketcan::SocketCanReceiverNode)
