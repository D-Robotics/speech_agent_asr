[English](./README.md) | 简体中文

# speech_agent_asr

**speech_agent_asr** 提供语音识别（ASR）功能。节点接收降噪后的音频数据，并调用 ASR SDK进行语音识别，并将识别结果发布到 ROS2 topic。

## 运行方式

### 1. 编译

`<TROS_DISTRO>` 为当前启用的 TROS 发行版 —— `humble` 或 `jazzy`，请替换为实际安装并 source 的发行版。

```bash
source /opt/tros/<TROS_DISTRO>/setup.bash
colcon build --packages-select speech_agent_asr
```

### 2. 启动

手动下载资源包，并解压到`/opt/tros/${TROS_DISTRO}/lib`目录下

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_asr/speech_agent_asr_v1.2.0.tar.gz
tar -zxvf speech_agent_asr_v1.2.0.tar.gz -C /opt/tros/${TROS_DISTRO}/lib/
```

- 启动

```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/asr_text
```

注意：由于hobot_llamacpp订阅/prompt_text消息，所以如果后续使用hobot_llamacpp中的语言模型，则使用下面命令启动
```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/prompt_text
```

运行成功后，节点订阅如下 topic 并发布识别结果。

### 3. 订阅与发布

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
| enable_punct | 最终识别结果加标点 | bool | false |
| enable_itn | 中文数字逆文本规整 | bool | false |
| num_threads | 推理线程数，default:4, 0=auto| int | 4 |
| cpu_affinity | 绑核配置，如"0,1,2,3"或"0-3"，空串=不绑核 | std::string | "" |
| hotwords_file | 热词 JSON 路径，空串=不启用 | std::string | "" |
| hotwords_score | 热词权重（建议 1.5-3.0） | double | 2.0 |

## 关键技术细节

### ASR 会话管理（会话复用）

`sa_asr_create` 开销大（实测约 26 秒），因此会话**仅在节点启动时创建一次**，
每轮会话结束用 `sa_asr_reset` 清空状态复用句柄，`sa_asr_destroy` 仅在节点退出时调用：

```
节点启动 → sa_init（加载模型）→ sa_asr_create（一次，约 26s）
       ↓
VAD BOS → 刷入回溯队列音频 → 开始喂音
       ↓
VAD MID → 持续写入音频（partial 结果实时发布）
       ↓
VAD EOS → sa_asr_finish（触发最终结果）→ sa_asr_reset（复用会话）
       ↓
下一轮 BOS → 直接续用会话（无重建开销）
       ↓
节点退出 → sa_asr_destroy → sa_destroy
```

兼容性：`sa_asr_reset` 符号缺失或调用失败时，自动回退为旧模式（每轮销毁并重建会话）。
孤立 EOS（无对应 BOS）不会触发 finish/reset。

### 启动耗时说明

节点构造函数阻塞约 30 秒（模型加载 + 会话创建 + 首次联网授权），属一次性开销；
之后每轮识别响应无会话重建延迟。

### 音频回溯

维护 3 帧（约 240ms）的回溯队列。VAD BOS 到达时，将回溯队列中的音频数据刷入引擎，
避免丢失语音开头的音频。

### 依赖关系

```
speech_agent_asr
  ├── rclcpp, std_msgs, audio_msg
  ├── libasr_sdk.so (dynamically loaded at runtime)
  ├── libspeech_agent_asr_bootstrap.so
  └── libdl
```

## 常见问题

### 1. 无识别结果

- 确认speech_agent_audio节点发送的音频数据是否正常
- 确认speech_agent_audio节点是否发送VAD BEGIN事件

### 2. 句首截断

- 确认是否唤醒词后的停顿时间是否过短

### 3. 句尾截断

- 确认语句中间的停顿时间是否过长

### 4. 联网激活

- 首次运行需联网激活授权，license 落盘后可离线运行，license 和设备绑定，丢失后需重新联网激活。

### 5. 节点启动慢（约 30 秒）

- 属正常现象：模型加载 + 会话创建（约 26 秒，一次性开销）+ 首次运行需联网授权（最长等待 30 秒）
- 启动日志 `create asr session success, cost xx ms` 可确认会话创建耗时
- 若卡在授权并报 `sa_init failed:-3`：检查外网连通性，或确认 license 文件未被删除