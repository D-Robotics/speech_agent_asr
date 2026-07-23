[English](./README.md) | [简体中文](./README_cn.md)

# speech_agent_asr

**speech_agent_asr** provides Automatic Speech Recognition (ASR) functionality. The node creates/destroys ASR sessions based on VAD endpoint detection events, receives denoised audio data, invokes the ASR SDK for speech recognition, and publishes the recognition results to a ROS2 topic.

## Usage

### 1. Build

- Build

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select speech_agent_asr
```

### 2. Launch

Manually download resource package and decompress to /opt/tros/humble/lib

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_asr/speech_agent_asr_v1.0.0.tar.gz
tar -zxvf speech_agent_asr_v1.0.0.tar.gz -C /opt/tros/humble/lib/
```

- launch

```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/asr_text
```

Note: Since `hobot_llamacpp` subscribes to the `/prompt_text` message, use the following command if you plan to use the language model from `hobot_llamacpp`:
```bash
ros2 launch speech_agent_asr speech_agent_asr.launch.py asr_pub_topic:=/prompt_text
```

After launching successfully, the node subscribes to the following topics and publishes recognition results.

### 3. Subscriptions and Publications

| Direction | Topic | Type | Description |
| --------- | ----- | ---- | ----------- |
| Subscription | `/vad_data` | `audio_msg::msg::SmartAudioData` | VAD endpoint detection events |
| Subscription | `/asr_data` | `audio_msg::msg::SmartAudioData` | Denoised audio data |
| Publication | `/asr_text` | `std_msgs::msg::String` | ASR recognition results |

## Parameters

| Parameter | Description | Type | Default |
| --------- | ----------- | ---- | ------- |
| vad_sub_topic_name | VAD subscription topic | std::string | "/vad_data" |
| asr_sub_topic_name | ASR audio subscription topic | std::string | "/asr_data" |
| asr_pub_topic_name | ASR result publication topic | std::string | "/asr_text" |

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

A lookback queue of 2 frames (approximately 160 ms) is maintained. When VAD BOS arrives, an ASR session is created first, then the audio data from the lookback queue is flushed into the engine to avoid losing the audio at the beginning of speech.

### Dependencies

```
speech_agent_asr
  ├── rclcpp, std_msgs, audio_msg
  ├── libasr_sdk.so (dynamically loaded at runtime)
  ├── libspeech_agent_asr_bootstrap.so
  └── libdl
```

## FAQ

### 1. No Recognition Results

- Verify that the audio data sent by the `speech_agent_audio` node is working correctly.
- Verify that the `speech_agent_audio` node is sending VAD BEGIN events.

### 2. Sentence-Initial Truncation

- Check whether the pause after the wake word is too short.

### 3. Mid-Sentence / Sentence-Final Truncation

- Check whether the pause in the middle of the utterance is too long.