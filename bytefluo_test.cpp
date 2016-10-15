/*  bytefluo unit test is distributed under the MIT License:

        MIT License

        Copyright (c) 2008,2009,2010,2016 Anthony C. Hay

        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in all
        copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.
*/

#include "bytefluo.h"

#include <iostream>
#include <climits>
#include <chrono>


namespace {

unsigned g_test_count;      // count of number of unit tests executed
unsigned g_fault_count;     // count of number of unit tests that fail

// write a message to std::cout if value != expected_value
#define TEST_EQUAL(value, expected_value)               \
    ++g_test_count;                                     \
    if ((value) != (expected_value)) {                  \
        std::cout                                       \
            << __FILE__ << '(' << __LINE__ << ") : "    \
            << " expected " << (expected_value)         \
            << ", got " << (value)                      \
            << '\n';                                    \
        ++g_fault_count;                                \
    }

#define TEST_EXCEPTION(expression, exception_expected)  \
    {                                                   \
        bool got_exception = false;                     \
        try {                                           \
            expression;                                 \
        }                                               \
        catch (const bytefluo_exception & e) {          \
            got_exception = true;                       \
            TEST_EQUAL(e.id(), exception_expected);     \
        }                                               \
        TEST_EQUAL(got_exception, true);                \
    }                                                   \


class timer {
public:
    timer() : running_(true), start_time(std::chrono::high_resolution_clock::now()) {}

    void reset()
    {
        start_time = std::chrono::high_resolution_clock::now();
        running_ = true;
    }

    void stop()
    {
        stop_time = std::chrono::high_resolution_clock::now();
        running_ = false;
    }

    double elapsed_seconds()
    {
        using namespace std::chrono;
        return running_
            ? duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count()
            : duration_cast<duration<double>>(stop_time - start_time).count();
    }

    uint64_t elapsed_ms()
    {
        using namespace std::chrono;
        return running_
            ? duration_cast<milliseconds>(high_resolution_clock::now() - start_time).count()
            : duration_cast<milliseconds>(stop_time - start_time).count();
    }

private:
    bool running_;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point stop_time;
};


}

namespace rfc_4122 {
    
// more-or-less real-world example, borrowing uuid_t type from RFC 4122

typedef struct {
    uint32_t  time_low;
    uint16_t  time_mid;
    uint16_t  time_hi_and_version;
    uint8_t   clock_seq_hi_and_reserved;
    uint8_t   clock_seq_low;
    uint8_t   node[6];
} uuid_t;

}//namespace rfc_4122


// read UUID structure from given bytefluo stream
rfc_4122::uuid_t read_common(bytefluo & buf)
{
    // note that this code will read the data in the correct byte order
    // regardless of the natural byte order of this system; also reading
    // the structure one field at a time makes it immune to possible
    // compiler-specific structure packing for machine word alignment
    rfc_4122::uuid_t result;
    buf >> result.time_low
        >> result.time_mid
        >> result.time_hi_and_version
        >> result.clock_seq_hi_and_reserved
        >> result.clock_seq_low;
    buf.read(result.node, sizeof(result.node));
    return result;
}

// read big-endian encoded UUID structure from given 16-byte buffer
rfc_4122::uuid_t uuid_from_16_bytes_big(const uint8_t * bytes)
{
    bytefluo buf(bytes, bytes + 16, bytefluo::big);
    return read_common(buf);
}
    
// read little-endian encoded UUID structure from given 16-byte buffer
rfc_4122::uuid_t uuid_from_16_bytes_little(const uint8_t * bytes)
{
    bytefluo buf(bytes, bytes + 16, bytefluo::little);
    return read_common(buf);
}

