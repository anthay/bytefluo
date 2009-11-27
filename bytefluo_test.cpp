/*  bytefluo unit test
    Copyright (c) 2008,2009 Anthony C. Hay
    Distributed under the BSD license, see:
    http://creativecommons.org/licenses/BSD/ */

#include "bytefluo.h"

#include <iostream>


unsigned g_test_count;      // count of number of unit tests executed
unsigned g_fault_count;     // count of number of unit tests that fail

// write a message to std::cout if value != expected_value
#define TEST_EQUAL(value, expected_value)               \
    ++g_test_count;                                     \
    if (value != expected_value) {                      \
        std::cout                                       \
            << __FILE__ << '(' << __LINE__ << ") : "    \
            << " expected " << expected_value           \
            << ", got " << value                        \
            << '\n';                                    \
        ++g_fault_count;                                \
    }

#define TEST_EXCEPTION(expression, exception_expected)  \
    {                                                   \
        bool got_exception = false;                     \
        try {                                           \
            expression;                                 \
        }                                               \
        catch (const exception_expected &) {            \
            got_exception = true;                       \
        }                                               \
        TEST_EQUAL(got_exception, true);                \
    }                                                   \


typedef unsigned char   uint8_type;
typedef unsigned short  uint16_type;
typedef unsigned int    uint32_type;
typedef signed char     int8_type;
typedef signed short    int16_type;
typedef signed int      int32_type;


namespace RFC_4122 {
    
// more-or-less real-world example, borrowing types from RFC 4122

typedef unsigned long   unsigned32;
typedef unsigned short  unsigned16;
typedef unsigned char   unsigned8;
typedef unsigned char   byte;
    
typedef struct {
    unsigned32  time_low;
    unsigned16  time_mid;
    unsigned16  time_hi_and_version;
    unsigned8   clock_seq_hi_and_reserved;
    unsigned8   clock_seq_low;
    byte        node[6];
} uuid_t;

}//namespace RFC_4122


// read UUID structure from given bytefluo stream
RFC_4122::uuid_t read_common(bytefluo & buf)
{
    // note that this code will read the data in the correct byte order
    // regardless of the natural byte order of this system; also reading
    // the structure one field at a time makes it immune to possible
    // compiler-specific structure packing for machine word alignment
    RFC_4122::uuid_t result;
    buf >> result.time_low
        >> result.time_mid
        >> result.time_hi_and_version
        >> result.clock_seq_hi_and_reserved
        >> result.clock_seq_low;
    buf.read(result.node, sizeof(result.node));
    return result;
}

// read big-endian encoded UUID structure from given 16-byte buffer
RFC_4122::uuid_t uuid_from_16_bytes_big(const unsigned char * bytes)
{
    bytefluo buf(bytes, bytes + 16, bytefluo::big);
    return read_common(buf);
}
    
// read little-endian encoded UUID structure from given 16-byte buffer
RFC_4122::uuid_t uuid_from_16_bytes_little(const unsigned char * bytes)
{
    bytefluo buf(bytes, bytes + 16, bytefluo::little);
    return read_common(buf);
}

void test_more_or_less_real_world_example()
{
    // let's say you wanted to read a UUID encoded in 16 contiguous bytes...
    const unsigned char bytes[16] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    using RFC_4122::uuid_t;
    
    // first read raw data assuming big-endian byte ordering
    {
        const uuid_t uuid(uuid_from_16_bytes_big(bytes));
        TEST_EQUAL(uuid.time_low,                   0x00112233);
        TEST_EQUAL(uuid.time_mid,                   0x4455);
        TEST_EQUAL(uuid.time_hi_and_version,        0x6677);
        TEST_EQUAL(uuid.clock_seq_hi_and_reserved,  0x88);
        TEST_EQUAL(uuid.clock_seq_low,              0x99);
        TEST_EQUAL(::memcmp(uuid.node, "\xAA\xBB\xCC\xDD\xEE\xFF", 6), 0);
    }
    
    // now read the same raw data assuming little-endian byte ordering
    {
        const uuid_t uuid(uuid_from_16_bytes_little(bytes));
        TEST_EQUAL(uuid.time_low,                   0x33221100);
        TEST_EQUAL(uuid.time_mid,                   0x5544);
        TEST_EQUAL(uuid.time_hi_and_version,        0x7766);
        TEST_EQUAL(uuid.clock_seq_hi_and_reserved,  0x88);
        TEST_EQUAL(uuid.clock_seq_low,              0x99);
        TEST_EQUAL(::memcmp(uuid.node, "\xAA\xBB\xCC\xDD\xEE\xFF", 6), 0);
    }
}


