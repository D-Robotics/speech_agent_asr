#!/bin/bash
# source setup_env.sh

_SDK_ROOT="/opt/tros/humble/lib/speech_agent_asr"
export LD_LIBRARY_PATH="${_SDK_ROOT}/resource/engine/asr/rspp/bin:${_SDK_ROOT}/resource/engine/asr/rspp/thirdparty:${_SDK_ROOT}/resource/engine/asr:${_SDK_ROOT}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_PRELOAD="/usr/lib/aarch64-linux-gnu/libgomp.so.1"
export BLAS_NUM_THREADS=4
export OMP_NUM_THREADS=1
export OMP_WAIT_POLICY=PASSIVE
export FASTDDS_BUILTIN_TRANSPORTS=UDPv4  # 强制只使用 UDPv4