#ifndef BYTEFLUO_H_INCLUDED
#define BYTEFLUO_H_INCLUDED

/*  bytefluo
    Copyright (c) 2008,2009 Anthony C. Hay
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
    enum byte_arrangement { // when reading a scalar...
        big_endian,   // ...assume most-significant byte has lowest address
        little_endian // ...assume least-significant byte has lowest address
    };

    // bytefluo will manage access to bytes in [begin, end)
    bytefluo::bytefluo(
        const void * begin,
        const void * end,
        byte_arrangement ba)
    : buf_begin(static_cast<const unsigned char *>(begin)),
      buf_end  (static_cast<const unsigned char *>(end)),
      cursor   (static_cast<const unsigned char *>(begin)),
      buf_byte_arrangement(ba)
    {
        if (buf_end < buf_begin)
            throw bytefluo_exception("bytefluo: end precedes begin");
    }

    // specify the byte arrangement to be used on subsequent scalar reads
    void set_byte_arrangement(byte_arrangement ba)
    {
        buf_byte_arrangement = ba;
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
        if (buf_byte_arrangement == big_endian) {
            // cursor -> most significant byte
            const unsigned char * p = cursor;
            cursor += out_size;
            while (p != cursor) {
                out <<= CHAR_BIT;
                out |= *p++;
            }
        }
        else {
            // cursor -> least significant byte
            const unsigned char * p = cursor + out_size;
            while (p != cursor) {
                out <<= CHAR_BIT;
                out |= *--p;
            }
            cursor += out_size;
        }
        return *this;
    }

    // copy 'len' bytes from buffer at current cursor position to given 'dest'
    // note that current buf_byte_arrangement has no affect on this operation
    void read(void * dest, size_t len)
    {
        if (buf_end - cursor < static_cast<ptrdiff_t>(len))
            throw bytefluo_exception(
                "bytefluo: attempt to read past end of data");
        ::memcpy(dest, cursor, len);
        cursor += len;
    }

    // move cursor 'pos' bytes from stream beginning
    long seek_begin(long pos)
    {
        return seeker(buf_begin + pos);
    }

    // move cursor 'pos' bytes from current position
    long seek_current(long pos)
    {
        return seeker(cursor + pos);
    }

    // move cursor 'pos' bytes from stream end
    long seek_end(long pos)
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
    byte_arrangement buf_byte_arrangement;

    // set cursor to given 'new_cursor' iff 'new_cursor' is within buffer;
    // throw if not within buffer; return distance from buffer start to cursor
    long seeker(const unsigned char * new_cursor)
    {
        if (new_cursor < buf_begin)
            throw bytefluo_exception(
                "bytefluo: attempt to seek before beginning of data");
        if (new_cursor > buf_end)
            throw bytefluo_exception(
                "bytefluo: attempt to seek after end of data");
        cursor = new_cursor;
        return static_cast<long>(cursor - buf_begin);
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
    bytefluo::byte_arrangement ba)
{
    return vec.empty()
        ? bytefluo(0, 0, ba)
        : bytefluo(&vec[0], &vec[0] + vec.size(), ba);
}


#endif //#ifdef BYTEFLUO_H_INCLUDED
