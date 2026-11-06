[English](./README.md) | [简体中文](./README_cn.md)

# speech_agent_asr

**speech_agent_asr** provides Automatic Speech Recognition (ASR) functionality. The node receives denoised audio data, invokes the ASR SDK for speech recognition, and publishes the recognition results to a ROS2 topic.

## Usage

### 1. Build

`<TROS_DISTRO>` is the active TROS distribution — `humble` or `jazzy`. Replace it with the distro you have installed and sourced.

```bash
source /opt/tros/<TROS_DISTRO>/setup.bash
colcon build --packages-select speech_agent_asr
```

### 2. Launch

Manually download resource package and decompress to `/opt/tros/${TROS_DISTRO}/lib`

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_asr/speech_agent_asr_v1.2.0.tar.gz
tar -zxvf speech_agent_asr_v1.2.0.tar.gz -C /opt/tros/${TROS_DISTRO}/lib/
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
| enable_punct | Punctuation on final recognition results | bool | false |
| enable_itn | Chinese numeral inverse text normalization | bool | false |
| num_threads | Inference threads, default: 4, 0 = auto | int | 4 |
| cpu_affinity | CPU affinity, e.g. "0,1,2,3" or "0-3", empty = no binding | std::string | "" |
| hotwords_file | Hotwords JSON path, empty = disabled | std::string | "" |
| hotwords_score | Hotwords boost score (recommended 1.5-3.0) | double | 2.0 |

## Key Technical Details

### ASR Session Management (session reuse)

`sa_asr_create` is expensive (measured ~26 seconds), so the session is created **once at node
startup**; each utterance end clears state with `sa_asr_reset` and reuses the handle, and
`sa_asr_destroy` is only called at node shutdown:

```
node start → sa_init (load models) → sa_asr_create (once, ~26s)
       ↓
VAD BOS → flush lookback queue → start feeding
       ↓
VAD MID → continuously write audio (partial results published in real time)
       ↓
VAD EOS → sa_asr_finish (triggers final result) → sa_asr_reset (reuse session)
       ↓
next BOS → session reused directly (no re-creation cost)
       ↓
node exit → sa_asr_destroy → sa_destroy
```

Compatibility: if the `sa_asr_reset` symbol is missing or the call fails, the node falls back
to the legacy mode (destroy + re-create per utterance). A spurious EOS (without a matching BOS)
does not trigger finish/reset.

### Startup Latency

The node constructor blocks for about 30 seconds (model loading + session creation + first-run
online authorization) — a one-time cost; per-utterance recognition afterwards has no session
re-creation delay.

### Audio Lookback

A lookback queue of 3 frames (approximately 240 ms) is maintained. When VAD BOS arrives, the audio data from the lookback queue is flushed into the engine to avoid losing the audio at the beginning of speech.

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

### 4. Online Activation

- The first run requires network access for license activation; afterwards the SDK
  runs offline. The license is bound to the device — if lost, re-activation
  requires network access again.

### 5. Slow Node Startup (~30 seconds)

- This is expected: model loading + session creation (~26 s, one-time) + first-run online
  authorization (up to 30 s timeout)
- The startup log line `create asr session success, cost xxms` shows the session-creation cost
- If authorization hangs and `sa_init failed:-3` appears: check internet connectivity, or
  whether the license file has been deleted