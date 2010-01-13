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
    explicit bytefluo_exception(const std::string & msg)
    : std::runtime_error(msg)
    {
    }

    virtual ~bytefluo_exception() throw ()
    {
    }
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
        if (buf_end < buf_begin)
            throw bytefluo_exception("bytefluo: end precedes begin");
        if (buf_begin == 0 && buf_end != 0)
            throw bytefluo_exception("bytefluo: begin is 0, end isn't");
        if (buf_byte_order != big && buf_byte_order != little)
            throw bytefluo_exception("bytefluo: invalid byte order");
    }

    // bytefluo will manage access to bytes in [begin, end)
    bytefluo & set_data_range(const void * begin, const void * end)
    {
        if (end < begin)
            throw bytefluo_exception("bytefluo: end precedes begin");
        if (begin == 0 && end != 0)
            throw bytefluo_exception("bytefluo: begin is 0, end isn't");
        cursor    =
        buf_begin = static_cast<const unsigned char *>(begin);
        buf_end   = static_cast<const unsigned char *>(end);
        return *this;
    }
    
    // specify the byte order to be used on subsequent scalar reads
    bytefluo & set_byte_order(byte_order bo)
    {
        if (bo != big && bo != little)
            throw bytefluo_exception("bytefluo: invalid byte order");
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
                "bytefluo: attempt to read past end of data");
        ::memcpy(dest, cursor, len);
        cursor += len;
        return *this;
    }

    // move cursor 'pos' bytes from stream beginning
    size_t seek_begin(size_t pos)
    {
        return seeker(buf_begin + pos);
    }

    // move cursor 'pos' bytes from current position
    size_t seek_current(long pos)
    {
        return seeker(cursor + pos);
    }

    // move cursor 'pos' bytes from stream end
    size_t seek_end(size_t pos)
    {
        return seeker(buf_end - pos);
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

    // set cursor to given 'new_cursor' iff 'new_cursor' is within buffer;
    // throw if not within buffer; return distance from buffer start to cursor
    size_t seeker(const unsigned char * new_cursor)
    {
        if (new_cursor < buf_begin)
            throw bytefluo_exception(
                "bytefluo: attempt to seek before beginning of data");
        if (new_cursor > buf_end)
            throw bytefluo_exception(
                "bytefluo: attempt to seek after end of data");
        cursor = new_cursor;
        return static_cast<size_t>(cursor - buf_begin);
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