void test_more_or_less_real_world_example()
{
    // let's say you wanted to read a UUID encoded in 16 contiguous bytes...
    const uint8_t bytes[16] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    using rfc_4122::uuid_t;
    
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
    const uint8_t raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    
    // some valid streams
    bytefluo(raw_data, raw_data + 1, bytefluo::big);
    bytefluo(raw_data, raw_data, bytefluo::big);
    
    // invalid stream where end precedes begin - exception expected
    TEST_EXCEPTION(
        bytefluo(raw_data + 1, raw_data, bytefluo::big),
        bytefluo_exception::end_precedes_begin);
    
    // invalid begin - exception expected
    TEST_EXCEPTION(
        bytefluo(0, raw_data, bytefluo::big),
        bytefluo_exception::null_begin_non_null_end);
    
    // invalid end - exception expected
    TEST_EXCEPTION(
        bytefluo(raw_data, 0, bytefluo::big),
        bytefluo_exception::null_end_non_null_begin);
    
    // another valid stream
    bytefluo(raw_data, raw_data + 1, bytefluo::little);
    
    // invalid byte order specified - exception expected
    TEST_EXCEPTION(
        bytefluo(raw_data, raw_data + 1, (bytefluo::byte_order)99),
        bytefluo_exception::invalid_byte_order);
    
    // (also test set_byte_order here)
    bytefluo buf(raw_data, raw_data, bytefluo::big);
    
    // the only valid byte order values
    buf.set_byte_order(bytefluo::big);
    buf.set_byte_order(bytefluo::little);
    
    // invalid byte order specified - exception expected
    TEST_EXCEPTION(
        buf.set_byte_order((bytefluo::byte_order)12345),
        bytefluo_exception::invalid_byte_order);
}


void test_assignment()
{
    // raw_data is some arbitrary array of bytes
    const uint8_t raw_data[7] = {
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    bytefluo buf1(raw_data, raw_data + sizeof(raw_data), bytefluo::big);
    bytefluo buf2(buf1);
    
    TEST_EQUAL(buf1.tellg(), 0);
    TEST_EQUAL(buf2.tellg(), 0);
    
    uint8_t val8;
    buf1 >> val8;
    TEST_EQUAL(buf1.tellg(), 1);
    TEST_EQUAL(buf2.tellg(), 0);
    TEST_EQUAL(val8, 0x99);

    uint16_t val16;
    buf2 >> val16;
    TEST_EQUAL(buf1.tellg(), 1);
    TEST_EQUAL(buf2.tellg(), 2);
    TEST_EQUAL(val16, 0x99AA);

    uint32_t val32;
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
    const uint8_t raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const size_t raw_data_len(sizeof(raw_data));

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
        for (size_t i = 0; i != raw_data_len; ++i) {
            TEST_EQUAL(buf.eos(), false);
            uint8_t val;
            buf >> val;
        }
        // eos() true when cursor at end of stream
        TEST_EQUAL(buf.eos(), true);
    }
}


void test_size()
{
    // raw_data is some arbitrary array of bytes
    const uint8_t raw_data[] = {
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
        uint8_t val;
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
        TEST_EXCEPTION(bytefluo(raw_data + 1, raw_data, bytefluo::big),
            bytefluo_exception::end_precedes_begin);
    }
    // stream constructed from empty vector
    {
        std::vector<uint32_t> vec;
        bytefluo buf(bytefluo_from_vector(vec, bytefluo::big));
        TEST_EQUAL(buf.size(), 0);
    }
    // stream constructed from vector of 8-bit values
    {
        std::vector<uint8_t> vec{1, 2, 3};
        bytefluo buf(bytefluo_from_vector(vec, bytefluo::big));
        TEST_EQUAL(buf.size(), 3); // there are 3 bytes in the vector
    }
    // stream constructed from vector of 32-bit values
    {
        std::vector<uint32_t> vec{1, 2, 3};
        bytefluo buf(bytefluo_from_vector(vec, bytefluo::big));
        TEST_EQUAL(buf.size(), 12); // there are 12 bytes in the vector
    }
}


