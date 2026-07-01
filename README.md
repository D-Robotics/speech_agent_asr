English | [简体中文](./README_cn.md)

# speech_agent_asr

**speech_agent_asr** provides automatic speech recognition (ASR) functionality. The node creates/destroys ASR sessions based on VAD endpoint detection events, receives noise-reduced audio data, calls the ASR SDK (`libasr_sdk.so`) for speech recognition, and publishes recognition results (partial and final) to a ROS2 topic.

## How to Run

### 1. Build

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select speech_agent_asr
```

### 2. Launch

```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/asr_text
```

Note: since `hobot_llamacpp` subscribes to `/prompt_text`, if you plan to use the language model in `hobot_llamacpp`, launch with the following command instead:

```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/prompt_text
```

Once running, the node subscribes to the following topics and publishes recognition results.

### 3. Subscriptions and Publications

| Direction | Topic | Type | Description |
| --------- | ----- | ---- | ----------- |
| Subscribe | `/vad_data` | `audio_msg::msg::SmartAudioData` | VAD endpoint detection events |
| Subscribe | `/asr_data` | `audio_msg::msg::SmartAudioData` | Noise-reduced audio data |
| Publish | `/asr_text` | `std_msgs::msg::String` | ASR recognition results |

## Parameters

| Parameter | Description | Type | Default |
| --------- | ----------- | ---- | ------- |
| vad_sub_topic_name | VAD subscription topic | std::string | "/vad_data" |
| asr_sub_topic_name | ASR audio subscription topic | std::string | "/asr_data" |
| asr_pub_topic_name | ASR result publish topic | std::string | "/asr_text" |

## Key Technical Details

### ASR Session Management

```
VAD BOS → CreateAsrSession() → Create ASR session, flush lookback audio
       ↓
VAD MID → Continuously write audio
       ↓
VAD EOS → DestroyAsrSession() → finish + destroy ASR session
```

### Audio Lookback

A 2-frame (~160ms) lookback queue is maintained. When VAD BOS arrives, the ASR session is created first, then the queued audio data is flushed into the engine to avoid losing the beginning of speech.

### Dependencies

```
speech_agent_asr
  ├── rclcpp, std_msgs, audio_msg
  ├── libasr_sdk.so (dynamically loaded at runtime)
  ├── libspeech_agent_asr_bootstrap.so
  └── libdl
```