#ifndef BYTEFLUO_H_INCLUDED
#define BYTEFLUO_H_INCLUDED

/*  bytefluo
    Copyright (c) 2008,2009,2010,2016 Anthony C. Hay
    Distributed under the BSD license, see:
    http://creativecommons.org/licenses/BSD/

    Purpose:
    1. To read simple integer scalar values with a specified byte order
       from a buffer, regardless of the native byte order of this computer.
    2. To throw an exception if any attempt is made to access data beyond
       the specified bounds. */

#include <stdexcept>
#include <climits>
#include <cstring>
#include <vector>
#include <cstdint>

// the bytefluo class throws execptions of class bytefluo_exception
class bytefluo_exception : public std::runtime_error {
public:
    enum error_id {
        null_begin_non_null_end             = 1,
        null_end_non_null_begin             = 2,
        end_precedes_begin                  = 3,
        invalid_byte_order                  = 4,
        attempt_to_read_past_end            = 5,
        attempt_to_seek_after_end           = 6,
        attempt_to_seek_before_beginning    = 7,
    };
    
    bytefluo_exception(error_id id, const char * msg)
    : std::runtime_error(msg), id_(id)
    {
    }

    virtual ~bytefluo_exception() throw ()
    {
    }

    error_id id() const { return id_; }
    
private:
    error_id id_;
};


// manage specific byte-order read-only access to a given buffer
class bytefluo {
public:
    enum byte_order { // when reading a scalar...
        big,   // big endian: assume most-significant byte has lowest address
        little // little e.: assume least-significant byte has lowest address
    };

private:
    // impl provides compile-time selection of scalar size
    template <typename scalar_type, size_t sizeof_out>
    struct impl; // leave undefined; all supported scalar sizes are specialisations

    template <typename scalar_type>
    struct impl<scalar_type, 1> {
        // read one 8-bit value at the current cursor location; byte order is irrelevant
        static inline void read(scalar_type & out, const uint8_t *& cursor,
            const uint8_t * const buf_end, byte_order)
        {
            if (buf_end - cursor < 1)
                throw bytefluo_exception(
                    bytefluo_exception::attempt_to_read_past_end,
                    "bytefluo: attempt to read past end of data");

            out = scalar_type(*cursor++);
        }
    };

    template <typename scalar_type>
    struct impl<scalar_type, 2> {
        // read one 16-bit value at the current cursor location
        static inline void read(scalar_type & out, const uint8_t *& cursor,
            const uint8_t * const buf_end, byte_order buf_byte_order)
        {
            if (buf_end - cursor < 2)
                throw bytefluo_exception(
                    bytefluo_exception::attempt_to_read_past_end,
                    "bytefluo: attempt to read past end of data");

            if (buf_byte_order == little) {
                // cursor -> least significant byte
                out = scalar_type(uint16_t(cursor[1]) << CHAR_BIT | uint16_t(cursor[0]));
            }
            else {
                // cursor -> most significant byte
                out = scalar_type(uint16_t(cursor[0]) << CHAR_BIT | uint16_t(cursor[1]));
            }

            cursor += 2;
        }
    };

    template <typename scalar_type>
    struct impl<scalar_type, 4> {
        // read one 32-bit value at the current cursor location
        static inline void read(scalar_type & out, const uint8_t *& cursor,
            const uint8_t * const buf_end, byte_order buf_byte_order)
        {
            if (buf_end - cursor < 4)
                throw bytefluo_exception(
                    bytefluo_exception::attempt_to_read_past_end,
                    "bytefluo: attempt to read past end of data");

            if (buf_byte_order == little) {
                // cursor -> least significant byte
                auto o3 = uint32_t(cursor[3]) << CHAR_BIT * 3;
                auto o2 = uint32_t(cursor[2]) << CHAR_BIT * 2;
                auto o1 = uint32_t(cursor[1]) << CHAR_BIT;
                auto o0 = uint32_t(cursor[0]);
                out = scalar_type(o3 | o2 | o1 | o0);
            }
            else {
                // cursor -> most significant byte
                auto o3 = uint32_t(cursor[0]) << CHAR_BIT * 3;
                auto o2 = uint32_t(cursor[1]) << CHAR_BIT * 2;
                auto o1 = uint32_t(cursor[2]) << CHAR_BIT;
                auto o0 = uint32_t(cursor[3]);
                out = scalar_type(o3 | o2 | o1 | o0);
            }

            cursor += 4;
        }
    };