void test_seek_current()
{
    // raw_data is some arbitrary array of bytes
    const uint8_t raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const long raw_data_len(static_cast<long>(sizeof(raw_data)));
    const uint8_t default_value(42); // value not found in raw_data
    uint8_t val;

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + sizeof(raw_data), bytefluo::big);

    // seek to end of data; seek_current() should return current offset
    TEST_EQUAL(buf.seek_current(raw_data_len), static_cast<size_t>(raw_data_len));
    TEST_EQUAL(buf.tellg(), static_cast<size_t>(raw_data_len));

    // should get exception if we now attempt to read
    TEST_EXCEPTION(buf >> val,
        bytefluo_exception::attempt_to_read_past_end);

    // seek to last byte of data and read it
    TEST_EQUAL(buf.seek_current(-1), raw_data_len - 1);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 7);

    // seek_current(0) gives you the current cursor position without moving it
    TEST_EQUAL(buf.seek_current(0), static_cast<size_t>(raw_data_len));
    // or you could just use tellg(), which also gives the current cursor pos
    TEST_EQUAL(buf.tellg(), static_cast<size_t>(raw_data_len));

    // you can't seek after the end
    TEST_EXCEPTION(buf.seek_current(1),
        bytefluo_exception::attempt_to_seek_after_end);
    TEST_EQUAL(buf.seek_current(0), static_cast<size_t>(raw_data_len));

    // and you can't seek before the beginning
    TEST_EXCEPTION(buf.seek_current(-raw_data_len - 1),
        bytefluo_exception::attempt_to_seek_before_beginning);
    TEST_EQUAL(buf.seek_current(0), static_cast<size_t>(raw_data_len));

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

    // you can't seek if the object is not not yet managing real data
    {
        bytefluo b;
        TEST_EXCEPTION(b.seek_current(1),
            bytefluo_exception::attempt_to_seek_after_end);
    }

    // test that very large out-of-range seeks are correctly detected
    TEST_EXCEPTION(buf.seek_current(LONG_MAX),
        bytefluo_exception::attempt_to_seek_after_end);
    TEST_EXCEPTION(buf.seek_current(LONG_MAX/2),
        bytefluo_exception::attempt_to_seek_after_end);
    TEST_EXCEPTION(buf.seek_current(LONG_MIN),
        bytefluo_exception::attempt_to_seek_before_beginning);
    TEST_EXCEPTION(buf.seek_current(LONG_MIN/2),
        bytefluo_exception::attempt_to_seek_before_beginning);
}


void test_seek_end()
{
    // raw_data is some arbitrary array of bytes
    const uint8_t raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const size_t raw_data_len(sizeof(raw_data));
    const uint8_t default_value(42); // value not found in raw_data
    uint8_t val;

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + raw_data_len, bytefluo::big);

    // seek to end of data; seek_end() should return current offset
    TEST_EQUAL(buf.seek_end(0), raw_data_len);

    // should get exception if we now attempt to read
    TEST_EXCEPTION(buf >> val,
        bytefluo_exception::attempt_to_read_past_end);

    // seek to last byte of data and read it
    TEST_EQUAL(buf.seek_end(1), raw_data_len - 1);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 7);

    // you can't seek after the end
    TEST_EXCEPTION(buf.seek_end(static_cast<size_t>(-1)),
        bytefluo_exception::attempt_to_seek_before_beginning);

    // and you can't seek before the beginning
    TEST_EXCEPTION(buf.seek_end(raw_data_len + 1),
        bytefluo_exception::attempt_to_seek_before_beginning);

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
    
    // you can't seek if the object is not not yet managing real data
    {
        bytefluo b;
        TEST_EXCEPTION(b.seek_end(1),
            bytefluo_exception::attempt_to_seek_before_beginning);
    }

    // test that very large out-of-range seeks are correctly detected
    TEST_EXCEPTION(buf.seek_end(ULONG_MAX),
        bytefluo_exception::attempt_to_seek_before_beginning);
    TEST_EXCEPTION(buf.seek_end(ULONG_MAX/2),
        bytefluo_exception::attempt_to_seek_before_beginning);
}


void test_seek_begin()
{
    // raw_data is some arbitrary array of bytes
    const uint8_t raw_data[] = {
        1, 2, 3, 4, 5, 6, 7
    };
    const size_t raw_data_len(sizeof(raw_data));
    const uint8_t default_value(42); // value not found in raw_data
    uint8_t val;

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + sizeof(raw_data), bytefluo::big);

    // seek to end of data; seek_begin() should return current offset
    TEST_EQUAL(buf.seek_begin(raw_data_len), raw_data_len);

    // should get exception if we now attempt to read
    TEST_EXCEPTION(buf >> val,
        bytefluo_exception::attempt_to_read_past_end);

    // seek to last byte of data and read it
    TEST_EQUAL(buf.seek_begin(raw_data_len - 1), raw_data_len - 1);
    val = default_value;
    buf >> val;
    TEST_EQUAL(val, 7);

    // you can't seek after the end
    TEST_EXCEPTION(buf.seek_begin(raw_data_len + 1),
        bytefluo_exception::attempt_to_seek_after_end);

    // and you can't seek before the beginning
    TEST_EXCEPTION(buf.seek_begin(static_cast<size_t>(-1)),
        bytefluo_exception::attempt_to_seek_after_end);

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
    
    // you can't seek if the object is not not yet managing real data
    {
        bytefluo b;
        TEST_EXCEPTION(b.seek_begin(1),
            bytefluo_exception::attempt_to_seek_after_end);
    }
    
    // test that very large out-of-range seeks are correctly detected
    TEST_EXCEPTION(buf.seek_begin(ULONG_MAX),
        bytefluo_exception::attempt_to_seek_after_end);
    TEST_EXCEPTION(buf.seek_begin(ULONG_MAX/2),
        bytefluo_exception::attempt_to_seek_after_end);
}


