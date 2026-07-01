// Copyright (c) 2024，D-Robotics.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SPEECH_AGENT_BOOTSTRAP_H_
#define SPEECH_AGENT_BOOTSTRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// VTN (Voice Trigger / Noise reduction engine)
// ============================================================================

/** Context for VTN engine, returned by speech_agent_vtn_init. */
typedef struct {
  void *vtn_handle;      /**< VTN handle for subsequent interact calls */
  void *proc_interact;   /**< Function pointer: vtn_api_interact(vtn_handle, info) */
  void *_internal;       /**< Reserved for internal use by speech_agent_bootstrap */
} speech_agent_vtn_ctx_t;

/**
 * @brief Initialize the VTN speech engine.
 *
 * Loads the appropriate libvtn.so based on mic_type, builds init params
 * with embedded credentials, and calls vtn_api_init.
 *
 * @param ctx           [out] VTN context (vtn_handle + proc_interact pointer)
 * @param callback_handler  [in]  Callback function for VTN events
 * @param callback_user_data [in]  User data passed through to callback
 * @param cfg_root_path  [in]  Root resource path (e.g. "/opt/tros/humble/lib/speech_agent_audio/")
 * @param mic_type       [in]  Mic type: "4mic", or "6mic"
 * @return 0 on success, -1 on failure
 */
int speech_agent_vtn_init(speech_agent_vtn_ctx_t *ctx,
                          void *callback_handler,
                          void *callback_user_data,
                          const char *cfg_root_path,
                          const char *mic_type);

/**
 * @brief Deinitialize the VTN engine.
 *
 * Calls vtn_api_destroy and unloads the library.
 *
 * @param ctx  [in] VTN context from speech_agent_vtn_init
 * @return 0 on success, -1 on failure
 */
int speech_agent_vtn_deinit(speech_agent_vtn_ctx_t *ctx);

// ============================================================================
// VAD (Voice Activity Detection)
// ============================================================================

/**
 * @brief Initialize the VAD engine.
 *
 * Constructs VADParams with embedded credentials, loads the VAD resource
 * file, and calls VADNew → VADAuth → VADStart.
 *
 * @param handle        [out] VAD handle for subsequent VAD operations
 * @param cb            [in]  VAD callback function (vad_audio_cb)
 * @param user_data     [in]  User data passed through to callback
 * @param cfg_root_path [in]  Root resource path (e.g. "/opt/tros/humble/lib/speech_agent_audio/")
 * @param eos_timeout   [in]  End-of-speech timeout in milliseconds
 * @return 0 on success, -1 on failure
 */
int speech_agent_vad_init(void **handle,
                          void *cb,
                          void *user_data,
                          const char *cfg_root_path,
                          int eos_timeout);

/**
 * @brief Deinitialize the VAD engine.
 *
 * Calls VADDestroy on the handle.
 *
 * @param handle  [in] VAD handle from speech_agent_vad_init
 * @return 0 on success, -1 on failure
 */
int speech_agent_vad_deinit(void *handle);

// ============================================================================
// ASR (Automatic Speech Recognition)
// ============================================================================

/**
 * @brief Initialize the ASR SDK.
 *
 * Loads libasr_sdk.so, builds sa_config_t with embedded credentials,
 * and calls sa_init().
 *
 * @param resource_path  [in]  Path to ASR resources (e.g. "/opt/tros/humble/lib/speech_agent_asr")
 * @return LIB_HANDLE on success, NULL on failure.
 *         The caller uses this handle to dlsym sa_asr_create, sa_asr_write, etc.
 */
void *speech_agent_asr_init(const char *resource_path);

/**
 * @brief Deinitialize the ASR SDK.
 *
 * Calls sa_destroy() and unloads libasr_sdk.so.
 * The caller must have already destroyed all ASR sessions before calling this.
 *
 * @param lib_handle  [in] LIB_HANDLE returned by speech_agent_asr_init
 */
void speech_agent_asr_deinit(void *lib_handle);

// ============================================================================
// TTS (Text-to-Speech)
// ============================================================================

/**
 * @brief Initialize the TTS SDK.
 *
 * Loads libtts_sdk.so, builds sa_config_t with embedded credentials,
 * and calls sa_init().
 *
 * @param resource_path  [in]  Path to TTS resources (e.g. "/opt/tros/humble/lib/speech_agent_tts")
 * @return LIB_HANDLE on success, NULL on failure.
 *         The caller uses this handle to dlsym sa_tts_create, sa_tts_synthesize, etc.
 */
void *speech_agent_tts_init(const char *resource_path);

/**
 * @brief Deinitialize the TTS SDK.
 *
 * Calls sa_destroy() and unloads libtts_sdk.so.
 * The caller must have already destroyed all TTS sessions before calling this.
 *
 * @param lib_handle  [in] LIB_HANDLE returned by speech_agent_tts_init
 */
void speech_agent_tts_deinit(void *lib_handle);

#ifdef __cplusplus
}
#endif

#endif  // SPEECH_AGENT_BOOTSTRAP_H_