    template <typename scalar_type>
    struct impl<scalar_type, 8> {
        // read one 64-bit value at the current cursor location
        static inline void read(scalar_type & out, const uint8_t *& cursor,
            const uint8_t * const buf_end, byte_order buf_byte_order)
        {
            if (buf_end - cursor < 8)
                throw bytefluo_exception(
                    bytefluo_exception::attempt_to_read_past_end,
                    "bytefluo: attempt to read past end of data");

            if (buf_byte_order == little) {
                // cursor -> least significant byte
                auto h3 = uint32_t(cursor[7]) << CHAR_BIT * 3;
                auto h2 = uint32_t(cursor[6]) << CHAR_BIT * 2;
                auto h1 = uint32_t(cursor[5]) << CHAR_BIT;
                auto h0 = uint32_t(cursor[4]);
                auto h = h3 | h2 | h1 | h0;
                auto l3 = uint32_t(cursor[3]) << CHAR_BIT * 3;
                auto l2 = uint32_t(cursor[2]) << CHAR_BIT * 2;
                auto l1 = uint32_t(cursor[1]) << CHAR_BIT;
                auto l0 = uint32_t(cursor[0]);
                auto l = l3 | l2 | l1 | l0;
                out = scalar_type(uint64_t(h) << CHAR_BIT * 4 | uint64_t(l));
            }
            else {
                // cursor -> most significant byte
                auto h3 = uint32_t(cursor[0]) << CHAR_BIT * 3;
                auto h2 = uint32_t(cursor[1]) << CHAR_BIT * 2;
                auto h1 = uint32_t(cursor[2]) << CHAR_BIT;
                auto h0 = uint32_t(cursor[3]);
                auto h = h3 | h2 | h1 | h0;
                auto l3 = uint32_t(cursor[4]) << CHAR_BIT * 3;
                auto l2 = uint32_t(cursor[5]) << CHAR_BIT * 2;
                auto l1 = uint32_t(cursor[6]) << CHAR_BIT;
                auto l0 = uint32_t(cursor[7]);
                auto l = l3 | l2 | l1 | l0;
                out = scalar_type(uint64_t(h) << CHAR_BIT * 4 | uint64_t(l));
            }

            cursor += 8;
        }
    };


public:
    // default to empty range [0, 0), big-endian
    bytefluo()
    : buf_begin(0), buf_end(0), cursor(0), buf_byte_order(big)
    {
    }
    
    // bytefluo will manage access to bytes in [begin, end)
    // and scalar reads will assume given byte order 'bo'
    bytefluo(
        const void * begin,
        const void * end,
        byte_order bo)
    : buf_begin(static_cast<const uint8_t *>(begin)),
      buf_end  (static_cast<const uint8_t *>(end)),
      cursor   (static_cast<const uint8_t *>(begin)),
      buf_byte_order(bo)
    {
        validate(begin, end);
        if (buf_byte_order != big && buf_byte_order != little)
            throw bytefluo_exception(bytefluo_exception::invalid_byte_order,
                "bytefluo: invalid byte order");
    }

    // bytefluo will manage access to bytes in [begin, end)
    bytefluo & set_data_range(const void * begin, const void * end)
    {
        validate(begin, end);
        cursor    =
        buf_begin = static_cast<const uint8_t *>(begin);
        buf_end   = static_cast<const uint8_t *>(end);
        return *this;
    }
    
    // specify the byte order to be used on subsequent scalar reads
    bytefluo & set_byte_order(byte_order bo)
    {
        if (bo != big && bo != little)
            throw bytefluo_exception(bytefluo_exception::invalid_byte_order,
                "bytefluo: invalid byte order");
        buf_byte_order = bo;
        return *this;
    }

