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
from launch.actions import ExecuteProcess, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    pkg_share = get_package_share_directory('speech_agent_asr')
    setup_env = os.path.join(pkg_share, 'setup_env.sh')

    vad_sub_topic = LaunchConfiguration('vad_sub_topic')
    asr_sub_topic = LaunchConfiguration('asr_sub_topic')
    asr_pub_topic = LaunchConfiguration('asr_pub_topic')

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

        ExecuteProcess(
            cmd=['bash', '-c',
                 'source ' + setup_env + ' && '
                 'exec ros2 run speech_agent_asr speech_agent_asr '
                 '--ros-args -p vad_sub_topic_name:="$1" -p asr_sub_topic_name:="$2" -p asr_pub_topic_name:="$3"',
                 'bash',
                 vad_sub_topic,
                 asr_sub_topic,
                 asr_pub_topic],
            output='screen',
        ),
    ])