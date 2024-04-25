#include "src/cbor.h"
#define elif else if

//==============================================================================
void CBOR::assign_input_buffer(uint8_t *input, size_t size) {
  int i = 0;
  buffer = input;
  cap = size;
  pos = 0;
}

//==============================================================================
// Return true if data would exceed capacity.
inline bool CBOR::write_byte(uint8_t data) {
  if (pos >= cap)
    return true;
  buffer[pos++] = data;
  return false;
}

//==============================================================================
size_t CBOR::report_size() {
  return pos;
}

//==============================================================================
// @return True if value was written successfully
bool CBOR::write_integer_value(uint64_t data, bool unsigned_value) {
  enum TYPE major;
  major = (unsigned_value) ? TYPE_POS_INT : TYPE_NEG_INT;
  return write_initial_bytes(major, data);
}

//==============================================================================
// @return True if string was written successfully
bool CBOR::write_string(enum CBOR::TYPE major, char *string, int length) {
  const int LOOP_UNROLL_WIDTH = 16;
  uint8_t size, bytes, info;
  int i, j, k;
  bool error;
  int loop_unroll = (int) length - LOOP_UNROLL_WIDTH - 1;

  error = write_initial_bytes(major, length);

  if (error)
    return false;

  // Only check for errors once every 16 iterations (speed optimization)
  for (i = 0; i < loop_unroll; i += LOOP_UNROLL_WIDTH) {
    for (j = 0; j < LOOP_UNROLL_WIDTH; j++) {
      // Constant values will be constant-folded (AKA removed) by compiler
      k = i + j;
      error |= write_byte(string[k]);
      Serial.write(k);
      Serial.write(' ');
    }
    if (error)
      return false;
  }
  // Transmit remaining bytes that didn't end at a 16 byte boundary;
  while (i < length)
    error |= write_byte(string[i++]);

  // Return true if successfully transferred all data
  return !error;
}

//==============================================================================
bool CBOR::write_byte_string(char *byte_string, int byte_length) {
  const enum TYPE major = TYPE_BYTE_STR;
  return write_string(major, byte_string, byte_length);
}

//==============================================================================
bool CBOR::write_utf8_string(char *utf8_string, int byte_length) {
  const enum TYPE major = TYPE_UTF8_STR;
  return write_string(major, utf8_string, byte_length);
}

//==============================================================================
bool CBOR::declare_fixed_length_array(size_t num_elements) {
  const enum TYPE major = TYPE_ARRAY;
  return write_initial_bytes(major, num_elements);
}

//==============================================================================
bool CBOR::declare_fixed_length_map(size_t num_elements) {
  const enum TYPE major = TYPE_MAP;
  return write_initial_bytes(major, num_elements);
}

//==============================================================================
bool CBOR::declare_variable_length_array() {
  const enum TYPE major = TYPE_ARRAY;
  const enum SIZE info = SIZE_STREAM;
  return !write_byte((major << 5) | info);
}

//==============================================================================
bool CBOR::declare_variable_length_map() {
  const enum TYPE major = TYPE_MAP;
  const enum SIZE info = SIZE_STREAM;
  return !write_byte((major << 5) | info);
}

//==============================================================================
bool CBOR::terminate_variable_length_object() {
  return !write_byte(STOP_CODE);
}

//==============================================================================
bool CBOR::write_float_value(float value) {
  bool error = write_byte((TYPE_FLOAT << 5) | SIZE_4BYTE);
  error |= write_bytes((void *)(&value), 4);
  return !error;
}

//==============================================================================
bool CBOR::write_double_value(double value) {
  bool error = write_byte((TYPE_FLOAT << 5) | SIZE_8BYTE);
  error |= write_bytes((void *)(&value), 8);
  return !error;
}


//==============================================================================
bool CBOR::write_boolean_value(bool value) {
  return !write_byte(value ? VALUE_TRUE : VALUE_FALSE);
}

//==============================================================================
bool CBOR::write_null_value() {
  return !write_byte(VALUE_NULL);
}

//==============================================================================
bool CBOR::write_undefined() {
  return !write_byte(VALUE_UNDEF);
}

//==============================================================================
inline enum CBOR::SIZE CBOR::get_size(uint64_t data, uint8_t *bytes) {
  enum SIZE size;
  uint8_t throw_away;
  if (!bytes) bytes = &throw_away;

  if (data & 0xFFFFFFFF00000000UL) { size = SIZE_8BYTE; *bytes = 8; }
  else if (data & 0xFFFF0000U)     { size = SIZE_4BYTE; *bytes = 4; }
  else if (data & 0xFF00U)         { size = SIZE_2BYTE; *bytes = 2; }
  elif (data & 0xE0U)              { size = SIZE_1BYTE; *bytes = 1; }
  else                             { size = SIZE_SMALL; *bytes = 0; }

  return size;
}

//==============================================================================
inline bool CBOR::write_bytes(void *data, uint8_t length) {
  int i;
  bool error;
  uint8_t *bytes = (uint8_t *)(data);

  error = false;
  for (i = length-1; i >= 0; i--)
    error |= write_byte(bytes[i]);
  return !error;
}

//==============================================================================
inline bool CBOR::write_initial_bytes(enum CBOR::TYPE major, size_t length) {
  enum SIZE size;
  uint8_t bytes, info;
  bool error;

  bytes = 0;
  error = false;
  size = get_size(length, &bytes);
  info = (size == SIZE_SMALL) ? ((uint8_t)(length)) : size;
  error |= write_byte((major << 5) | info);
  error |= write_bytes((void *)(&length), bytes);
  return !error;
}

#undef elif

