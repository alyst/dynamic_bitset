// (C) Copyright Chuck Allison and Jeremy Siek 2001, 2002.
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all
// copies. This software is provided "as is" without express or
// implied warranty, and with no claim as to its suitability for any
// purpose.

// With optimizations, bug fixes, and improvements by Gennaro Prota.

// See http://www.boost.org/libs/dynamic_bitset for documentation.

// -------------------------------------
// CHANGE LOG:
//
//  The changes up to the marking "---" line below are referred
//  to:
//       v. 1.25   of    boost/dynamic_bitset.hpp
//       v. 1.10   of    boost/detail/dynamic_bitset.hpp
//       v. 1.3    of    boost/dynamic_bitset_fwd.hpp    [no change]
//
// into the main CVS repository.

// - Major implementation change:
//
//    Everything rewritten in terms of std::vector<>:
//     note that the code is now much shorter and that the exception
//     safety guarantees of the size-changing operations (resize,
//     clear, push_back, append(Block), append(Iter, Iter)) are
//     easily identifiable in terms of the guarantees provided by
//     vector<Block>'s operations (with Block having a no-throw
//     copy constructor)
//
//     Note that dynamic_bitset::reference has been changed too:
//     a reference is now basically a reference into a Block,
//     and it remains valid if an exception is thrown by
//     a strong-guarantee operation.

//
// - [... see CVS logs ...]

// - several comment fixes and additions (others are needed though)
// - changed operator>() implementation
// - included istream and ostream instead of iosfwd
//     (we use sentry, setstate, rdbuf, etc. so iosfwd isn't enough)
//
// - boost/config.hpp now included after most std headers.
// - several workarounds updated and BOOST_WORKAROUND() macro used
// - small changes to count()
// - object_representation() added to boost::detail
// - inserter and extractor for iostreams rewritten
// - manipulator to extract "infinite" bits added
// - added again #include<vector>, for now.
// - Changes related to initial_num_blocks:
//      - all overloads made inline
//      - added overload for random access iterators
//      - removed superfluous declarations (fwd refs)
//
//  - to_string and dump_to_string reimplemented
//
//  - from_string reimplemented

//  Minor:
//
//  - added local workaround for boost/config/stdlib/stlport.hpp bug
//    I hope this is a temporary one.
//  - set_(size_type bit, bool val) made void
//  - added parens in calc_num_blocks()
//
// -------------------------------------------------------------

// - corrected workaround for Dinkum lib's allocate() [GP]
// - changed macro test for old iostreams [GP]
// - removed #include <vector> for now. [JGS]
// - Added __GNUC__ to compilers that cannot handle the constructor from basic_string. [JGS]
// - corrected to_block_range [GP]
// - corrected from_block_range [GP]
// - Removed __GNUC__ from compilers that cannot handle the constructor
//     from basic_string and added the workaround suggested by GP. [JGS]
// - Removed __BORLANDC__ from the #if around the basic_string
//     constructor. Luckily the fix by GP for g++ also fixes Borland. [JGS]
//

#ifndef BOOST_DYNAMIC_BITSET_HPP
#define BOOST_DYNAMIC_BITSET_HPP

#include <cassert>
#include <string>
#include <stdexcept>           // for std::overflow_error
#include <algorithm>           // for std::swap, std::min, std::copy, std::fill
#include <vector>

#include "boost/config.hpp"

#ifndef BOOST_NO_STD_LOCALE
# include <locale> // G.P.S
#endif


//  The purpose of this BOOST_OLD_IOSTREAMS macro is basically to support
//  pre-standard implementations of the g++ library (the best place for such
//  a test would be in the boost config files, but there isn't probably any
//  intent for a general old iostreams support). The test here exploits the
//  fact that old (in fact, pre 3.0) versions of libstdc++ are based on the
//  old sgi [Thanks to Phil Edwards for useful info about libstdc++ history]
//
#if defined (__STL_CONFIG_H) && !defined (__STL_USE_NEW_IOSTREAMS)
#  define BOOST_OLD_IOSTREAMS
#  include <iostream.h>
#  include <ctype.h> // for isspace
#else
#  include <istream>
#  include <ostream>
#endif

#include "boost/dynamic_bitset_fwd.hpp" //G.P.S.
#include "boost/detail/dynamic_bitset.hpp"
#include "boost/limits.hpp" // [gps] now included here
#include "boost/lowest_bit.hpp" // used by find_first/next



#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)                   // 1300 == VC++ 7.0
  //  in certain situations VC++ requires a redefinition of
  //  default template arguments, in contrast with 14.1/12
  //
# define BOOST_WORKAROUND_REPEAT_DEFAULT_TEMPLATE_ARGUMENTS // macro 'local' to this file
#endif


#if defined (_STLP_OWN_IOSTREAMS) && !defined(__SGI_STL_OWN_IOSTREAMS) && \
    defined(__STL_USE_NAMESPACES) &&  defined(BOOST_NO_USING_TEMPLATE) && !defined(__BORLANDC__)
  //  This is a local workaround for what I think is a bug
  //  in boost/config/stdlib/stlport.hpp: with MSVC+STLport
  //  the macro BOOST_NO_STD_LOCALE gets erroneusly defined
  //  (due to a spurious check of __SGI_STL_OWN_IOSTREAMS);
  //  this causes the subsequent code in suffix.hpp to fail
  //  to provide a definition for BOOST_USE_FACET.
  //
# define LOCAL_FACET_CONFIG_BUG
#include <locale>
# define LOCAL_BOOST_USE_FACET(Type, loc) std::use_facet< Type >(loc)
#else
# define LOCAL_BOOST_USE_FACET(Type, loc) BOOST_USE_FACET(Type, loc)
#endif



