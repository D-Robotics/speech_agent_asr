#ifndef ASR_SDK_H
#define ASR_SDK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Error codes ====================
#define SA_OK                    0
#define SA_ERR_INVALID_PARAM    -1
#define SA_ERR_NOT_INITIALIZED  -2
#define SA_ERR_AUTH_FAILED      -3
#define SA_ERR_ENGINE_BUSY      -4
#define SA_ERR_ENGINE_INIT      -5
#define SA_ERR_ENGINE_FAILED    -6
#define SA_ERR_OUT_OF_MEMORY    -7
#define SA_ERR_CONFIG_LOAD      -8
#define SA_ERR_THREAD_CREATE    -9
#define SA_ERR_FILE_NOT_FOUND   -10

// Get error string from error code
const char* sa_error_string(int error_code);

// ==================== Configuration ====================
typedef struct {
    // Logging
    const char* log_file;          // Log file path (NULL for stdout)
    const char* log_level;         // "DEBUG", "INFO", "WARN", "ERROR"

    // ASR engine paths
    const char* asr_engine_path;   // Path to ASR engine directory
    const char* data_path;         // Path to data directory (hotwords, etc.)
    const char* hotwords_file;     // Path to hotwords JSON file (optional)

    // Authorization
    const char* auth_appid;        // Application ID (required)
    const char* auth_sn;           // Device serial number (optional, auto-detect if NULL)
    const char* auth_lib_path;     // Path to libauth.so (optional)
    const char* license_file;      // License file path (optional)
    const char* auth_uri;          // Auth server URI (optional, use default if NULL)

    // ASR parameters
    int num_threads;               // Number of threads (default: 4, 0=auto)
    const char* cpu_affinity;      // CPU affinity, e.g. "0,1,2,3" or "0-3" (NULL=no binding)
    int sample_rate;               // Sample rate in Hz (default: 16000)
    int enable_punct;              // Enable punctuation (0=off, 1=on)
    int enable_itn;                // Enable inverse text normalization (0=off, 1=on)
    float hotwords_score;          // Hotwords boost score (default: 2.0, range: 1.5-3.0)
} sa_config_t;

// SDK lifecycle
int  sa_init(const sa_config_t* config);
void sa_destroy(void);

// Get SDK version
const char* sa_version(void);

// ==================== ASR session ====================
// ASR callback function
// text: recognized text (partial or final)
// is_final: 1 if this is the final result for a speech segment, 0 for partial
// user_data: user-provided context pointer
typedef void (*sa_asr_callback_t)(const char* text, int is_final, void* user_data);

// ASR session handle (opaque)
typedef struct sa_asr_handle_t* sa_asr_handle;

// Create an ASR session
// handle: output handle pointer
// callback: callback function for recognition results
// user_data: user context passed to callback
int  sa_asr_create(sa_asr_handle* handle, sa_asr_callback_t callback, void* user_data);

// Write audio data to ASR session
// handle: ASR session handle
// pcm_data: PCM audio data (16kHz, 16-bit, mono, little-endian)
// pcm_len: length of audio data in bytes (must be even)
int  sa_asr_write(sa_asr_handle handle, const void* pcm_data, size_t pcm_len);

// Finish ASR session (flush remaining audio)
// handle: ASR session handle
int  sa_asr_finish(sa_asr_handle handle);

// Destroy ASR session
// handle: ASR session handle
void sa_asr_destroy(sa_asr_handle handle);

// Reset ASR session for reuse (clear state but keep handle)
// handle: ASR session handle
int  sa_asr_reset(sa_asr_handle handle);

#ifdef __cplusplus
}
#endif
#endif // ASR_SDK_H