void test_basic_vector_functionality()
{
    {
        std::vector<char> v;
        bytefluo buf(bytefluo_from_vector(v, bytefluo::big));
        TEST_EQUAL(buf.size(), 0);
        TEST_EQUAL(buf.eos(), true);
    }

    {
        // a vector of 8-bit numbers
        std::vector<uint8_t> vec{1, 2, 3, 4, 5, 6, 7};
    
        // create a bytefluo object to manage access to the contents of the
        // vec vector; let's say the bytes are in big-endian order
        bytefluo buf(bytefluo_from_vector(vec, bytefluo::big));
    
        // note that the bytefluo object, buf, does not contain a copy of
        // vec, it merely provides a convenient means to access vec

        TEST_EQUAL(buf.size(), vec.size());

        // read successive values from vec (via buf) and check them
        {
            uint8_t val = 0x99;
            buf >> val;
            TEST_EQUAL(val, 0x01);
        }
        {
            uint16_t val = 0x9999;
            buf >> val;
            TEST_EQUAL(val, 0x0203);
        }
        {
            uint32_t val = 0x99999999;
            buf >> val;
            TEST_EQUAL(val, 0x04050607);
        }

        // now rewind buf and tell it to read the data as little-endian
        buf.seek_begin(0);
        buf.set_byte_order(bytefluo::little);
    
        {
            uint8_t val = 0x99;
            buf >> val;
            TEST_EQUAL(val, 0x01);
        }
        {
            uint16_t val = 0x9999;
            buf >> val;
            TEST_EQUAL(val, 0x0302);
        }
        {
            uint32_t val = 0x99999999;
            buf >> val;
            TEST_EQUAL(val, 0x07060504);
        }
    
        // we are now at the end of the vec; check that attempting
        // to read any further will result in an exception being thrown
        {
            uint8_t val;
            TEST_EXCEPTION(buf >> val,
                bytefluo_exception::attempt_to_read_past_end);
        }

        // if you only need to read a few values you could just use the
        // temporary returned by bytefluo_from_vector() directly
        {
            uint16_t a;
            uint8_t b;
            uint32_t c;
            bytefluo_from_vector(vec, bytefluo::big) >> a >> b >> c;
            TEST_EQUAL(a, 0x0102);
            TEST_EQUAL(b, 0x03);
            TEST_EQUAL(c, 0x04050607);
        }
    }

    {
        // a vector of 32-bit numbers
        std::vector<uint32_t> vec{0x11111111, 0x22222222, 0x33333333};
    
        // create a bytefluo object to manage access to the contents of the
        // vec vector; let's say the bytes are in big-endian order
        bytefluo buf(bytefluo_from_vector(vec, bytefluo::big));
        TEST_EQUAL(buf.size(), vec.size() * sizeof(vec[0]));

        uint8_t v8;
        v8 = 0x99;          buf >> v8;  TEST_EQUAL(v8, 0x11);
        v8 = 0x99;          buf >> v8;  TEST_EQUAL(v8, 0x11);
        v8 = 0x99;          buf >> v8;  TEST_EQUAL(v8, 0x11);
        v8 = 0x99;          buf >> v8;  TEST_EQUAL(v8, 0x11);

        uint16_t v16;
        v16 = 0x9999;       buf >> v16; TEST_EQUAL(v16, 0x2222);
        v16 = 0x9999;       buf >> v16; TEST_EQUAL(v16, 0x2222);

        uint32_t v32;
        v32 = 0x99999999;   buf >> v32; TEST_EQUAL(v32, 0x33333333);
    }
}


