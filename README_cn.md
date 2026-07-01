[English](./README.md) | 简体中文

# speech_agent_asr

**speech_agent_asr** 提供语音识别（ASR）功能。节点根据 VAD 端点检测事件创建/销毁 ASR 会话，接收降噪后的音频数据，调用 ASR SDK（`libasr_sdk.so`）进行语音识别，并将识别结果（部分结果和最终结果）发布到 ROS2 topic。

## 运行方式

### 1. 下载资源包

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_asr/speech_agent_asr_v1.0.0.tar.gz
tar -zxvf speech_agent_asr_v1.0.0.tar.gz -C /opt/tros/humble/lib/
```

### 2. 编译

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select speech_agent_asr
```

### 3. 启动

```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/asr_text
```

注意：由于hobot_llamacpp订阅/prompt_text消息，所以如果后续使用hobot_llamacpp中的语言模型，则使用下面命令启动
```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/prompt_text
```

运行成功后，节点订阅如下 topic 并发布识别结果。

### 4. 订阅与发布

| 方向 | Topic | 类型 | 说明 |
| ---- | ----- | ---- | ---- |
| 订阅 | `/vad_data` | `audio_msg::msg::SmartAudioData` | VAD 端点检测事件 |
| 订阅 | `/asr_data` | `audio_msg::msg::SmartAudioData` | 降噪后音频数据 |
| 发布 | `/asr_text` | `std_msgs::msg::String` | ASR 识别结果|

## 参数列表

| 参数名 | 解释 | 类型 | 默认值 |
| ------ | ---- | ---- | ------ |
| vad_sub_topic_name | VAD 订阅 topic | std::string | "/vad_data" |
| asr_sub_topic_name | ASR 音频订阅 topic | std::string | "/asr_data" |
| asr_pub_topic_name | ASR 结果发布 topic | std::string | "/asr_text" |

## 关键技术细节

### ASR 会话管理

```
VAD BOS → CreateAsrSession() → 创建 ASR 会话，刷入回溯音频
       ↓
VAD MID → 持续写入音频
       ↓
VAD EOS → DestroyAsrSession() → finish + destroy ASR 会话
```

### 音频回溯

维护 2 帧（约 160ms）的回溯队列。VAD BOS 到达时，先创建 ASR 会话，再将回溯队列中的音频数据刷入引擎，避免丢失语音开头的音频。

### 依赖关系

```
speech_agent_asr
  ├── rclcpp, std_msgs, audio_msg
  ├── libasr_sdk.so (dynamically loaded at runtime)
  ├── libspeech_agent_asr_bootstrap.so
  └── libdl
```