void test_ctor_exceptions()
{
    const unsigned char raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    
    // some valid streams
    bytefluo(raw_data, raw_data + 1, bytefluo::big);
    bytefluo(raw_data, raw_data, bytefluo::big);
    
    // invalid stream where end precedes begin - exception expected
    TEST_EXCEPTION(
        bytefluo(raw_data, raw_data - 1, bytefluo::big),
        bytefluo_exception);
    
    // another valid stream
    bytefluo(raw_data, raw_data + 1, bytefluo::little);
    
    // invalid byte order specified - exception expected
    TEST_EXCEPTION(
        bytefluo(raw_data, raw_data + 1, (bytefluo::byte_order)99),
        bytefluo_exception);
    
    // (also test set_byte_order here)
    bytefluo buf(raw_data, raw_data, bytefluo::big);
    
    // the only valid byte order values
    buf.set_byte_order(bytefluo::big);
    buf.set_byte_order(bytefluo::little);
    
    // invalid byte order specified - exception expected
    TEST_EXCEPTION(
        buf.set_byte_order((bytefluo::byte_order)12345),
        bytefluo_exception);
}


void test_assignment()
{
    // raw_data is some arbitrary array of bytes
    const unsigned char raw_data[7] = {
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    bytefluo buf1(raw_data, raw_data + sizeof(raw_data), bytefluo::big);
    bytefluo buf2(buf1);
    
    TEST_EQUAL(buf1.tellg(), 0);
    TEST_EQUAL(buf2.tellg(), 0);
    
    uint8_type val8;
    buf1 >> val8;
    TEST_EQUAL(buf1.tellg(), 1);
    TEST_EQUAL(buf2.tellg(), 0);
    TEST_EQUAL(val8, 0x99);

    uint16_type val16;
    buf2 >> val16;
    TEST_EQUAL(buf1.tellg(), 1);
    TEST_EQUAL(buf2.tellg(), 2);
    TEST_EQUAL(val16, 0x99AA);

    uint32_type val32;
    buf1 >> val32;
    TEST_EQUAL(buf1.tellg(), 5);
    TEST_EQUAL(buf2.tellg(), 2);
    TEST_EQUAL(val32, 0xAABBCCDD);

    buf1 = buf2;                 // assignment
    TEST_EQUAL(buf1.tellg(), 2);
    TEST_EQUAL(buf2.tellg(), 2);

    buf2.set_byte_order(bytefluo::little);
    buf1 >> val32;
    TEST_EQUAL(buf1.tellg(), 6);
    TEST_EQUAL(buf2.tellg(), 2);
    TEST_EQUAL(val32, 0xBBCCDDEE);
    buf2 >> val32;
    TEST_EQUAL(buf1.tellg(), 6);
    TEST_EQUAL(buf2.tellg(), 6);
    TEST_EQUAL(val32, 0xEEDDCCBB);
}


void test_eos()
{
    // raw_data is some arbitrary array of bytes
    const unsigned char raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const long raw_data_len(static_cast<long>(sizeof(raw_data)));

    // empty stream
    {
        bytefluo buf(raw_data, raw_data, bytefluo::big);
        TEST_EQUAL(buf.eos(), true);
    }
    // stream of 1 byte
    {
        bytefluo buf(raw_data, raw_data + 1, bytefluo::big);
        TEST_EQUAL(buf.eos(), false);
        buf.seek_end(0); // move cursor to end of stream
        TEST_EQUAL(buf.eos(), true);
    }
    // multi-byte stream
    {
        bytefluo buf(raw_data, raw_data + raw_data_len, bytefluo::big);
        // eos() false while cursor not at end of stream
        for (long i = 0; i != raw_data_len; ++i) {
            TEST_EQUAL(buf.eos(), false);
            uint8_type val;
            buf >> val;
        }
        // eos() true when cursor at end of stream
        TEST_EQUAL(buf.eos(), true);
    }
}


void test_size()
{
    // raw_data is some arbitrary array of bytes
    const unsigned char raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const size_t raw_data_len(sizeof(raw_data));

    // empty stream
    {
        bytefluo buf(raw_data, raw_data, bytefluo::big);
        TEST_EQUAL(buf.size(), 0);
    }
    // stream of 1 byte
    {
        bytefluo buf(raw_data, raw_data + 1, bytefluo::big);
        // getting the size does not affect the cursor location
        TEST_EQUAL(buf.size(), 1);
        TEST_EQUAL(buf.seek_current(0), 0);
        uint8_type val;
        buf >> val;
        // cursor location does not affect the size
        TEST_EQUAL(buf.seek_current(0), 1);
        TEST_EQUAL(buf.size(), 1);
    }
    // multi-byte stream
    {
        bytefluo buf(raw_data, raw_data + raw_data_len, bytefluo::big);
        TEST_EQUAL(buf.size(), raw_data_len);
    }
    // invalid stream where end precedes begin
    {
        TEST_EXCEPTION(bytefluo(raw_data, raw_data - 1, bytefluo::big), bytefluo_exception);
    }
}


void test_seek_current()
{
    // raw_data is some arbitrary array of bytes
    const unsigned char raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const long raw_data_len(static_cast<long>(sizeof(raw_data)));
    const uint8_type default_value(42); // value not found in raw_data
    uint8_type val;

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + sizeof(raw_data), bytefluo::big);

    // seek to end of data; seek_current() should return current offset
    TEST_EQUAL(buf.seek_current(raw_data_len), raw_data_len);
    TEST_EQUAL(buf.tellg(), static_cast<size_t>(raw_data_len));

    // should get exception if we now attempt to read
    TEST_EXCEPTION(buf >> val, bytefluo_exception);

    // seek to last byte of data and read it
    TEST_EQUAL(buf.seek_current(-1), raw_data_len - 1);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 7);

    // seek_current(0) gives you the current cursor position without moving it
    TEST_EQUAL(buf.seek_current(0), raw_data_len);
    // or you could just use tellg(), which also gives the current cursor pos
    TEST_EQUAL(buf.tellg(), static_cast<size_t>(raw_data_len));

    // you can't seek after the end
    TEST_EXCEPTION(buf.seek_current(1), bytefluo_exception);
    TEST_EQUAL(buf.seek_current(0), raw_data_len);

    // and you can't seek before the beginning
    TEST_EXCEPTION(buf.seek_current(-raw_data_len - 1), bytefluo_exception);
    TEST_EQUAL(buf.seek_current(0), raw_data_len);

    // but you can seek to the beginning and read the byte there
    TEST_EQUAL(buf.seek_current(-raw_data_len), 0);
    TEST_EQUAL(buf.seek_current(0), 0);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 1);
    TEST_EQUAL(buf.seek_current(0), 1);

    // and the middle
    TEST_EQUAL(buf.seek_current(3), 4);
    TEST_EQUAL(buf.seek_current(0), 4);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 5);
    TEST_EQUAL(buf.seek_current(0), 5);
    TEST_EQUAL(buf.tellg(), 5);
}


