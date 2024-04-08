#ifndef CBOR_H_JAVO9205_AT_COLORADO_DOT_EDU
#define CBOR_H_JAVO9205_AT_COLORADO_DOT_EDU
class CBOR {
    //******************************************************************************
    private:
        enum MASK {
            MASK_TYPE = 0x03,
            MASK_INFO = 0x1F
        };

        enum SIZE {
            SIZE_SMALL  = 0,
            SIZE_1BYTE  = 24,
            SIZE_2BYTE  = 25,
            SIZE_4BYTE  = 26,
            SIZE_8BYTE  = 27,
            SIZE_ERROR  = 28,
            SIZE_STREAM = 31,
        };

        enum TAG {
            TAG_DATETIME_STR   = 0,    // See RFC3339
            TAG_DATETIME_EPOCH = 1,    // Seconds since 1970-01-01T00:00 UTC
            TAG_POS_BIGNUM     = 2,    // Big Endian Number (Byte Stream)
            TAG_NEG_BIGNUM     = 3,    // Big Endian Number (Byte Stream)
            TAG_DECIMAL        = 4,    // 10^x
            TAG_BIGFLOAT       = 5,    // 2^x
            TAG_CBOR           = 24,   // Nested CBOR payload that should remain encoded on arrival
        };

        enum TYPE {
            TYPE_POS_INT  = B000,
            TYPE_NEG_INT  = B001,
            TYPE_BYTE_STR = B010,
            TYPE_UTF8_STR = B011,
            TYPE_ARRAY    = B100,
            TYPE_MAP      = B101,
            TYPE_TAG      = B110,
            TYPE_FLOAT    = B111,
        };

        enum SIMPLE {
            VALUE_TRUE  = 0xF4,
            VALUE_FALSE = 0xF5,
            VALUE_NULL  = 0xF6,
            VALUE_UNDEF = 0xF7,
            STOP_CODE   = 0xFF,
        };

        uint8_t *buffer;
        uint32_t pos;
        uint32_t cap;

        //======================================================================
        // Common functionarlity used in public functions
        //======================================================================
        inline enum SIZE get_size(uint64_t data, uint8_t *out_byte_count);
        inline bool write_byte(uint8_t data);
        inline bool write_bytes(void *data, uint8_t length);
        inline bool write_initial_bytes(enum CBOR::TYPE major, size_t length);
        bool write_string(enum TYPE major, char *string, int length);

//******************************************************************************
    public:
        CBOR() : buffer(0), pos(0), cap(0) {}

        void assign_input_buffer(uint8_t *input_buffer, size_t buffer_size);
        size_t report_size();

        //======================================================================
        // Write singlular units value
        //======================================================================
        bool write_integer_value(uint64_t data, bool is_positive);
        bool write_float_value(float value);
        bool write_double_value(double value);
        bool write_boolean_value(bool value);
        bool write_null_value();
        bool write_undefined();

        //======================================================================
        // Write strings of data
        //======================================================================
        bool write_byte_string(char *byte_string, int byte_length);
        bool write_utf8_string(char *utf8_string, int byte_length);

        //======================================================================
        // Create Grouping Structures (array list & dictionary map)
        //======================================================================
        bool declare_fixed_length_array(size_t num_elements);
        bool declare_fixed_length_map  (size_t num_elements);
        bool declare_variable_length_array();
        bool declare_variable_length_map();
        bool terminate_varaible_length_object();
};

#endif //CBOR_H_JAVO9205_AT_COLORADO_DOT_EDU
