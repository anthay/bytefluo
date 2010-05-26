#ifndef BYTEFLUO_H_INCLUDED
#define BYTEFLUO_H_INCLUDED

/*  bytefluo
    Copyright (c) 2008,2009,2010 Anthony C. Hay
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
    
    bytefluo_exception(error_id id, const std::string & msg)
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
    : buf_begin(static_cast<const unsigned char *>(begin)),
      buf_end  (static_cast<const unsigned char *>(end)),
      cursor   (static_cast<const unsigned char *>(begin)),
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
        buf_begin = static_cast<const unsigned char *>(begin);
        buf_end   = static_cast<const unsigned char *>(end);
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
        out = scalar_type();    // initialise result to 'zero'
        const size_t out_size = sizeof(out);
        if (buf_end - cursor < static_cast<ptrdiff_t>(out_size))
            throw bytefluo_exception(
                bytefluo_exception::attempt_to_read_past_end,
                "bytefluo: attempt to read past end of data");
        if (buf_byte_order == big) {
            // cursor -> most significant byte
            const unsigned char * p = cursor;
            cursor += out_size;
            for (;;) {
                out |= *p++;
                if (p == cursor)
                    break;
                out <<= CHAR_BIT;
            }
        }
        else {
            // cursor -> least significant byte
            for (const unsigned char * p = cursor + out_size;;) {
                out |= *--p;
                if (p == cursor)
                    break;
                out <<= CHAR_BIT;
            }
            cursor += out_size;
        }
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
    const unsigned char * buf_begin;
    const unsigned char * buf_end;
    const unsigned char * cursor;
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