void test_seek_end()
{
    // raw_data is some arbitrary array of bytes
    const unsigned char raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const long raw_data_len(static_cast<long>(sizeof(raw_data)));
    const uint8_type default_value(42); // value not found in raw_data
    uint8_type val;

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + sizeof(raw_data), bytefluo::big);

    // seek to end of data; seek_end() should return current offset
    TEST_EQUAL(buf.seek_end(0), raw_data_len);

    // should get exception if we now attempt to read
    TEST_EXCEPTION(buf >> val, bytefluo_exception);

    // seek to last byte of data and read it
    TEST_EQUAL(buf.seek_end(1), raw_data_len - 1);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 7);

    // you can't seek after the end
    TEST_EXCEPTION(buf.seek_end(-1), bytefluo_exception);

    // and you can't seek before the beginning
    TEST_EXCEPTION(buf.seek_end(raw_data_len + 1), bytefluo_exception);

    // but you can seek to the beginning and read the byte there
    TEST_EQUAL(buf.seek_end(raw_data_len), 0);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 1);

    // and the middle
    TEST_EQUAL(buf.seek_end(3), 4);
    TEST_EQUAL(buf.tellg(), 4);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 5);
}


void test_seek_begin()
{
    // raw_data is some arbitrary array of bytes
    const unsigned char raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const long raw_data_len(static_cast<long>(sizeof(raw_data)));
    const uint8_type default_value(42); // value not found in raw_data
    uint8_type val;

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + sizeof(raw_data), bytefluo::big);

    // seek to end of data; seek_begin() should return current offset
    TEST_EQUAL(buf.seek_begin(raw_data_len), raw_data_len);

    // should get exception if we now attempt to read
    TEST_EXCEPTION(buf >> val, bytefluo_exception);

    // seek to last byte of data and read it
    TEST_EQUAL(buf.seek_begin(raw_data_len - 1), raw_data_len - 1);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 7);

    // you can't seek after the end
    TEST_EXCEPTION(buf.seek_begin(raw_data_len + 1), bytefluo_exception);

    // and you can't seek before the beginning
    TEST_EXCEPTION(buf.seek_begin(-1), bytefluo_exception);

    // but you can seek to the beginning and read the byte there
    TEST_EQUAL(buf.seek_begin(0), 0);
    TEST_EQUAL(buf.tellg(), 0);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 1);

    // and the middle
    TEST_EQUAL(buf.seek_begin(3), 3);
    TEST_EQUAL(buf.tellg(), 3);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 4);
}


