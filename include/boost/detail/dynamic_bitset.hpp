// (C) Copyright Chuck Allison and Jeremy Siek 2001, 2002.
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all
// copies. This software is provided "as is" without express or
// implied warranty, and with no claim as to its suitability for any
// purpose.

// With optimizations by Gennaro Prota.

#ifndef BOOST_DETAIL_DYNAMIC_BITSET_HPP
#define BOOST_DETAIL_DYNAMIC_BITSET_HPP

#include "boost/config.hpp"
#include "boost/detail/workaround.hpp"
#include "boost/detail/iterator.hpp"

#if !(BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551)) || defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS))
#define BOOST_DYN_BITSET_USE_FRIENDS
#endif

namespace boost {

  namespace detail {

/* [gps]   // Forward references
    template <typename BlockInputIterator>
    std::size_t initial_num_blocks(BlockInputIterator first,
                                   BlockInputIterator last,
                                   std::input_iterator_tag);
    template <typename BlockForwardIterator>
    std::size_t initial_num_blocks(BlockForwardIterator first,
                                   BlockForwardIterator last,
                                   std::forward_iterator_tag);
*/

    // The following 2 classes make sure that the bitset
    // gets allocated in an exception safe manner
    template <typename Allocator>
    class dynamic_bitset_alloc_base {
    public:
      dynamic_bitset_alloc_base(const Allocator& alloc)
        : m_alloc(alloc) { }
    protected:
      Allocator m_alloc;
    };


    template <typename Block, typename Allocator>
    class dynamic_bitset_base :
#ifdef BOOST_DYN_BITSET_USE_FRIENDS
        protected
#else
        public
#endif
        dynamic_bitset_alloc_base<Allocator>
    {
      typedef std::size_t size_type;
#ifndef BOOST_DYN_BITSET_USE_FRIENDS
    public:
#endif
      enum { bits_per_block = CHAR_BIT * sizeof(Block) };
    public:
      dynamic_bitset_base()
        : m_bits(0), m_num_bits(0), m_num_blocks(0) { }

      dynamic_bitset_base(size_type num_bits, const Allocator& alloc)
        : dynamic_bitset_alloc_base<Allocator>(alloc),
          m_bits(dynamic_bitset_alloc_base<Allocator>::
                 m_alloc.allocate(calc_num_blocks(num_bits), static_cast<void const*>(0))),
          m_num_bits(num_bits),
          m_num_blocks(calc_num_blocks(num_bits))
      {
        using namespace std;
        memset(m_bits, 0, m_num_blocks * sizeof(Block)); // G.P.S. ask to Jeremy
      }
      ~dynamic_bitset_base() {
        if (m_bits)
          this->m_alloc.deallocate(m_bits, m_num_blocks);
      }
#ifdef BOOST_DYN_BITSET_USE_FRIENDS
    protected:
#endif
      Block* m_bits;
      size_type m_num_bits;
      size_type m_num_blocks;

      static size_type word(size_type bit)  { return bit / bits_per_block; } // [gps]
      static size_type offset(size_type bit){ return bit % bits_per_block; } // [gps]
      static Block mask1(size_type bit) { return Block(1) << offset(bit); }
      static Block mask0(size_type bit) { return ~(Block(1) << offset(bit)); }
      static size_type calc_num_blocks(size_type num_bits)
        { return (num_bits + (bits_per_block - 1)) / bits_per_block; }
    };


    // ------- count table implementation --------------

    typedef unsigned char byte_type;

    // only count<true> has a definition
#if 0
    // This was giving Intel C++ and Borland C++ trouble -JGS
    template <bool = true> struct count;
    template <> struct count <true> {
#else
    template <bool bogus = true>
    struct count {
#endif
        typedef byte_type element_type;
        static const byte_type table[];
        BOOST_STATIC_CONSTANT (unsigned int, max_bit = 8); // must be a power of two

    };
    //typedef count<true> table_t;


    // the table: wrapped in a class template, so
    // that it is only instantiated if/when needed
    //
#if 0
      // Intel C++ and Borland C++ trouble -JGS
     template <>
     const byte_type count <true>::table[] =
#else
     template <bool bogus>
     const byte_type count<bogus>::table[] =
#endif
     {
       // Automatically generated by GPTableGen.exe v.1.0
       //
     0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
     };


    // -------------------------------------------------------




    template <typename BlockInputIterator>
    inline std::size_t initial_num_blocks(BlockInputIterator first,
                   BlockInputIterator last,
                   std::input_iterator_tag)
    {
      return 0;
    }

    template <typename BlockForwardIterator>
    inline std::size_t initial_num_blocks(BlockForwardIterator first,
                   BlockForwardIterator last,
                   std::forward_iterator_tag)
    {
      std::size_t n = 0;
      while (first != last)
        ++first, ++n;
      return n;
    }

    template <typename RandomAccessIterator>
    inline std::size_t initial_num_blocks(RandomAccessIterator first,
                                          RandomAccessIterator last,
                                          std::random_access_iterator_tag)
    {
        return last - first; // [gps]

    }

    template <typename BlockInputIterator>
    inline std::size_t initial_num_blocks(BlockInputIterator first,
                   BlockInputIterator last)
    {
      typename detail::iterator_traits<BlockInputIterator>::iterator_category cat;
      return initial_num_blocks(first, last, cat);
    }


    // gives access to the object representation
    // of an object of type T (3.9p4)
    template <typename T>
    inline const unsigned char * object_representation (T* p)
    {
        return static_cast<const unsigned char *>(static_cast<const void *>(p));
    }

  } // namespace detail

} // namespace boost

#endif // BOOST_DETAIL_DYNAMIC_BITSET_HPP

