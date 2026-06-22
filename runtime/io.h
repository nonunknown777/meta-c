#ifndef BRICK_IO_H
#define BRICK_IO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char*  data;
    size_t len;
} BrickString;

void io_print_u8(uint8_t val);
void io_print_u16(uint16_t val);
void io_print_u32(uint32_t val);
void io_print_u64(uint64_t val);
void io_print_i8(int8_t val);
void io_print_i16(int16_t val);
void io_print_i32(int32_t val);
void io_print_i64(int64_t val);
void io_print_f32(float val);
void io_print_f64(double val);
void io_print_usize(size_t val);
void io_print_isize(ptrdiff_t val);
void io_print_int(int64_t val);
void io_print_float(double val);
void io_print_string(const char* data, size_t len);
void io_print_char(char val);
void io_print_bool(uint8_t val);
void io_print_newline(void);
void io_printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // BRICK_IO_H
     // BRICK_IO_H