void test_basic_vector_functionality()
{
    {
        std::vector<char> v;
        bytefluo buf(bytefluo_from_vector(v, bytefluo::big));
        TEST_EQUAL(buf.size(), 0);
        TEST_EQUAL(buf.eos(), true);
    }

    std::vector<unsigned char> vec;
    for (unsigned char i = 1; i < 8; ++i)
        vec.push_back(i);
    
    // create a bytefluo object to manage access to the contents of the
    // vec vector; let's say the bytes are in big-endian order
    bytefluo buf(bytefluo_from_vector(vec, bytefluo::big));
    
    // note that the bytefluo object, buf, does not contain a copy of
    // vec, it merely provides a convenient means to access vec

    TEST_EQUAL(buf.size(), vec.size());

    // read successive values from vec (via buf) and check them
    {
        uint8_type val = 0x99;
        buf >> val;
        TEST_EQUAL(val, 0x01);
    }
    {
        uint16_type val = 0x9999;
        buf >> val;
        TEST_EQUAL(val, 0x0203);
    }
    {
        uint32_type val = 0x99999999;
        buf >> val;
        TEST_EQUAL(val, 0x04050607);
    }

    // now rewind buf and tell it to read the data as little-endian
    buf.seek_begin(0);
    buf.set_byte_order(bytefluo::little);
    
    {
        uint8_type val = 0x99;
        buf >> val;
        TEST_EQUAL(val, 0x01);
    }
    {
        uint16_type val = 0x9999;
        buf >> val;
        TEST_EQUAL(val, 0x0302);
    }
    {
        uint32_type val = 0x99999999;
        buf >> val;
        TEST_EQUAL(val, 0x07060504);
    }
    
    // we are now at the end of the vec; check that attempting
    // to read any further will result in an exception being thrown
    {
        uint8_type val;
        TEST_EXCEPTION(buf >> val, bytefluo_exception);
    }

    // if you only need to read a few values you could just use the
    // temporary returned by bytefluo_from_vector() directly
    {
        uint16_type a;
        uint8_type b;
        uint32_type c;
        bytefluo_from_vector(vec, bytefluo::big) >> a >> b >> c;
        TEST_EQUAL(a, 0x0102);
        TEST_EQUAL(b, 0x03);
        TEST_EQUAL(c, 0x04050607);
    }
}