namespace boost {

#ifdef BOOST_WORKAROUND_REPEAT_DEFAULT_TEMPLATE_ARGUMENTS
 template <typename Block = unsigned long, typename Allocator = std::allocator<Block> >
#else
 template <typename Block, typename Allocator>
#endif

class dynamic_bitset
{
  // Portability note: member function templates are defined inside
  // this class definition to avoid problems with VC++. Similarly,
  // with the member functions of the nested class.


public:
    typedef Block block_type;
    typedef Allocator allocator_type; // [gps] to be documented
    typedef std::size_t size_type;
    BOOST_STATIC_CONSTANT(int, bits_per_block = (std::numeric_limits<Block>::digits));
    BOOST_STATIC_CONSTANT(size_type, npos = -1);


public:

    // A proxy class to simulate lvalues of bit type.
    // Shouldn't it be private? [gps]
    //
    class reference
    {
        friend class dynamic_bitset<Block, Allocator>;


        // the one and only non-copy ctor
        reference(block_type & b, int pos)
            :m_block(b), m_mask(block_type(1) << pos)
        {}

        void operator&(); // not defined

    public:

        // copy constructor: compiler generated

        operator bool() const { return (m_block & m_mask) != 0; }
        bool operator~() const { return (m_block & m_mask) == 0; }

        reference& flip() { do_flip(); return *this; }

        reference& operator=(bool x)               { do_assign(x);   return *this; } // for b[i] = x
        reference& operator=(const reference& rhs) { do_assign(rhs); return *this; } // for b[i] = b[j]

        reference& operator|=(bool x) { if  (x) do_set();   return *this; }
        reference& operator&=(bool x) { if (!x) do_reset(); return *this; }
        reference& operator^=(bool x) { if  (x) do_flip();  return *this; }
        reference& operator-=(bool x) { if  (x) do_reset(); return *this; }

     private:
        block_type & m_block;
        const block_type m_mask;

        void do_set() { m_block |= m_mask; }
        void do_reset() { m_block &= ~m_mask; }
        void do_flip() { m_block ^= m_mask; }
        void do_assign(bool x) { x? do_set() : do_reset(); }
    };

    typedef bool const_reference;

    // constructors, etc.
    explicit
    dynamic_bitset(const Allocator& alloc = Allocator());

    explicit
    dynamic_bitset(size_type num_bits, unsigned long value = 0,
               const Allocator& alloc = Allocator());

    // from string
#if defined(BOOST_OLD_IOSTREAMS)
    explicit
    dynamic_bitset(const std::string& s,
               std::string::size_type pos = 0,
               std::string::size_type n = std::string::npos,
               const Allocator& alloc = Allocator())
#else
    // The parentheses around std::basic_string<CharT, Traits, Alloc>::npos
    // in the code below are to avoid a g++ 3.2 bug and a Borland bug. -JGS
    template <typename CharT, typename Traits, typename Alloc>
    explicit
    dynamic_bitset(const std::basic_string<CharT, Traits, Alloc>& s,
        typename std::basic_string<CharT, Traits, Alloc>::size_type pos = 0,
        typename std::basic_string<CharT, Traits, Alloc>::size_type n
            = (std::basic_string<CharT, Traits, Alloc>::npos),
        const Allocator& alloc = Allocator())
#endif

    :m_bits(alloc),
     m_num_bits(0)
    {
        // [gps] to be optimized

        assert(pos <= s.size()); // [gps] <=??
        const size_type len = std::min(s.size() - pos, n);

        m_bits.resize(calc_num_blocks(len));
        m_num_bits = len;

        from_string(s, pos, len);

    }

    // The first bit in *first is the least significant bit, and the
    // last bit in the block just before *last is the most significant bit.
    template <typename BlockInputIterator>
    dynamic_bitset(BlockInputIterator first, BlockInputIterator last,
                   const Allocator& alloc = Allocator())

    :m_bits(first, last, alloc),
     m_num_bits(m_bits.size() * bits_per_block)
    {}


    // copy constructor
    dynamic_bitset(const dynamic_bitset& b);

    void swap(dynamic_bitset& b);

    dynamic_bitset& operator=(const dynamic_bitset& b);

    // size changing operations
    void resize(size_type num_bits, bool value = false);
    void clear();
    void push_back(bool bit);
    void append(Block block);

    template <typename BlockInputIterator>
    void append(BlockInputIterator first, BlockInputIterator last) // basic guarantee (for now?)
    {
        // to be optimized? [gps]
        for ( ; first != last; ++first)
            append(*first);
    }


    // bitset operations
    dynamic_bitset& operator&=(const dynamic_bitset& b);
    dynamic_bitset& operator|=(const dynamic_bitset& b);
    dynamic_bitset& operator^=(const dynamic_bitset& b);
    dynamic_bitset& operator-=(const dynamic_bitset& b);
    dynamic_bitset& operator<<=(size_type n);
    dynamic_bitset& operator>>=(size_type n);
    dynamic_bitset operator<<(size_type n) const;
    dynamic_bitset operator>>(size_type n) const;

    // basic bit operations
    dynamic_bitset& set(size_type n, bool val = true);
    dynamic_bitset& set();
    dynamic_bitset& reset(size_type n);
    dynamic_bitset& reset();
    dynamic_bitset& flip(size_type n);
    dynamic_bitset& flip();
    bool test(size_type n) const;
    bool any() const;
    bool none() const;
    dynamic_bitset operator~() const;
    size_type count() const;

    // subscript
    reference operator[](size_type pos) {
        return reference(this->m_bits[this->block_index(pos)], this->bit_index(pos));
    }
    bool operator[](size_type pos) const { return test(pos); }

    unsigned long to_ulong() const;

    size_type size() const;
    size_type num_blocks() const;

    bool is_subset_of(const dynamic_bitset& a) const;
    bool is_proper_subset_of(const dynamic_bitset& a) const;

