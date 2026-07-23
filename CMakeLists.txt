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

cmake_minimum_required(VERSION 3.5)
project(speech_agent_asr)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(audio_msg REQUIRED)

include_directories(
  include
  src/asr_sdk/include
)

link_directories(
  lib/
)

set(SOURCES
  src/main.cpp
  src/speech_agent_asr.cpp
)

add_executable(speech_agent_asr ${SOURCES})
ament_target_dependencies(speech_agent_asr
  rclcpp
  std_msgs
  audio_msg
)
target_compile_definitions(speech_agent_asr PRIVATE
    SPDLOG_FMT_EXTERNAL=ON
)

# Link only asr_bootstrap — no transitive evad dependency
target_link_libraries(speech_agent_asr
  speech_agent_asr_bootstrap
  dl
)

install(TARGETS
  speech_agent_asr
  DESTINATION lib/${PROJECT_NAME})

install(PROGRAMS
  ${PROJECT_SOURCE_DIR}/setup_env.sh
  DESTINATION share/${PROJECT_NAME}/)

install(DIRECTORY
  ${PROJECT_SOURCE_DIR}/launch/
  DESTINATION share/${PROJECT_NAME}/launch)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()