// demonstrate some typical usage
void test_basic_functionality()
{
    // raw_data is some arbitrary array of bytes
    const unsigned char raw_data[7] = {
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + sizeof(raw_data), bytefluo::big);

    // note that the bytefluo object, buf, does not contain a copy of
    // raw_data, it merely provides a convenient means to access raw_data
    // so you can do stuff like this
    uint16_type a;
    uint8_type b;
    uint32_type c;
    buf >> a >> b >> c;
    TEST_EQUAL(a, 0x99AA);
    TEST_EQUAL(b, 0xBB);
    TEST_EQUAL(c, 0xCCDDEEFF);

    // now move the cursor back to the start of the data and do it again
    buf.seek_begin(0);

    // read successive values from raw_data (via buf) and check them
    {
        uint8_type val = 0x12;
        buf >> val;
        TEST_EQUAL(val, 0x99);
    }
    {
        uint16_type val = 0x1212;
        buf >> val;
        TEST_EQUAL(val, 0xAABB);
    }
    {
        uint32_type val = 0x12121212;
        buf >> val;
        TEST_EQUAL(val, 0xCCDDEEFF);
    }

    // now rewind buf and tell it to read the data as little-endian
    buf.seek_begin(0);
    buf.set_byte_order(bytefluo::little);

    {
        uint8_type val = 0x12;
        buf >> val;
        TEST_EQUAL(val, 0x99);
    }
    {
        uint16_type val = 0x1212;
        buf >> val;
        TEST_EQUAL(val, 0xBBAA);
    }
    {
        uint32_type val = 0x12121212;
        buf >> val;
        TEST_EQUAL(val, 0xFFEEDDCC);
    }

    // we are now at the end of the raw_data; check that attempting
    // to read any further will result in an exception being thrown
    {
        TEST_EQUAL(buf.eos(), true);
        uint8_type val8;
        TEST_EXCEPTION(buf >> val8, bytefluo_exception);
        // the bytefluo buf is still in a well defined usable
        // state - as if no exception had been thrown - we
        // are still sitting at the end of raw_data
        TEST_EQUAL(buf.eos(), true);

        buf.seek_end(1);
        uint16_type val16;
        // can't read a 16-bit value when cursor is only 8 bits from buf end
        TEST_EXCEPTION(buf >> val16, bytefluo_exception);
        // still at 8 bits from buf end; we can read an 8-bit value
        buf >> val8;
        TEST_EQUAL(val8, 0xFF);
    }

    // now rewind buf and check it works with signed data too
    buf.seek_begin(0);
    {
        int8_type val(0);
        buf >> val;
        TEST_EQUAL(val, -103); // 0x99 8-bit two's complement = -103
    }
    {
        int16_type val(0);
        buf >> val;
        TEST_EQUAL(val, -17494); // 0xBBAA 16-bit two's complement = -17494
    }
    {
        int32_type val(0);
        buf >> val;
        TEST_EQUAL(val, -1122868); // 0xFFEEDDCC 32-bit 2's comp. = -1122868
    }

    // check copy construction and assignment
    TEST_EQUAL(buf.seek_begin(1), 1);
    {
        bytefluo buf_copy(buf);
        TEST_EQUAL(buf.size(), sizeof(raw_data));
        TEST_EQUAL(buf_copy.size(), sizeof(raw_data));
        TEST_EQUAL(buf.tellg(), 1);
        TEST_EQUAL(buf_copy.tellg(), 1);
        uint16_type val;
        buf >> val;
        TEST_EQUAL(val, 0xBBAA);
        val = 0;
        buf_copy >> val;
        TEST_EQUAL(val, 0xBBAA);

        bytefluo buf_assign(0, 0, bytefluo::big);
        TEST_EQUAL(buf_assign.size(), 0);
        buf_assign = buf;
        TEST_EQUAL(buf_assign.size(), sizeof(raw_data));
        buf_assign >> val;
        TEST_EQUAL(val, 0xDDCC);
    }
}



// bytefluo unit test
int main()
{
    test_basic_functionality();
    test_basic_vector_functionality();
    test_seek_begin();
    test_seek_end();
    test_seek_current();
    test_size();
    test_eos();
    test_assignment();
    test_ctor_exceptions();
    test_more_or_less_real_world_example();

    std::cout << "tests executed " << g_test_count;
    std::cout << ", tests failed " << g_fault_count << '\n';
	return g_fault_count ? EXIT_FAILURE : EXIT_SUCCESS;
}
// "Those who play with bytes will get bytten." - Jon Bentley