    // read an integer scalar value from buffer at current cursor position
    template <typename scalar_type>
    bytefluo & operator>>(scalar_type & out)
    {
        impl<scalar_type, sizeof(scalar_type)>::read(out, cursor, buf_end, buf_byte_order);
        return *this;
    }

    // copy 'len' bytes from buffer at current cursor position to given 'dest'
    // note that current buf_byte_order has no affect on this operation
    bytefluo & read(void * dest, size_t len)
    {
        if (buf_end - cursor < static_cast<ptrdiff_t>(len))
            throw bytefluo_exception(
                bytefluo_exception::attempt_to_read_past_end,
                "bytefluo: attempt to read past end of data");
        ::memcpy(dest, cursor, len);
        cursor += len;
        return *this;
    }

    // move cursor 'pos' bytes from stream beginning
    size_t seek_begin(size_t pos)
    {
        if (pos > static_cast<size_t>(buf_end - buf_begin))
            throw bytefluo_exception(
                bytefluo_exception::attempt_to_seek_after_end,
                "bytefluo: attempt to seek after end of data");
        cursor = buf_begin + pos;
        return static_cast<size_t>(cursor - buf_begin);
    }

    // move cursor 'pos' bytes from current position
    size_t seek_current(long pos)
    {
        if (pos < 0) { // seek backward
            if (pos < static_cast<long>(buf_begin - cursor))
                throw bytefluo_exception(
                    bytefluo_exception::attempt_to_seek_before_beginning,
                    "bytefluo: attempt to seek before beginning of data");
        }
        else { // seek forward (or nowhere)
            if (pos > static_cast<long>(buf_end - cursor))
                throw bytefluo_exception(
                    bytefluo_exception::attempt_to_seek_after_end,
                    "bytefluo: attempt to seek after end of data");
        }
        cursor += pos;
        return static_cast<size_t>(cursor - buf_begin);
    }

    // move cursor 'pos' bytes from stream end
    size_t seek_end(size_t pos)
    {
        if (pos > static_cast<size_t>(buf_end - buf_begin))
            throw bytefluo_exception(
                bytefluo_exception::attempt_to_seek_before_beginning,
                "bytefluo: attempt to seek before beginning of data");
        cursor = buf_end - pos;
        return static_cast<size_t>(cursor - buf_begin);
    }

    // return true iff cursor is at end of stream
    bool eos() const
    {
        return cursor == buf_end;
    }

    // return number of bytes in stream
    size_t size() const
    {
        return static_cast<size_t>(buf_end - buf_begin);
    }

    // return distance from buffer start to cursor
    size_t tellg() const
    {
        return static_cast<size_t>(cursor - buf_begin);
    }
    
private:
    const uint8_t * buf_begin;
    const uint8_t * buf_end;
    const uint8_t * cursor;
    byte_order buf_byte_order;

    // throw an exception if given buffer limits are obviously bad
    void validate(const void * begin, const void * end)
    {
        if (begin == 0 && end != 0)
            throw bytefluo_exception(
                bytefluo_exception::null_begin_non_null_end,
                "bytefluo: begin is 0, end isn't");
        if (begin != 0 && end == 0)
            throw bytefluo_exception(
                bytefluo_exception::null_end_non_null_begin,
                "bytefluo: end is 0, begin isn't");
        if (end < begin)
            throw bytefluo_exception(
                bytefluo_exception::end_precedes_begin,
                "bytefluo: end precedes begin");
    }
};


// return bytefluo object to manage access to bytes in given 'vec';
// NOTE that any operations on the vector that might change the value
// of &vec[0] will silently invalidate the associated bytefluo object
// so that attempts to read the vector contents via that bytefluo
// object may cause a CRASH
template <class item_type>
bytefluo bytefluo_from_vector(
    const std::vector<item_type> & vec,
    bytefluo::byte_order bo)
{
    return vec.empty()
        ? bytefluo(0, 0, bo)
        : bytefluo(&vec[0], &vec[0] + vec.size(), bo);
}


#endif //#ifdef BYTEFLUO_H_INCLUDED
