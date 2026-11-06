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

#ifndef SPEECH_AGENT_ASR_INCLUDE_H_
#define SPEECH_AGENT_ASR_INCLUDE_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "rclcpp/rclcpp.hpp"
#include "audio_msg/msg/smart_audio_data.hpp"
#include "std_msgs/msg/string.hpp"
#include "asr_sdk.h"
#include "dynamic_lib_loader.h"
#include "speech_agent_bootstrap/speech_agent_bootstrap.h"

namespace speech_agent_asr {

class SpeechAgentASRNode {
 public:
  SpeechAgentASRNode(rclcpp::Node::SharedPtr& nh);
  ~SpeechAgentASRNode();

  void OnGetText(const std_msgs::msg::String::SharedPtr msg);

 private:
  rclcpp::Node::SharedPtr nh_;
  rclcpp::Subscription<audio_msg::msg::SmartAudioData>::SharedPtr vad_msg_subscription_;
  std::string vad_sub_topic_name_ = "/vad_data";
  rclcpp::Subscription<audio_msg::msg::SmartAudioData>::SharedPtr asr_msg_subscription_;
  std::string asr_sub_topic_name_ = "/asr_data";
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr asr_msg_publisher_;
  std::string asr_pub_topic_name_ = "/asr_text";

  // 定义与头文件中签名完全一致的函数指针类型
  typedef int (*sa_asr_create_t)(sa_asr_handle*, sa_asr_callback_t, void*);
  typedef int (*sa_asr_write_t)(sa_asr_handle, const void*, size_t);
  typedef int (*sa_asr_finish_t)(sa_asr_handle);
  typedef void (*sa_asr_destroy_t)(sa_asr_handle);
  typedef int (*sa_asr_reset_t)(sa_asr_handle);

  void VadDataMessageCallback(const audio_msg::msg::SmartAudioData::SharedPtr msg);

  void AsrDataMessageCallback(const audio_msg::msg::SmartAudioData::SharedPtr msg);

  static void AsrResultCallback(const char* text, int is_final, void* user_data);

  void CreateAsrSession();

  void FinishAsrSession();

  void DestroyAsrSession();

  void FillAudioData();

  void ClearAudioData();

  std::atomic<bool> feed_audio_flag_{false};
  std::atomic<bool> session_inited_flag_{false};
  std::queue<std::vector<uint8_t>> fill_queue_;
  const std::size_t MAX_FILL_QUEUE_SIZE_{3};  // 回溯3帧asr数据，共240ms
  sa_asr_handle asr_handle_;
  sa_asr_create_t p_sa_asr_create_ = nullptr;
  sa_asr_write_t p_sa_asr_write_ = nullptr;
  sa_asr_finish_t p_sa_asr_finish_ = nullptr;
  sa_asr_destroy_t p_sa_asr_destroy_ = nullptr;
  sa_asr_reset_t p_sa_asr_reset_ = nullptr;
  LIB_HANDLE lib_handle_ = nullptr;
};

}  // namespace speech_agent_asr

#endif
