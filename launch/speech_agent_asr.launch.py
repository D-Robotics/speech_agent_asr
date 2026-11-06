#!/usr/bin/env python3
# Copyright (c) 2024，D-Robotics.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from launch import LaunchDescription
from launch.actions import ExecuteProcess, DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    pkg_share = get_package_share_directory('speech_agent_asr')
    setup_env = os.path.join(pkg_share, 'setup_env.sh')

    vad_sub_topic = LaunchConfiguration('vad_sub_topic')
    asr_sub_topic = LaunchConfiguration('asr_sub_topic')
    asr_pub_topic = LaunchConfiguration('asr_pub_topic')
    enable_punct = LaunchConfiguration('enable_punct')
    enable_itn = LaunchConfiguration('enable_itn')
    num_threads = LaunchConfiguration('num_threads')
    cpu_affinity = LaunchConfiguration('cpu_affinity')
    hotwords_file = LaunchConfiguration('hotwords_file')
    hotwords_score = LaunchConfiguration('hotwords_score')

    return LaunchDescription([
        DeclareLaunchArgument(
            'vad_sub_topic', default_value='/vad_data',
            description='VAD data subscription topic'),
        DeclareLaunchArgument(
            'asr_sub_topic', default_value='/asr_data',
            description='ASR audio data subscription topic'),
        DeclareLaunchArgument(
            'asr_pub_topic', default_value='/asr_text',
            description='ASR result publication topic'),
        DeclareLaunchArgument(
            'enable_punct', default_value='false',
            description='Add punctuation to final ASR results'),
        DeclareLaunchArgument(
            'enable_itn', default_value='false',
            description='Chinese inverse text normalization on final results'),
        DeclareLaunchArgument(
            'num_threads', default_value='4',
            description='ASR inference threads, default:4, 0=auto'),
        DeclareLaunchArgument(
            'cpu_affinity', default_value='',
            description='ASR CPU affinity, e.g. "0,1,2,3" or "0-3"; empty = no binding'),
        DeclareLaunchArgument(
            'hotwords_file', default_value='',
            description='Hotwords JSON path; empty = disabled'),
        DeclareLaunchArgument(
            'hotwords_score', default_value='2.0',
            description='Hotwords boost score 1.5-3.0'),

        LogInfo(
            msg=["\n",
                 "========== speech_agent_asr.launch.py usage ==========\n",
                 "usage:\n",
                 "  ros2 launch speech_agent_asr speech_agent_asr.launch.py ",
                 "[<argument_name>:=<value> ...]\n",
                 "\n",
                 "arguments (all optional, pass as <name>:=<value>):\n",
                 "  vad_sub_topic  VAD data subscription topic,       default: '/vad_data'\n",
                 "  asr_sub_topic  ASR audio data subscription topic, default: '/asr_data'\n",
                 "  asr_pub_topic  ASR result publication topic,      default: '/asr_text'\n",
                 "  enable_punct   punctuation on final results,      default: 'false'\n",
                 "  enable_itn     chinese ITN on final results,      default: 'false'\n",
                 "  num_threads    inference threads, default:4, 0=auto, default: '4'\n",
                 "  cpu_affinity   e.g. '0,1,2,3' or '0-3', empty = no binding, default: ''\n",
                 "  hotwords_file  hotwords JSON, empty = disabled,   default: ''\n",
                 "  hotwords_score boost score 1.5-3.0,              default: '2.0'\n",
                 "\n",
                 "examples:\n",
                 "  # launch with all default values\n",
                 "  ros2 launch speech_agent_asr speech_agent_asr.launch.py\n",
                 "  # launch with a different ASR result publication topic\n",
                 "  ros2 launch speech_agent_asr speech_agent_asr.launch.py ",
                 "asr_pub_topic:=/prompt_text\n",
                 "\n",
                 "current values:\n",
                 "  vad_sub_topic=", vad_sub_topic, "\n",
                 "  asr_sub_topic=", asr_sub_topic, "\n",
                 "  asr_pub_topic=", asr_pub_topic, "\n",
                 "  enable_punct=", enable_punct, "  enable_itn=", enable_itn, "\n",
                 "  num_threads=", num_threads, "  cpu_affinity=", cpu_affinity, "\n",
                 "  hotwords_file=", hotwords_file, "  hotwords_score=", hotwords_score, "\n",
                 "=======================================================\n"]),

        ExecuteProcess(
            cmd=['bash', '-c',
                 'source ' + setup_env + ' && '
                 'exec ros2 run speech_agent_asr speech_agent_asr '
                 '--ros-args -p vad_sub_topic_name:="$1" -p asr_sub_topic_name:="$2" '
                 '-p asr_pub_topic_name:="$3" '
                 '-p enable_punct:="$4" -p enable_itn:="$5" -p num_threads:="$6" '
                 '${7:+-p cpu_affinity:=$7} ${8:+-p hotwords_file:=$8} '
                 '-p hotwords_score:="$9"',
                 'bash',
                 vad_sub_topic,
                 asr_sub_topic,
                 asr_pub_topic,
                 enable_punct,
                 enable_itn,
                 num_threads,
                 cpu_affinity,
                 hotwords_file,
                 hotwords_score],
            output='screen',
        ),
    ])