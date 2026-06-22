#pragma once
#include <cstddef>
#include <cstdint>

namespace brick { namespace embedded {

struct EmbeddedFile {
    const char* name;
    const char* content;
    size_t length;
};

struct RuntimeInfo {
    const char* target;
    const char* cc;
    int num_sources;
    const EmbeddedFile* sources;
    int num_flags;
    const char* const* flags;
};

extern const EmbeddedFile _runtime_sources[];
extern const char* _runtime_flags[];
extern const RuntimeInfo runtime_info;

extern const char _runtime_block_memory_h[];
extern const size_t _runtime_block_memory_h_len;
extern const char _runtime_block_memory_c[];
extern const size_t _runtime_block_memory_c_len;
extern const char _runtime_io_h[];
extern const size_t _runtime_io_h_len;
extern const char _runtime_io_c[];
extern const size_t _runtime_io_c_len;
extern const char _runtime_hot_reload_h[];
extern const size_t _runtime_hot_reload_h_len;
extern const char _runtime_libs_window_window_h[];
extern const size_t _runtime_libs_window_window_h_len;
extern const char _runtime_libs_window_window_internal_h[];
extern const size_t _runtime_libs_window_window_internal_h_len;
extern const char _runtime_libs_window_window_hr_h[];
extern const size_t _runtime_libs_window_window_hr_h_len;
extern const char _runtime_hot_reload_c[];
extern const size_t _runtime_hot_reload_c_len;
extern const char _runtime_libs_window_window_linux_c[];
extern const size_t _runtime_libs_window_window_linux_c_len;
extern const char _runtime_libs_window_window_hr_c[];
extern const size_t _runtime_libs_window_window_hr_c_len;

} }