// demonstrate some typical usage
void test_basic_functionality()
{
    // raw_data is some arbitrary array of bytes
    const uint8_t raw_data[7] = {
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    // create a bytefluo object to manage access to raw_data
    // let's say the bytes in buf are in big-endian order
    bytefluo buf(raw_data, raw_data + sizeof(raw_data), bytefluo::big);
    // note that we could have achieved the exact same thing like this
    //   bytefluo buf;
    //   buf.set_data_range(raw_data, raw_data + sizeof(raw_data));
    //   buf.set_byte_order(bytefluo::big);

    // note that the bytefluo object, buf, does not contain a copy of
    // raw_data, it merely provides a convenient means to access raw_data
    // so you can do stuff like this
    uint16_t a;
    uint8_t b;
    uint32_t c;
    buf >> a >> b >> c;
    TEST_EQUAL(a, 0x99AA);
    TEST_EQUAL(b, 0xBB);
    TEST_EQUAL(c, 0xCCDDEEFF);

    // now move the cursor back to the start of the data and do it again
    buf.seek_begin(0);

    // read successive values from raw_data (via buf) and check them
    {
        uint8_t val = 0x12;
        buf >> val;
        TEST_EQUAL(val, 0x99);
    }
    {
        uint16_t val = 0x1212;
        buf >> val;
        TEST_EQUAL(val, 0xAABB);
    }
    {
        uint32_t val = 0x12121212;
        buf >> val;
        TEST_EQUAL(val, 0xCCDDEEFF);
    }

    // now rewind buf and tell it to read the data as little-endian
    buf.seek_begin(0);
    buf.set_byte_order(bytefluo::little);

    {
        uint8_t val = 0x12;
        buf >> val;
        TEST_EQUAL(val, 0x99);
    }
    {
        uint16_t val = 0x1212;
        buf >> val;
        TEST_EQUAL(val, 0xBBAA);
    }
    {
        uint32_t val = 0x12121212;
        buf >> val;
        TEST_EQUAL(val, 0xFFEEDDCC);
    }

    // rewind again and test non-scalar read (unaffected by byte order)
    buf.seek_begin(0);
    {
        uint8_t data3[3] = {0};
        buf.set_byte_order(bytefluo::little).read(data3, 3);
        TEST_EQUAL(static_cast<unsigned>(data3[0]), 0x99);
        TEST_EQUAL(static_cast<unsigned>(data3[1]), 0xAA);
        TEST_EQUAL(static_cast<unsigned>(data3[2]), 0xBB);
    }
    {
        uint8_t data4[4] = {0};
        buf.set_byte_order(bytefluo::big).read(data4, 4);
        TEST_EQUAL(static_cast<unsigned>(data4[0]), 0xCC);
        TEST_EQUAL(static_cast<unsigned>(data4[1]), 0xDD);
        TEST_EQUAL(static_cast<unsigned>(data4[2]), 0xEE);
        TEST_EQUAL(static_cast<unsigned>(data4[3]), 0xFF);
    }
    
    // we are now at the end of the raw_data; check that attempting
    // to read any further will result in an exception being thrown
    {
        TEST_EQUAL(buf.eos(), true);
        uint8_t val8;
        
        bool got_exception = false;
        try {
            buf >> val8;
        }
        catch (const bytefluo_exception & e) {
            TEST_EQUAL(e.id(), bytefluo_exception::attempt_to_read_past_end);
            got_exception = true;
        }
        TEST_EQUAL(got_exception, true);
        // from here on we'll use the TEST_EXCEPTION macro to do this
                   
        // at this point the bytefluo buf is still in a well defined
        // usable state - as if no exception had been thrown - we
        // are still sitting at the end of raw_data
        TEST_EQUAL(buf.eos(), true);

        buf.seek_end(1);
        uint16_t val16;
        // can't read a 16-bit value when cursor is only 8 bits from buf end
        TEST_EXCEPTION(buf >> val16,
            bytefluo_exception::attempt_to_read_past_end);
        // still at 8 bits from buf end; we can read an 8-bit value
        buf >> val8;
        TEST_EQUAL(val8, 0xFF);
    }

    // now rewind buf and check it works with signed data too
    buf.set_byte_order(bytefluo::little).seek_begin(0);
    {
        int8_t val(0);
        buf >> val;
        TEST_EQUAL(val, -103); // 0x99 8-bit two's complement = -103
    }
    {
        int16_t val(0);
        buf >> val;
        TEST_EQUAL(val, -17494); // 0xBBAA 16-bit two's complement = -17494
    }
    {
        int32_t val(0);
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
        uint16_t val;
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

    // check default constructor
    {
        bytefluo b;
        TEST_EQUAL(b.eos(), true);
        TEST_EQUAL(b.tellg(), 0);
        TEST_EQUAL(b.size(), 0);
        uint8_t val8;
        TEST_EXCEPTION(b >> val8,
            bytefluo_exception::attempt_to_read_past_end);

        // now give this object some data to manage
        b.set_data_range(raw_data, raw_data + sizeof(raw_data));
        uint16_t val16;
        b >> val16;
        TEST_EQUAL(val16, 0x99AA);
        TEST_EQUAL(b.eos(), false);
        TEST_EQUAL(b.tellg(), 2);
        TEST_EQUAL(b.size(), sizeof(raw_data));

        // show that (a) it won't accept end < begin, and
        // (b) the object is unchanged after throwing the exception
        TEST_EXCEPTION(
            b.set_data_range(raw_data + sizeof(raw_data), raw_data),
            bytefluo_exception::end_precedes_begin);
        TEST_EQUAL(b.eos(), false);
        TEST_EQUAL(b.tellg(), 2);
        TEST_EQUAL(b.size(), sizeof(raw_data));
        b >> val16;
        TEST_EQUAL(val16, 0xBBCC);
    }
    {
        // get some raw memory big enough to hold a bytefluo object
        const size_t s = (sizeof(bytefluo)+sizeof(char *)-1) / sizeof(char *);
        char * mem[s];
        // (use an array of pointers so allocated memory should be
        // correctly aligned for bytefluo, probably)
        for (size_t i = 0; i < s; ++i)
            mem[i] = reinterpret_cast<char *>(0xCCCCCCCC + i);
        // can now create a default-constructed bytefluo object
        // in memory with known state to test it does initialise itself
        bytefluo * b = new(mem) bytefluo;
        TEST_EQUAL(b->eos(), true);
        TEST_EQUAL(b->tellg(), 0);
        TEST_EQUAL(b->size(), 0);
        uint8_t val8;
        TEST_EXCEPTION(*b >> val8,
            bytefluo_exception::attempt_to_read_past_end);
        b->~bytefluo();
    }

    // test 64-bit reads
    {
        const uint8_t bytes[] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
        };
        bytefluo b(bytes, bytes + sizeof(bytes), bytefluo::big);
        uint64_t val64;

        b >> val64;
        TEST_EQUAL(val64, 0x0102030405060708ull);
        TEST_EQUAL(b.tellg(), 8);
        b >> val64;
        TEST_EQUAL(val64, 0x090A0B0C0D0E0F10ull);
        TEST_EQUAL(b.tellg(), 16);

        TEST_EXCEPTION(b >> val64, bytefluo_exception::attempt_to_read_past_end);

        b.seek_begin(1);
        b >> val64;
        TEST_EQUAL(val64, 0x0203040506070809ull);
        TEST_EQUAL(b.tellg(), 9);

        b.set_byte_order(bytefluo::little).seek_begin(1);
        b >> val64;
        TEST_EQUAL(val64, 0x0908070605040302ull);
        TEST_EQUAL(b.tellg(), 9);
    }
}


void test_performance()
{
    // some of what follows may look weird - it's an attempt to stop
    // the compiler optimising away the test code

    timer t;
    const int best_of_attempts = 5; // report only the best time seen
    const int repeats = 1000; // repeat inner loop this many times
    const size_t bytes_len = 1024 * 1024;
    uint64_t best_raw_read_ms, best_read_ms;

    // test 8-bit reads
    best_raw_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        std::vector<uint8_t> bytes(bytes_len);
        for (int b = 0; b < bytes_len; ++b)
            bytes[b] = uint8_t(rand());

        const int limit = int(bytes_len);
        uint8_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            const uint8_t * p = &bytes[0];
            for (int k = 0; k < limit; ++k) {
                uint8_t v;
                v = *p++;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_raw_read_ms)
            best_raw_read_ms = ms;
    }
    best_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        std::vector<uint8_t> bytes(bytes_len);
        for (int b = 0; b < bytes_len; ++b)
            bytes[b] = uint8_t(rand());

        bytefluo b(bytefluo_from_vector(bytes, bytefluo::little));
        const int limit = int(bytes_len);
        uint8_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            b.seek_begin(0);
            for (int k = 0; k < limit; ++k) {
                uint8_t v;
                b >> v;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_read_ms)
            best_read_ms = ms;
    }
    std::cout
        << "read " << (repeats * bytes_len) / (1024 * 1024)
        << " MiB via raw pointer to 8-bit " << best_raw_read_ms
        << " ms, via bytefluo " << best_read_ms
        << " ms (" << double(best_read_ms)/double(best_raw_read_ms)
        << ")\n";

    // test 16-bit reads
    best_raw_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        const int limit = int(bytes_len / 2);
        std::vector<uint16_t> bytes(limit);
        for (int b = 0; b < limit; ++b)
            bytes[b] = uint16_t(rand());

        uint16_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            const uint16_t * p = &bytes[0];
            for (int k = 0; k < limit; ++k) {
                uint16_t v;
                v = *p++;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_raw_read_ms)
            best_raw_read_ms = ms;
    }
    best_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        std::vector<uint8_t> bytes(bytes_len);
        for (int b = 0; b < bytes_len; ++b)
            bytes[b] = uint8_t(rand());

        bytefluo b(bytefluo_from_vector(bytes, bytefluo::little));
        const int limit = int(bytes_len / 2);
        uint16_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            b.seek_begin(0);
            for (int k = 0; k < limit; ++k) {
                uint16_t v;
                b >> v;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_read_ms)
            best_read_ms = ms;
    }
    std::cout
        << "read " << (repeats * bytes_len) / (1024 * 1024)
        << " MiB via raw pointer to 16-bit " << best_raw_read_ms
        << " ms, via bytefluo " << best_read_ms
        << " ms (" << double(best_read_ms)/double(best_raw_read_ms)
        << ")\n";

    // test 32-bit reads
    best_raw_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        const int limit = int(bytes_len / 4);
        std::vector<uint32_t> bytes(limit);
        for (int b = 0; b < limit; ++b)
            bytes[b] = uint32_t(rand());

        uint32_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            const uint32_t * p = &bytes[0];
            for (int k = 0; k < limit; ++k) {
                uint32_t v;
                v = *p++;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_raw_read_ms)
            best_raw_read_ms = ms;
    }
    best_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        std::vector<uint8_t> bytes(bytes_len);
        for (int b = 0; b < bytes_len; ++b)
            bytes[b] = uint8_t(rand());

        bytefluo b(bytefluo_from_vector(bytes, bytefluo::little));
        const int limit = int(bytes_len / 4);
        uint32_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            b.seek_begin(0);
            for (int k = 0; k < limit; ++k) {
                uint32_t v;
                b >> v;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_read_ms)
            best_read_ms = ms;
    }
    std::cout
        << "read " << (repeats * bytes_len) / (1024 * 1024)
        << " MiB via raw pointer to 32-bit " << best_raw_read_ms
        << " ms, via bytefluo " << best_read_ms
        << " ms (" << double(best_read_ms)/double(best_raw_read_ms)
        << ")\n";

    // test 64-bit reads
    best_raw_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        const int limit = int(bytes_len / 8);
        std::vector<uint64_t> bytes(limit);
        for (int b = 0; b < limit; ++b)
            bytes[b] = uint64_t(rand());

        uint64_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            const uint64_t * p = &bytes[0];
            for (int k = 0; k < limit; ++k) {
                uint64_t v;
                v = *p++;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_raw_read_ms)
            best_raw_read_ms = ms;
    }
    best_read_ms = 99999999;
    for (int attempt = 0; attempt < best_of_attempts; ++attempt) {
        std::vector<uint8_t> bytes(bytes_len);
        for (int b = 0; b < bytes_len; ++b)
            bytes[b] = uint8_t(rand());

        bytefluo b(bytefluo_from_vector(bytes, bytefluo::little));
        const int limit = int(bytes_len / 8);
        uint64_t x = 0;

        t.reset();
        for (int j = 0; j < repeats; ++j) {
            b.seek_begin(0);
            for (int k = 0; k < limit; ++k) {
                uint64_t v;
                b >> v;
                x += v;
            }
        }
        const uint64_t ms = t.elapsed_ms();

        if (x == ms)
            std::cout << "thank you for your patience\n";

        if (ms < best_read_ms)
            best_read_ms = ms;
    }
    std::cout
        << "read " << (repeats * bytes_len) / (1024 * 1024)
        << " MiB via raw pointer to 64-bit " << best_raw_read_ms
        << " ms, via bytefluo " << best_read_ms
        << " ms (" << double(best_read_ms)/double(best_raw_read_ms)
        << ")\n";
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

    //test_performance(); // uncomment to get some feel for performance

    return g_fault_count ? EXIT_FAILURE : EXIT_SUCCESS;
}
// "Those who play with bytes will get bytten." - Jon Bentley

