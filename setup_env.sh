#!/bin/bash
# source setup_env.sh
#
# Resolves the SDK root from $TROS_DISTRO (exported by
# /opt/tros/<distro>/setup.bash), so the same script works under both
# humble and jazzy without modification.

if [ -z "$TROS_DISTRO" ]; then
  echo "[setup_env.sh] TROS_DISTRO is not set. Please source the target" \
       "setup first, e.g.:" >&2
  echo "    source /opt/tros/humble/setup.bash" >&2
  echo "    source /opt/tros/jazzy/setup.bash" >&2
  return 1 2>/dev/null || exit 1
fi

_SDK_ROOT="/opt/tros/${TROS_DISTRO}/lib/speech_agent_asr"
export LD_LIBRARY_PATH="${_SDK_ROOT}/resource/engine/asr/rspp/bin:${_SDK_ROOT}/resource/engine/asr/rspp/thirdparty:${_SDK_ROOT}/resource/engine/asr:${_SDK_ROOT}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_PRELOAD="/usr/lib/aarch64-linux-gnu/libgomp.so.1"
export BLAS_NUM_THREADS=4
export OMP_NUM_THREADS=1
export OMP_WAIT_POLICY=PASSIVE
export FASTDDS_BUILTIN_TRANSPORTS=UDPv4  # 强制只使用 UDPv4