    // lookup
    size_type find_first() const;
    size_type find_next(size_type pos) const;


#ifdef BOOST_DYN_BITSET_USE_FRIENDS
    // lexicographical comparison
    template <typename B, typename A>
    friend bool operator==(const dynamic_bitset<B, A>& a,
                           const dynamic_bitset<B, A>& b);

    template <typename B, typename A>
    friend bool operator<(const dynamic_bitset<B, A>& a,
                          const dynamic_bitset<B, A>& b);


    template <typename B, typename A, typename BlockOutputIterator>
    friend void to_block_range(const dynamic_bitset<B, A>& b,
                               BlockOutputIterator result);

    template <typename BlockIterator, typename B, typename A>
    friend void from_block_range(BlockIterator first, BlockIterator last,
                                 dynamic_bitset<B, A>& result);


    template <typename CharT, typename Traits, typename B, typename A>
    friend std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>& is,
                                                         dynamic_bitset<B, A>& b);

    template <typename B, typename A, typename stringT>
    friend void to_string_helper (const dynamic_bitset<B, A> & b, stringT & s, std::size_t len);

    // stream extractor helper
    template <typename B, typename A>
    friend void from_vect_of_blocks(std::vector<B> const & vect,
                                    typename dynamic_bitset<B, A>::size_type nbits, dynamic_bitset<B, A>& b);

#endif


public:

    // This is templated on the whole String instead of just CharT,
    // Traits, Alloc to avoid compiler bugs.
    template <typename String>
    void from_string(const String& s, typename String::size_type pos,
                     typename String::size_type rlen)
    {
        // TO Jeremy: this function is now used in one of the ctors only.
        //            Do we really want it separately?
        //
        typedef typename String::value_type  Ch;
        typedef typename String::traits_type Tr;

#if defined (BOOST_USE_FACET) || defined (LOCAL_FACET_CONFIG_BUG)
    Ch const one  = LOCAL_BOOST_USE_FACET(std::ctype<Ch>, std::locale()).widen('1');
#else
    Ch const one  = '1'; // temporary- G.P.S.
#endif

        reset(); // bugfix [gps]
        size_type const tot = std::min (rlen, s.length()); // bugfix [gps]

        typename String::const_iterator iter = s.begin() + pos + tot - 1;
        for (size_type i = 0; i < tot; ++i) {
            if (Tr::eq(*iter, one))
                set(i);
            //else
              //  assert(Tr::eq(*iter, std::locale().widen('0')) );
            --iter;
        }

    }

    //
private:
    BOOST_STATIC_CONSTANT(int, ulong_width = std::numeric_limits<unsigned long>::digits);
    typedef std::vector<block_type, allocator_type> buffer_type;

    void m_zero_unused_bits();
    size_type m_do_find_from(size_type first_block) const;

    static size_type block_index(size_type pos) { return pos / bits_per_block; }
    static int bit_index(size_type pos) { return pos % bits_per_block; }
    static Block bit_mask(size_type pos) { return Block(1) << bit_index(pos); }



BOOST_DYN_BITSET_PRIVATE:

    static size_type calc_num_blocks(size_type num_bits)
    { return (num_bits + (bits_per_block - 1)) / bits_per_block; }


    buffer_type m_bits; // [gps] to be renamed
    size_type   m_num_bits;

};

// Global Functions:

// comparison
template <typename Block, typename Allocator>
bool operator!=(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
bool operator<=(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
bool operator>(const dynamic_bitset<Block, Allocator>& a,
               const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
bool operator>=(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b);

// stream operators
#ifdef BOOST_OLD_IOSTREAMS
template <typename Block, typename Allocator>
std::ostream& operator<<(std::ostream& os,
                         const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
std::istream& operator>>(std::istream& is, dynamic_bitset<Block,Allocator>& b);
#else
template <typename CharT, typename Traits, typename Block, typename Allocator>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const dynamic_bitset<Block, Allocator>& b);

template <typename CharT, typename Traits, typename Block, typename Allocator>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is,
           dynamic_bitset<Block, Allocator>& b);
#endif

// bitset operations
template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator&(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator|(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator^(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator-(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);


template <typename Block, typename Allocator, typename stringT>
void
to_string(const dynamic_bitset<Block, Allocator>& b, stringT & s); // G.P.S.

template <typename Block, typename Allocator, typename BlockOutputIterator>
void
to_block_range(const dynamic_bitset<Block, Allocator>& b,
               BlockOutputIterator result);


// G.P.S. check docs with Jeremy
//
template <typename BlockIterator, typename B, typename A>
inline void
from_block_range(BlockIterator first, BlockIterator last,
                 dynamic_bitset<B, A>& result)
{
    // PRE: distance(first, last) == numblocks()
    std::copy (first, last, result.m_bits.begin()); //[gps]
}

//=============================================================================
// dynamic_bitset implementation

#ifdef BOOST_OLD_IOSTREAMS
template <typename Block, typename Allocator>
inline std::ostream&
operator<<(std::ostream& os,
           const typename dynamic_bitset<Block, Allocator>::reference& br)
{
    return os << (bool)br;
}
#else
template <typename CharT, typename Traits, typename Block, typename Allocator>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const typename dynamic_bitset<Block, Allocator>::reference& br)
{
    return os << (bool)br;
}
#endif

//-----------------------------------------------------------------------------
// constructors, etc.

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>::dynamic_bitset(const Allocator& alloc)
  : m_bits(alloc), m_num_bits(0)
{

}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>::
dynamic_bitset(size_type num_bits, unsigned long value, const Allocator& alloc)
  : m_bits(calc_num_blocks(num_bits), Block(0), alloc),
    m_num_bits(num_bits)
{
  const size_type M = std::min(static_cast<size_type>(ulong_width), num_bits);
  for(size_type i = 0; i < M; ++i, value >>= 1) // [G.P.S.] to be optimized
    if ( value & 0x1 )
      set(i);
}

// copy constructor
template <typename Block, typename Allocator>
inline dynamic_bitset<Block, Allocator>::
dynamic_bitset(const dynamic_bitset& b)
  : m_bits(b.m_bits), m_num_bits(b.m_num_bits)  // [gps]
{

}

template <typename Block, typename Allocator>
inline void dynamic_bitset<Block, Allocator>::
swap(dynamic_bitset<Block, Allocator>& b)
{
    std::swap(m_bits, b.m_bits);
    std::swap(m_num_bits, b.m_num_bits);
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::
operator=(const dynamic_bitset<Block, Allocator>& b)
{
    dynamic_bitset<Block, Allocator> tmp(b);
    this->swap(tmp);
    return *this;
}

//-----------------------------------------------------------------------------
// size changing operations

template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
resize(size_type num_bits, bool value) // strong guarantee
{

  const size_type old_num_blocks = num_blocks();
  const size_type required_blocks = calc_num_blocks(num_bits);

  const block_type v = value? ~Block(0) : Block(0);

  if (required_blocks != old_num_blocks) {
    m_bits.resize(required_blocks, v); // s.g. (copy) [gps]
  }


  // At this point:
  //
  //  - if the buffer was shrunk, there's nothing to do, except
  //    a call to m_zero_unused_bits()
  //
  //  - if it it is enlarged, all the (used) bits in the new blocks have
  //    the correct value, but we should also take care of the bits,
  //    if any, that were 'unused bits' before enlarging: if value == true,
  //    they must be set.

  if (value && (num_bits > m_num_bits)) {

    const size_type extra_bits = m_num_bits % bits_per_block; // G.P.S.
    if (extra_bits) {
        assert(old_num_blocks >= 1 && old_num_blocks <= m_bits.size());

        // Set them.
        m_bits[old_num_blocks - 1] |= (v << extra_bits); // G.P.S.
    }

  }



  m_num_bits = num_bits;
  m_zero_unused_bits();

}

template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
clear() // no throw
{
  m_bits.clear();
  m_num_bits = 0;
}


template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
push_back(bool bit)
{
  this->resize(this->size() + 1);
  set(this->size() - 1, bit);
}

template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
append(Block value) // strong guarantee
{

    // G.P.S. to be reviewed...

     // if != 0 this is the number of bits used in the last block
    const size_type excess_bits = this->m_num_bits % bits_per_block;

    if (excess_bits == 0) {
        // the buffer was empty, or blocks are all filled
        m_bits.push_back(value);
    }
    else {
        const typename buffer_type::iterator it = m_bits.end() - 1;
        *it |= (value << excess_bits);
        m_bits.push_back(value >> (bits_per_block - excess_bits));
    }

    m_num_bits += bits_per_block;

}


//-----------------------------------------------------------------------------
// bitset operations
template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator&=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < num_blocks(); ++i)
        this->m_bits[i] &= rhs.m_bits[i];
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator|=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < num_blocks(); ++i)
        this->m_bits[i] |= rhs.m_bits[i];
    m_zero_unused_bits();
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator^=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < this->num_blocks(); ++i)
        this->m_bits[i] ^= rhs.m_bits[i];
    m_zero_unused_bits();
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator-=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < this->num_blocks(); ++i)
        this->m_bits[i] = this->m_bits[i] & ~rhs.m_bits[i];
    m_zero_unused_bits();
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator<<=(size_type n)
{
    if (n >= this->m_num_bits)
        return reset();
    //else
    if (n > 0)
    {
        size_type  const last = this->num_blocks() - 1; // num_blocks() is >= 1
        size_type  const div  = n / bits_per_block;   // div is <= last
        size_type  const r    = n % bits_per_block;
        block_type * b        = &m_bits[0];

        // PRE: div != 0  or  r != 0

        if (r != 0) {

            block_type const rs = bits_per_block - r;

            for (size_type i = last-div; i>0; --i) {
                b[i+div] = (b[i] << r) | (b[i-1] >> rs);
            }
            b[div] = b[0] << r;

        }
        else {
            for (size_type i = last-div; i>0; --i) {
                b[i+div] = b[i];
            }
            b[div] = b[0];
        }


        // div blocks are zero filled at the less significant end
        std::fill_n(b, div, static_cast<block_type>(0));


    }

    return *this;


}


//
// NOTE:
//
//      this function (as well as operator <<=) assumes that
//      within each block bits are arranged 'from right to left'.
//
//             static int bit_index(size_type pos)
//             { return pos % bits_per_block; }
//
//  Note also the 'if (r != 0)' in the implementation below:
//  besides being an optimization, it avoids the undefined behavior that
//  could result, when r==0, from shifting a block by ls = bits_per_block
//  bits (we says 'could' because actually, depending on its type, the left
//  operand of << or >> can be promoted to a type with more than bits_per_block
//  value-bits; but the if-else ensures we have well-defined behavior regardless
//  of integral promotions) - G.P.S.
//
template <typename B, typename A>
dynamic_bitset<B, A> & dynamic_bitset<B, A>::operator>>=(size_type n) {
    if (n >= this->m_num_bits) {
        return reset();
    }
    //else
    if (n>0){

        size_type  const last  = this->num_blocks() - 1; // num_blocks() is >= 1
        size_type  const div   = n / bits_per_block;   // div is <= last
        size_type  const r     = n % bits_per_block;
        block_type * b         = &m_bits[0];

        // PRE: div != 0  or  r != 0

        if (r != 0) {

            block_type const ls = bits_per_block - r;

            for (size_type i = div; i < last; ++i) {
                b[i-div] = (b[i] >> r) | (b[i+1]  << ls);
            }
            // r bits go to zero
            b[last-div] = b[last] >> r;
        }

        else {
            for (size_type i = div; i <= last; ++i) {
                b[i-div] = b[i];
            }
            // note the '<=': the last iteration 'absorbs'
            // b[last-div] = b[last] >> 0;
        }



        // div blocks are zero filled at the most significant end
        std::fill_n(b + (this->num_blocks()-div), div, static_cast<block_type>(0));
    }

    return *this;
}







template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
dynamic_bitset<Block, Allocator>::operator<<(size_type n) const
{
    dynamic_bitset r(*this);
    return r <<= n;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
dynamic_bitset<Block, Allocator>::operator>>(size_type n) const
{
    dynamic_bitset r(*this);
    return r >>= n;
}


//-----------------------------------------------------------------------------
// basic bit operations

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::set(size_type pos, bool val)
{
    // [gps]
    //
    // Below we have no set(size_type) function to call when
    // value == true; instead of using a helper, I think
    // overloading set (rather than giving it a default bool
    // argument) would be more elegant.

    assert(pos < this->m_num_bits);

    if (val)
        this->m_bits[this->block_index(pos)] |= this->bit_mask(pos);
    else
        reset(pos);

    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::set()
{
  std::fill(m_bits.begin(), m_bits.end(), ~Block(0));
  m_zero_unused_bits();
  return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::reset(size_type pos)
{
    assert(pos < this->m_num_bits);
    this->m_bits[this->block_index(pos)] &= ~this->bit_mask(pos);
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::reset()
{
  std::fill(m_bits.begin(), m_bits.end(), Block(0));
  return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::flip(size_type pos)
{
    assert(pos < this->m_num_bits);
    this->m_bits[this->block_index(pos)] ^= this->bit_mask(pos);
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::flip()
{
    for (size_type i = 0; i < this->num_blocks(); ++i)
        this->m_bits[i] = ~this->m_bits[i];
    m_zero_unused_bits();
    return *this;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::test(size_type pos) const
{
    assert(pos < this->m_num_bits);
    return (this->m_bits[this->block_index(pos)] & this->bit_mask(pos)) != 0;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::any() const
{
    for (size_type i = 0; i < this->num_blocks(); ++i)
        if (this->m_bits[i])
            return 1;
    return 0;
}

template <typename Block, typename Allocator>
inline bool dynamic_bitset<Block, Allocator>::none() const
{
    return !any();
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
dynamic_bitset<Block, Allocator>::operator~() const
{
    dynamic_bitset b(*this);
    b.flip();
    return b;
}


/*

The following is the straightforward implementation of count(), which
we leave here in a comment for documentation purposes.

template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::count() const
{
    size_type sum = 0;
    for (size_type i = 0; i != this->m_num_bits; ++i)
        if (test(i))
            ++sum;
    return sum;
}

The actual algorithm uses a lookup table.


  The basic idea of the method is to pick up X bits at a time
  from the internal array of blocks and consider those bits as
  the binary representation of a number N. Then, to use a table
  of 1<<X elements where table[N] is the number of '1' digits
  in the binary representation of N (i.e. in our X bits).


  In this implementation X is 8 (but can be easily changed: you
  just have to modify the definition of table_width and shrink/enlarge
  the table accordingly - it could be useful, for instance, to expand
  the table to 512 elements on an implementation with 9-bit bytes) and
  the internal array of blocks is seen, if possible, as an array of bytes.
  In practice the "reinterpretation" as array of bytes is possible if and
  only if X >= CHAR_BIT and Block has no padding bits (that would be counted
  together with the "real ones" if we saw the array as array of bytes).
  Otherwise we simply 'extract' X bits at a time from each Block.

*/


template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::count() const
{
    using namespace detail::dynamic_bitset_count_impl;

    const bool no_padding = bits_per_block == CHAR_BIT * sizeof(Block);
    const mode m = table_width >= CHAR_BIT && no_padding? access_by_bytes : access_by_blocks;

    return do_count(m_bits.begin(), this->num_blocks(), Block(0), (mode_to_type<m>*) 0 );

}


//-----------------------------------------------------------------------------
// conversions


template <typename B, typename A, typename stringT>
void to_string_helper (const dynamic_bitset<B, A> & b, stringT & s, std::size_t len)
{
    typedef typename stringT::traits_type Tr;
    typedef typename stringT::value_type  Ch;

    // G.P.S. qui non possiamo usare il locale per i vecchi g++ - temp
#if defined (BOOST_USE_FACET) || defined (LOCAL_FACET_CONFIG_BUG)
    std::locale loc; // global locale (copy of)
    Ch const zero = LOCAL_BOOST_USE_FACET(std::ctype<Ch>, loc).widen('0');
    Ch const one  = LOCAL_BOOST_USE_FACET(std::ctype<Ch>, loc).widen('1');
#else
    Ch const zero = '0';
    Ch const one  = '1'; //
#endif

    // Note that this function is implemented through operator[] and
    // uses only the public interface. Anyhow, this is a deceit because
    // it may call operator[] itself with an index >= b.size() and so
    // relies on an implementation detail of dynamic_bitset.
    // To highlight this, the dependency on the "internals" is made
    // explicit by granting it friendship.
    //
    assert (b.size() <= len /*&& len <= [G.P.S.]*/ );
    s.assign (len, zero);
    for (std::size_t i = 0; i < len; ++i) {
        if (b[i]) // G.P.S. da decidere con Jeremy
            Tr::assign(s[len - 1 - i], one);

    }

}



// take as ref param instead?
template <typename Block, typename Allocator, typename stringT> // G.P.S.
inline void
to_string(const dynamic_bitset<Block, Allocator>& b, stringT& s)
{
    to_string_helper (b, s, b.size());
}


// Differently from to_string this function dumps out
// every bit of the internal representation (useful
// for debugging purposes)
//
template <typename B, typename A, typename stringT>
inline void
dump_to_string(const dynamic_bitset<B, A>& b,
               /*std::basic_string<CharT, Alloc>*/stringT& s) // G.P.S.
{
    to_string_helper(b, s, b.m_num_blocks * (dynamic_bitset<B, A>::bits_per_block) );
}

template <typename Block, typename Allocator, typename BlockOutputIterator>
inline void
to_block_range(const dynamic_bitset<Block, Allocator>& b,
               BlockOutputIterator result)
{
    assert (b.m_bits != 0 || b.m_num_blocks == 0);
    std::copy (b.m_bits, b.m_bits + b.m_num_blocks, result); // [gps]
}

template <typename Block, typename Allocator>
unsigned long dynamic_bitset<Block, Allocator>::
to_ulong() const
{
  const std::overflow_error
    overflow("boost::bit_set::operator unsigned long()");

  if (this->num_blocks() == 0)
    return 0;

  if (sizeof(Block) >= sizeof(unsigned long)) {
    for (size_type i = 1; i < this->num_blocks(); ++i)
      if (this->m_bits[i])
        throw overflow;
    const Block mask = static_cast<Block>(static_cast<unsigned long>(-1));
    if (this->m_bits[0] & ~mask)
      throw overflow;
    size_type N = std::min(sizeof(unsigned long) * CHAR_BIT, this->size());
    unsigned long num = 0;
    for (size_type j = 0; j < N; ++j)
      if (this->test(j))
        num |= (1 << j);
    return num;
  }
  else { // sizeof(Block) < sizeof(unsigned long).
    const size_type nwords =
      (sizeof(unsigned long) + sizeof(Block) - 1) / sizeof(Block);

    size_type min_nwords = nwords;
    if (this->num_blocks() > nwords) {
      for (size_type i = nwords; i < this->num_blocks(); ++i)
        if (this->m_bits[i])
          throw overflow;
    }
    else
      min_nwords = this->num_blocks();

    unsigned long result = 0;
    size_type N = std::min(sizeof(unsigned long) * CHAR_BIT, this->size());
    for (size_type i = 0; i < N; ++i)
      if (this->test(i))
        result |= (1 << i);
    return result;
  }
}


template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::size() const
{
    return this->m_num_bits;
}

template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::num_blocks() const
{
    return m_bits.size();
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::
is_subset_of(const dynamic_bitset<Block, Allocator>& a) const
{
    assert(this->size() == a.size());
    for (size_type i = 0; i < this->num_blocks(); ++i)
        if (this->m_bits[i] & ~a.m_bits[i])
            return false;
    return true;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::
is_proper_subset_of(const dynamic_bitset<Block, Allocator>& a) const
{
    assert(this->size() == a.size());
    bool proper = false;
    for (size_type i = 0; i < this->num_blocks(); ++i) {
        Block bt = this->m_bits[i], ba = a.m_bits[i];
        if (ba & ~bt)
            proper = true;
        if (bt & ~ba)
            return false;
    }
    return proper;
}

// --------------------------------
// lookup


// look for the first bit "on", starting
// from the block with index first_block
//
template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::m_do_find_from(size_type first_block) const
{
    size_type i = first_block;

    // skip null blocks
    while (i < this->num_blocks() && this->m_bits[i] == 0)
        ++i;

    if (i>=this->num_blocks())
        return npos; // not found

    return i * bits_per_block + boost::lowest_bit(this->m_bits[i]);

}



template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::find_first() const
{
    return m_do_find_from(0);

}



template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::find_next(size_type pos) const
{
    ++pos;
    if (pos >= this->size())
        return npos;

    const size_type blk = this->block_index(pos);
    const size_type ind = this->bit_index(pos);

    // mask out bits before pos
    const Block fore = this->m_bits[blk] & ( ~Block(0) << ind );

    return fore?
        blk * bits_per_block + lowest_bit(fore)
        :
        m_do_find_from(blk + 1);

}



//-----------------------------------------------------------------------------
// comparison

template <typename Block, typename Allocator>
bool operator==(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b)
{
    return (a.m_num_bits == b.m_num_bits)
           && (a.m_bits == b.m_bits); // [gps]
}

template <typename Block, typename Allocator>
inline bool operator!=(const dynamic_bitset<Block, Allocator>& a,
                       const dynamic_bitset<Block, Allocator>& b)
{
    return !(a == b);
}

template <typename Block, typename Allocator>
bool operator<(const dynamic_bitset<Block, Allocator>& a,
               const dynamic_bitset<Block, Allocator>& b)
{
    assert(a.size() == b.size());
    typedef typename dynamic_bitset<Block, Allocator>::size_type size_type;

    if (a.size() == 0)
      return false;

    // Since we are storing the most significant bit
    // at pos == size() - 1, we need to do the comparisons in reverse.

    // Compare a block at a time
    for (size_type i = a.num_blocks() - 1; i > 0; --i)
      if (a.m_bits[i] < b.m_bits[i])
        return true;
      else if (a.m_bits[i] > b.m_bits[i])
        return false;

    if (a.m_bits[0] < b.m_bits[0])
      return true;
    else
      return false;
}

template <typename Block, typename Allocator>
inline bool operator<=(const dynamic_bitset<Block, Allocator>& a,
                       const dynamic_bitset<Block, Allocator>& b)
{
    return !(a > b);
}

template <typename Block, typename Allocator>
inline bool operator>(const dynamic_bitset<Block, Allocator>& a,
                      const dynamic_bitset<Block, Allocator>& b)
{
    return b < a;
}

template <typename Block, typename Allocator>
inline bool operator>=(const dynamic_bitset<Block, Allocator>& a,
                       const dynamic_bitset<Block, Allocator>& b)
{
    return !(a < b);
}

//-----------------------------------------------------------------------------
// stream operations

#ifdef BOOST_OLD_IOSTREAMS
template < typename Block, typename Allocator>
std::ostream&
operator<<(std::ostream& os, const dynamic_bitset<Block, Allocator>& b)
{
    std::string s;
    to_string(b, s);
    os << s.c_str(); // To Jeremy: should we support bs_limit() for old iostreams too? - G.P.S.
    return os;
}
#else

// Two words on the implementation: we use the stream buffer directly here
// so we have to deal with some details that are normally handled by the
// stream. In particular, with the standard format parameters (27.4.2.1.2, table 83).
// Fortunately, only a few of them have or may have a meaning for dynamic_bitset
// Also, exception handling needs a bit of care because setting state flags is
// itself an operation that may throw exceptions.
//
template <typename CharT, typename Traits, typename Block, typename Allocator>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const dynamic_bitset<Block, Allocator>& b)
{
#if 0 // old - excluded
    std::basic_string<CharT, Traits> s;
    to_string(b, s);
    os << s;
    return os;
#endif

    using namespace std;

    CharT const zero = os.widen('0');
    CharT const one  = os.widen('1');

    ios_base::iostate err = ios_base::goodbit; // G.P.S.
    typename basic_ostream<CharT, Traits>::sentry oprefix(os);
    if (oprefix) {

        try {


            typedef typename dynamic_bitset<Block, Allocator>::size_type bitsetsize_type;
            typedef basic_streambuf<CharT, Traits> buffer_type; // G.P.S.

            buffer_type * buf = os.rdbuf();
            size_t npad = os.width() <= 0  // careful: os.width() is signed (and can be < 0)
                || (bitsetsize_type) os.width() <= b.size()? 0 : os.width() - b.size(); //- G.P.S.

            // eventually fill at left; pad is decresed along the way
            bool const pad_left = (os.flags() & ios_base::adjustfield) == ios_base::left;
            if (npad && !pad_left) { // fills at left
                for (; err == ios_base::goodbit && 0 < npad; --npad)
                    if (Traits::eq_int_type(Traits::eof(),
                        buf->sputc(os.fill())))
                          err |= ios_base::failbit;   // G.P.S.
            }

            // output the bitset
            for (bitsetsize_type i = b.size(); i>0; --i) {// G.P.S.
                typename buffer_type::int_type ret = buf->sputc(b.test(i-1)? one : zero);
                if (Traits::eq_int_type(Traits::eof(), ret) ) { // - G.P.S.
                    err |= ios_base::failbit;
                    break;
                }
            }

            // in case fill at right
            for (; err == ios_base::goodbit && 0 < npad; --npad)
                if (Traits::eq_int_type(Traits::eof(),
                    buf->sputc(os.fill())))
                      err |= ios_base::failbit;


            os.width(0);

        } catch (...) {
            // note that if setstate() throws we rethrow the
            // original exception coming from the stream buffer
            //
            bool rethrow_original = false;
            try { os.setstate(ios_base::failbit); } catch (...) { rethrow_original = true; }

            if (rethrow_original)
                throw;
        }
    }

    // set the error bits: note that this may throw
    if(err != ios_base::goodbit)
        os.setstate(err);
    return os;

}
#endif


// ---------------------------------------------------------------------
// manipulators


// boost::bs_limit
//
// semantics (informal)
// ~~~~~~~~~~~~~~~~~~~~
//  Given a dynamic_bitset object bs:
//
//  a) cin >> boost::bs_limit(boost::bsinf) >> bs;
//     Extracts as many characters as possible,
//     eventually resizing bs as needed.
//
//  b) cin >> boost::bs_limit(20) >> bs
//     Extracts at most 20 characters (regardless
//     of bs.size())
//
//  c) cin >> boost::bs_limit(0)
//     Restores the default behavior (which is the
//     same behavior of std::bitset)

// Implementation note:
//       when type_ == inf, the value of m_n has no meaning
//       (it's 0 or the value previously set); thus you must
//       always check for that value before using m_n
//
enum bsinf_type { bsinf };
class bs_limit {

public:
    enum type { unset, set, inf };
    explicit bs_limit(std::size_t n) : type_(set), m_n(n) {}
    explicit bs_limit(bsinf_type) : type_(inf) {}

    template <typename Stream>
        friend Stream& operator>>(Stream&, const bs_limit&); // G.P.S.

//private: // made public for simplicity
    type type_;
    std::size_t m_n;

    static int const bsTypeIdx;
    static int const bsLimitIdx;
};


namespace detail {
# ifdef BOOST_OLD_IOSTREAMS
    typedef std::ios      io_hierarchy_base;
# else
    typedef std::ios_base io_hierarchy_base;
# endif
}
int const bs_limit::bsTypeIdx = detail::io_hierarchy_base::xalloc();
int const bs_limit::bsLimitIdx = detail::io_hierarchy_base::xalloc();



// parametrized on the whole stream to
// make it work with both new and old iostreams
//
template <typename Stream>
Stream & operator>>( Stream& is, const bs_limit& m)
{
    is.iword(bs_limit::bsTypeIdx) = static_cast<long> (m.type_);

    if (m.type_ != bs_limit::inf)
        is.iword(bs_limit::bsLimitIdx) = m.m_n;

    return is;
}

template <typename Stream>
inline bool can_extract_infinite_bitsets(Stream & is)
{
    return is.iword(bs_limit::bsTypeIdx) == bs_limit::inf;
}

template <typename Stream>
inline std::size_t bitset_max_bits(Stream & is)
{
 return is.iword(bs_limit::bsLimitIdx);
}
// ----------------------------------------------------------------------

// helper for stream extraction
// copies bits from a vector of blocks, where they
// are temporarily stored during extraction
//
template <typename B, typename A>
void from_vect_of_blocks(std::vector<B> const & vect,
                                typename dynamic_bitset<B, A>::size_type nbits, dynamic_bitset<B, A>& b)
{

    typedef typename dynamic_bitset<B, A>::size_type size_type;
    size_type const bits_per_block = dynamic_bitset<B, A>::bits_per_block;

    // G.P.S.
    //
    b.resize(nbits);

    if (!nbits)
        return; // G.P.S.

    size_type const r = nbits % bits_per_block;
    size_type const lasti = b.num_blocks() - 1; // G.P.S.
    size_type const rs = bits_per_block - r;

    if (r) {
        for (size_type i = lasti; i>0; --i) // G.P.S.
            b.m_bits[lasti-i] = (vect[i] >> rs) | (vect[i-1] << r);

        b.m_bits[lasti] = vect[0] >> rs;
    }
    else {
        for (size_type i = lasti; i>0; --i)
            b.m_bits[lasti-i] = vect[i];
        b.m_bits[lasti] = vect[0];
    }
}


#ifdef BOOST_OLD_IOSTREAMS
template <typename Block, typename Allocator>
std::istream&
operator>>(std::istream& is, dynamic_bitset<Block, Allocator>& b)
{
    typedef char CharT;
    std::string buf;
    typedef typename std::string::size_type size_type;
    const size_type N = b.size();
    buf.reserve(N);

    // skip whitespace
    if (is.flags() & std::ios::skipws) {
        char c;
        do {
            is.get(c);
        }
        while (is && isspace(c));

        if (is)
            is.putback(c);
    }

    size_type i;
    for (i = 0; i < N; ++i)
    {
        CharT c;
        is.get(c);
        if (c == '0' || c == '1')
            buf += c;
        else
        {
            is.putback(c);
            break;
        }
    }

    if (i == 0)
        is.clear(is.rdstate() | std::ios::failbit);
    else
    {
        dynamic_bitset<Block, Allocator> tmp(buf);
        b.swap(tmp);
    }
    return is;
}
#else // BOOST_OLD_IOSTREAMS

// Dynamic resizing is supported here, in a way similar to extraction
// of std::string-s. The strategy is to store the extracted bits in
// a vector<> of Blocks.
//
// [gps]
//
template <typename CharT, typename Traits, typename Block, typename Allocator>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is,
           dynamic_bitset<Block, Allocator>& b)
{
    // G.P.S. size_t vs size_type to be inspected

    using namespace std;

    // Check user-defined fmt variables
    //
    vector<Block> vect;
    size_t limit;
    if (can_extract_infinite_bitsets(is))
        limit = (static_cast<size_t>(-1)) - 1; // avoid using numeric_limits [gps]
    else {
        limit = bitset_max_bits(is);
        if (0 == limit) limit = b.size();
        vect.reserve(b.calc_num_blocks(limit)); // may throw std::bad_alloc
    }


    ios_base::iostate err = ios_base::goodbit; // G.P.S.
    CharT const zero = is.widen('0'); // in accordance with prop. resol. of lib dr 303.
    CharT const one  = is.widen('1');

    typename std::basic_istream<CharT, Traits>::sentry iprefix(is); // skips whitespaces
    try {

        if (iprefix) {
            size_t const bits_per_block = dynamic_bitset<Block, Allocator>::bits_per_block;

            basic_streambuf <CharT, Traits> * buf = is.rdbuf();// basic_streambuf � giusto?
            typename vector<Block>::iterator it;

            Block  mask      = 0;
            size_t bits_read = 0;
            typename Traits::int_type c = buf->sgetc(); // G.P.S.
            for (; err == ios_base::goodbit && bits_read < limit; c = buf->snextc()) {

                if (mask == 0) {
                    mask = Block(1) << (bits_per_block-1);
                    vect.push_back(0);
                    it = vect.end() - 1;
                }

                if (Traits::eq_int_type(Traits::eof(), c))
                    err |= ios_base::eofbit;   // G.P.S.
                else {
                    CharT to_c = Traits::to_char_type(c);

                    if (Traits::eq(to_c, one))
                        *it |= mask;
                    else if (!Traits::eq(to_c, zero)) // G.P.S.
                        break; // non digit character

                    mask >>= 1;
                    ++bits_read;
                }
            } // for

            if (bits_read)
                from_vect_of_blocks(vect, bits_read, b);

        }

    }

    catch (...) {
        // catches from stream buf, or from vector, except
        // bad_alloc originating from the initial vector<>::reserve()
        bool rethrow_original = false;
        try { is.setstate(ios_base::failbit); } catch(...) { rethrow_original = true; }

        if (rethrow_original)
            throw;

    }

    // Set error bits: note that this may throw
    if (err != ios_base::goodbit)
        is.setstate (err);

    return is;

}
#endif


//-----------------------------------------------------------------------------
// bitset operations

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator&(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b &= y;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator|(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b |= y;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator^(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b ^= y;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator-(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b -= y;
}


//-----------------------------------------------------------------------------
// private member functions


// If size() is not a multiple of bits_per_block
// then not all the bits in the last block are used.
// This function resets the unused bits (convenient
// for the implementation of many member functions)
//
template <typename Block, typename Allocator>
inline void dynamic_bitset<Block, Allocator>::m_zero_unused_bits()
{
    assert (this->num_blocks() == this->calc_num_blocks(this->m_num_bits));

    // if != 0 this is the number of bits used in the last block
    size_type const used_bits = this->m_num_bits % bits_per_block;

    if (used_bits != 0)
        this->m_bits[this->num_blocks() - 1] &= ~(~static_cast<Block>(0) << used_bits);

}


} // namespace boost


#undef BOOST_WORKAROUND_REPEAT_DEFAULT_TEMPLATE_ARGUMENTS
#undef LOCAL_BOOST_USE_FACET  // [gps]
#undef LOCAL_FACET_CONFIG_BUG  // [gps]

#endif // BOOST_DYNAMIC_BITSET_HPP


