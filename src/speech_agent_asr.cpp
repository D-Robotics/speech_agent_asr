// Copyright (c) 2024，D-Robotics.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "speech_agent_asr/speech_agent_asr.h"
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace speech_agent_asr {
std::string getSysTimeMs()
{
    auto tp = std::chrono::system_clock::now();
    auto sec = std::chrono::system_clock::to_time_t(tp);
    auto dur = tp.time_since_epoch();
    // 总毫秒数对1000取余 = 当前毫秒
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() % 1000;

    std::tm t = *std::localtime(&sec);
    std::ostringstream oss;
    oss << std::put_time(&t,"%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

SpeechAgentASRNode::SpeechAgentASRNode(rclcpp::Node::SharedPtr& nh) : nh_(nh) {
  nh_->declare_parameter<std::string>("vad_sub_topic_name", vad_sub_topic_name_);
  nh_->declare_parameter<std::string>("asr_sub_topic_name", asr_sub_topic_name_);
  nh_->declare_parameter<std::string>("asr_pub_topic_name", asr_pub_topic_name_);

  nh_->get_parameter<std::string>("vad_sub_topic_name", vad_sub_topic_name_);
  nh_->get_parameter<std::string>("asr_sub_topic_name", asr_sub_topic_name_);
  nh_->get_parameter<std::string>("asr_pub_topic_name", asr_pub_topic_name_);

  vad_msg_subscription_ = nh_->create_subscription<audio_msg::msg::SmartAudioData>(
        vad_sub_topic_name_, 10,
        std::bind(&SpeechAgentASRNode::VadDataMessageCallback, this, std::placeholders::_1));
  asr_msg_subscription_ = nh_->create_subscription<audio_msg::msg::SmartAudioData>(
        asr_sub_topic_name_, 10,
        std::bind(&SpeechAgentASRNode::AsrDataMessageCallback, this, std::placeholders::_1));
  asr_msg_publisher_ = nh_->create_publisher<std_msgs::msg::String>(
        asr_pub_topic_name_, 10);

  std::string tros_distro
      = std::string(std::getenv("TROS_DISTRO")? std::getenv("TROS_DISTRO") : "");
  std::string speech_agent_resource = "/opt/tros/" + tros_distro + "/lib/speech_agent_asr";
  // 使用 speech_agent_bootstrap 替代原来的动态库加载和 sa_init
  lib_handle_ = speech_agent_asr_init(speech_agent_resource.c_str());
  if (!lib_handle_) {
    RCLCPP_ERROR(nh_->get_logger(), "speech_agent_asr_init failed");
    return;
  }
  p_sa_asr_create_ = (sa_asr_create_t)LOAD_SYMBOL(lib_handle_, "sa_asr_create");
  p_sa_asr_write_ = (sa_asr_write_t)LOAD_SYMBOL(lib_handle_, "sa_asr_write");
  p_sa_asr_finish_ = (sa_asr_finish_t)LOAD_SYMBOL(lib_handle_, "sa_asr_finish");
  p_sa_asr_destroy_ = (sa_asr_destroy_t)LOAD_SYMBOL(lib_handle_, "sa_asr_destroy");
  if (!p_sa_asr_create_ || !p_sa_asr_write_ || !p_sa_asr_finish_ || !p_sa_asr_destroy_) {
    RCLCPP_ERROR(nh_->get_logger(), "load asr symbol failed");
    return;
  }
  RCLCPP_INFO(nh_->get_logger(), "asr init success");
}

SpeechAgentASRNode::~SpeechAgentASRNode() {
  DestroyAsrSession();
  speech_agent_asr_deinit(lib_handle_);
  lib_handle_ = nullptr;
}

void SpeechAgentASRNode::CreateAsrSession() {
  if (session_inited_flag_) {
    RCLCPP_WARN(nh_->get_logger(), "asr session has been created");
    return;
  }
  if (!p_sa_asr_create_) {
    RCLCPP_ERROR(nh_->get_logger(), "sa_asr_create not loaded");
    return;
  }
  int ret = p_sa_asr_create_(&asr_handle_, AsrResultCallback, this);
  if (ret) {
    RCLCPP_ERROR(nh_->get_logger(), "sa_asr_create failed:%d", ret);
    return;
  }
  session_inited_flag_ = true;
  RCLCPP_INFO(nh_->get_logger(), "create asr session success");
}

void SpeechAgentASRNode::DestroyAsrSession() {
  if (!session_inited_flag_) {
    RCLCPP_WARN(nh_->get_logger(), "asr session has not been created");
    return;
  }
  if (p_sa_asr_finish_) {
    p_sa_asr_finish_(asr_handle_);
  }
  if (p_sa_asr_destroy_) {
    p_sa_asr_destroy_(asr_handle_);
  }
  session_inited_flag_ = false;
}

void SpeechAgentASRNode::VadDataMessageCallback(const audio_msg::msg::SmartAudioData::SharedPtr msg) {
  if ((msg->event_type.value == msg->event_type.EVENT_VADBEGIN) || (msg->event_type.value == msg->event_type.EVENT_VADMID)) {
    if (!session_inited_flag_) {
      CreateAsrSession();
      FillAudioData();
    }
    feed_audio_flag_ = true;
    RCLCPP_WARN(nh_->get_logger(), "=== vad begine");
  } else {
    feed_audio_flag_ = false;
    if (session_inited_flag_) {
      DestroyAsrSession();
      ClearAudioData();
    }
    RCLCPP_WARN(nh_->get_logger(), "vad end ==== ");
  }
}

void SpeechAgentASRNode::AsrDataMessageCallback(const audio_msg::msg::SmartAudioData::SharedPtr msg) {
  if (!feed_audio_flag_ || !session_inited_flag_) {
    // 回溯数据
    if (fill_queue_.size() >= MAX_FILL_QUEUE_SIZE_) {
      fill_queue_.pop();
    }
    std::vector<uint8_t> asr_data = msg->data;
    fill_queue_.push(asr_data);
    return;
  }
  RCLCPP_WARN(nh_->get_logger(), "feed audio");
  std::vector<uint8_t> asr_data = msg->data;
  if (p_sa_asr_write_) {
    int ret = p_sa_asr_write_(asr_handle_, asr_data.data(), asr_data.size());
    if (ret) {
      RCLCPP_WARN(nh_->get_logger(), "sa_asr_write failed:%d", ret);
    }
  }
}

void SpeechAgentASRNode::AsrResultCallback(const char* text, int is_final, void* user_data) {
  SpeechAgentASRNode* self = static_cast<SpeechAgentASRNode*>(user_data);
  if (text && text[0]) {
    auto asr_msg = std::make_unique<std_msgs::msg::String>();
    asr_msg->data = text;
    self->asr_msg_publisher_->publish(std::move(asr_msg));
    if (is_final) {
      RCLCPP_WARN(self->nh_->get_logger(), "asr final data callback:%s, %s", text, getSysTimeMs().c_str());
    } else {
      RCLCPP_WARN(self->nh_->get_logger(), "asr partial data callback:%s, %s", text, getSysTimeMs().c_str());
    }
  }
}

void SpeechAgentASRNode::FillAudioData() {
  while (!fill_queue_.empty() && session_inited_flag_) {
    std::vector<uint8_t> asr_data = fill_queue_.front();
    fill_queue_.pop();
    if (p_sa_asr_write_) {
      int ret = p_sa_asr_write_(asr_handle_, asr_data.data(), asr_data.size());
      if (ret) {
        RCLCPP_WARN(nh_->get_logger(), "sa_asr_write failed:%d", ret);
      }
    }
  }
}

void SpeechAgentASRNode::ClearAudioData() {
  if (!fill_queue_.empty()) {
    std::queue<std::vector<uint8_t>> empty_queue;
    std::swap(fill_queue_, empty_queue);
  }
}

}  // namespace speech_agent_asr
