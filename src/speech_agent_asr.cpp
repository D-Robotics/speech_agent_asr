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
#include <cstring>
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

  bool enable_punct = false;
  bool enable_itn = false;
  int num_threads = 4;
  std::string cpu_affinity;
  std::string hotwords_file;
  double hotwords_score = 2.0;
  nh_->declare_parameter<bool>("enable_punct", enable_punct);
  nh_->declare_parameter<bool>("enable_itn", enable_itn);
  nh_->declare_parameter<int>("num_threads", num_threads);
  nh_->declare_parameter<std::string>("cpu_affinity", cpu_affinity);
  nh_->declare_parameter<std::string>("hotwords_file", hotwords_file);
  nh_->declare_parameter<double>("hotwords_score", hotwords_score);

  nh_->get_parameter<bool>("enable_punct", enable_punct);
  nh_->get_parameter<bool>("enable_itn", enable_itn);
  nh_->get_parameter<int>("num_threads", num_threads);
  nh_->get_parameter<std::string>("cpu_affinity", cpu_affinity);
  nh_->get_parameter<std::string>("hotwords_file", hotwords_file);
  nh_->get_parameter<double>("hotwords_score", hotwords_score);

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
  // 注意：asr_options 还支持 auth_uri（自定义授权服务器地址，NULL=默认服务器）
  speech_agent_asr_options_t asr_options;
  memset(&asr_options, 0, sizeof(asr_options));
  asr_options.enable_punct = enable_punct ? 1 : 0;
  asr_options.enable_itn = enable_itn ? 1 : 0;
  asr_options.num_threads = num_threads;
  asr_options.cpu_affinity = cpu_affinity.empty() ? nullptr : cpu_affinity.c_str();
  asr_options.hotwords_file = hotwords_file.empty() ? nullptr : hotwords_file.c_str();
  asr_options.hotwords_score = static_cast<float>(hotwords_score);
  RCLCPP_INFO(nh_->get_logger(),
              "asr options: punct=%d itn=%d threads=%d affinity=%s hotwords=%s score=%.1f",
              asr_options.enable_punct, asr_options.enable_itn, asr_options.num_threads,
              asr_options.cpu_affinity ? asr_options.cpu_affinity : "auto",
              asr_options.hotwords_file ? asr_options.hotwords_file : "off",
              asr_options.hotwords_score);
  lib_handle_ = speech_agent_asr_init_ex(speech_agent_resource.c_str(), &asr_options);
  if (!lib_handle_) {
    RCLCPP_ERROR(nh_->get_logger(), "speech_agent_asr_init failed");
    return;
  }
  p_sa_asr_create_ = (sa_asr_create_t)LOAD_SYMBOL(lib_handle_, "sa_asr_create");
  p_sa_asr_write_ = (sa_asr_write_t)LOAD_SYMBOL(lib_handle_, "sa_asr_write");
  p_sa_asr_finish_ = (sa_asr_finish_t)LOAD_SYMBOL(lib_handle_, "sa_asr_finish");
  p_sa_asr_destroy_ = (sa_asr_destroy_t)LOAD_SYMBOL(lib_handle_, "sa_asr_destroy");
  p_sa_asr_reset_ = (sa_asr_reset_t)LOAD_SYMBOL(lib_handle_, "sa_asr_reset");
  if (!p_sa_asr_create_ || !p_sa_asr_write_ || !p_sa_asr_finish_ || !p_sa_asr_destroy_) {
    RCLCPP_ERROR(nh_->get_logger(), "load asr symbol failed");
    return;
  }
  if (!p_sa_asr_reset_) {
    RCLCPP_WARN(nh_->get_logger(), "sa_asr_reset not available, fallback to destroy per utterance");
  }
  RCLCPP_INFO(nh_->get_logger(), "asr init success");
  // 会话只创建一次：create 开销大，每轮会话结束用 sa_asr_reset清空状态并复用会话
  CreateAsrSession();
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
  auto create_start = std::chrono::steady_clock::now();
  int ret = p_sa_asr_create_(&asr_handle_, AsrResultCallback, this);
  auto create_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - create_start).count();
  if (ret) {
    RCLCPP_ERROR(nh_->get_logger(), "sa_asr_create failed:%d", ret);
    return;
  }
  session_inited_flag_ = true;
  RCLCPP_INFO(nh_->get_logger(), "create asr session success, cost %ld ms", create_ms);
}

void SpeechAgentASRNode::FinishAsrSession() {
  if (!session_inited_flag_) {
    return;
  }
  // finish 冲刷尾部音频并同步触发最终结果回调
  if (p_sa_asr_finish_) {
    p_sa_asr_finish_(asr_handle_);
  }
  // reset 清空会话状态但保留句柄，下一轮直接续用，省去昂贵的重复 create
  if (p_sa_asr_reset_) {
    int ret = p_sa_asr_reset_(asr_handle_);
    if (ret) {
      RCLCPP_WARN(nh_->get_logger(), "sa_asr_reset failed:%d, destroy session for rebuild", ret);
      if (p_sa_asr_destroy_) {
        p_sa_asr_destroy_(asr_handle_);
      }
      session_inited_flag_ = false;
      return;
    }
    RCLCPP_INFO(nh_->get_logger(), "asr session reset for next utterance");
    return;
  }
  // 无reset接口，则退回每轮销毁重建的旧模式
  RCLCPP_WARN(nh_->get_logger(), "sa_asr_reset not loaded, destroy session");
  if (p_sa_asr_destroy_) {
    p_sa_asr_destroy_(asr_handle_);
  }
  session_inited_flag_ = false;
}

void SpeechAgentASRNode::DestroyAsrSession() {
  if (!session_inited_flag_) {
    RCLCPP_WARN(nh_->get_logger(), "asr session has not been created");
    return;
  }
  // 说话中途退出时先冲刷出最终结果；空闲时 finish 已在上一轮 EOS 处理过
  if (feed_audio_flag_ && p_sa_asr_finish_) {
    p_sa_asr_finish_(asr_handle_);
  }
  if (p_sa_asr_destroy_) {
    p_sa_asr_destroy_(asr_handle_);
  }
  session_inited_flag_ = false;
}

void SpeechAgentASRNode::VadDataMessageCallback(const audio_msg::msg::SmartAudioData::SharedPtr msg) {
  if ((msg->event_type.value == msg->event_type.EVENT_VADBEGIN) || (msg->event_type.value == msg->event_type.EVENT_VADMID)) {
    // 会话常驻：仅当启动时创建失败或 reset 失败销毁后才重建
    if (!session_inited_flag_) {
      CreateAsrSession();
    }
    if (session_inited_flag_) {
      FillAudioData();
    }
    feed_audio_flag_ = true;
    RCLCPP_WARN(nh_->get_logger(), "=== vad begine");
  } else {
    // 仅结束真正开始过的会话：会话常驻后 session_inited_flag_ 恒为真，
    // 孤立的 EOS（无对应 BOS）不再触发多余的 finish+reset
    bool was_feeding = feed_audio_flag_;
    feed_audio_flag_ = false;
    if (was_feeding && session_inited_flag_) {
      FinishAsrSession();
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
    if (is_final) {
      auto asr_msg = std::make_unique<std_msgs::msg::String>();
      asr_msg->data = text;
      self->asr_msg_publisher_->publish(std::move(asr_msg));
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
