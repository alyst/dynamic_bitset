// (C) Copyright Jeremy Siek 2001.
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all
// copies. This software is provided "as is" without express or
// implied warranty, and with no claim as to its suitability for any
// purpose.


#include "bitset_test.hpp"
#include "boost/dynamic_bitset.hpp"

#include "boost/limits.hpp"
#include "boost/config.hpp"


template <typename Block>
void run_test_cases( BOOST_EXPLICIT_TEMPLATE_TYPE(Block) )
{
  typedef boost::dynamic_bitset<Block> bitset_type;
  typedef bitset_test< bitset_type > Tests;
  const int bits_per_block = bitset_type::bits_per_block;

  std::string long_string = get_long_string();

  std::size_t   N,
                ul_width    = std::numeric_limits<unsigned long>::digits,
                block_width = std::numeric_limits<Block>::digits;
  unsigned long numbers[]   = { 0, 40247,
                                std::numeric_limits<unsigned long>::max() };

  //=====================================================================
  // Test construction from unsigned long
  for (std::size_t i = 0; i < 3; ++i) {
    unsigned long number = numbers[i];
    N = 0;
    Tests::from_unsigned_long(N, number);

    N = std::size_t(0.7 * double(ul_width));
    Tests::from_unsigned_long(N, number);

    N = 1 * ul_width;
    Tests::from_unsigned_long(N, number);

    N = std::size_t(1.3 * double(ul_width));
    Tests::from_unsigned_long(N, number);

    N = std::size_t(0.7 * double(block_width));
    Tests::from_unsigned_long(N, number);

    N = block_width;
    Tests::from_unsigned_long(N, number);

    N = std::size_t(1.3 * double(block_width));
    Tests::from_unsigned_long(N, number);

    N = 3 * block_width;
    Tests::from_unsigned_long(N, number);
  }
  //=====================================================================
  // Test construction from a string
  {
    // case pos > str.size()
    Tests::from_string(std::string(""), 1, 1);

    // invalid arguments
    Tests::from_string(std::string("x11"), 0, 3);
    Tests::from_string(std::string("0y1"), 0, 3);
    Tests::from_string(std::string("10z"), 0, 3);

    // valid arguments
    Tests::from_string(std::string(""), 0, 0);
    Tests::from_string(std::string("0"), 0, 1);
    Tests::from_string(std::string("1"), 0, 1);
    Tests::from_string(long_string, 0, long_string.size());
  }
  //=====================================================================
  // Test construction from a block range
  {
    std::vector<Block> blocks;
    Tests::from_block_range(blocks);
  }
  {
    std::vector<Block> blocks(3);
    blocks[0] = static_cast<Block>(0);
    blocks[1] = static_cast<Block>(1);
    blocks[2] = ~Block(0);
    Tests::from_block_range(blocks);
  }
  {
    std::vector<Block> blocks(101);
    for (typename std::vector<Block>::size_type i = 0;
         i < blocks.size(); ++i)
      blocks[i] = i;
    Tests::from_block_range(blocks);
  }
  //=====================================================================
  // Test copy constructor
  {
    boost::dynamic_bitset<Block> b;
    Tests::copy_constructor(b);
  }
  {
    boost::dynamic_bitset<Block> b(std::string("0"));
    Tests::copy_constructor(b);
  }
  {
    boost::dynamic_bitset<Block> b(long_string);
    Tests::copy_constructor(b);
  }
  //=====================================================================
  // Test assignment operator
  {
    boost::dynamic_bitset<Block> a, b;
    Tests::assignment_operator(a, b);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("1")), b(std::string("0"));
    Tests::assignment_operator(a, b);
  }
  {
    boost::dynamic_bitset<Block> a(long_string), b(long_string);
    Tests::assignment_operator(a, b);
  }
  //=====================================================================
  // Test resize
  {
    boost::dynamic_bitset<Block> a;
    Tests::resize(a);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("0"));
    Tests::resize(a);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("1"));
    Tests::resize(a);
  }
  {
    boost::dynamic_bitset<Block> a(long_string);
    Tests::resize(a);
  }
  //=====================================================================
  // Test clear
  {
    boost::dynamic_bitset<Block> a;
    Tests::clear(a);
  }
  {
    boost::dynamic_bitset<Block> a(long_string);
    Tests::clear(a);
  }
  //=====================================================================
  // Test append bit
  {
    boost::dynamic_bitset<Block> a;
    Tests::append_bit(a);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("0"));
    Tests::append_bit(a);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("1"));
    Tests::append_bit(a);
  }
  {
    const int size_to_fill_all_blocks = 4 * bits_per_block;
    boost::dynamic_bitset<Block> a(size_to_fill_all_blocks, 255ul);
    Tests::append_bit(a);
  }
  {
    boost::dynamic_bitset<Block> a(long_string);
    Tests::append_bit(a);
  }
  //=====================================================================
  // Test append block
  {
    boost::dynamic_bitset<Block> a;
    Tests::append_block(a);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("0"));
    Tests::append_block(a);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("1"));
    Tests::append_block(a);
  }
  {
    const int size_to_fill_all_blocks = 4 * bits_per_block;
    boost::dynamic_bitset<Block> a(size_to_fill_all_blocks, 15ul);
    Tests::append_block(a);
  }
  {
    boost::dynamic_bitset<Block> a(long_string);
    Tests::append_block(a);
  }
  //=====================================================================
  // Test append block range
  {
    boost::dynamic_bitset<Block> a;
    std::vector<Block> blocks;
    Tests::append_block_range(a, blocks);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("0"));
    std::vector<Block> blocks(3);
    blocks[0] = static_cast<Block>(0);
    blocks[1] = static_cast<Block>(1);
    blocks[2] = ~Block(0);
    Tests::append_block_range(a, blocks);
  }
  {
    boost::dynamic_bitset<Block> a(std::string("1"));
    std::vector<Block> blocks(101);
    for (typename std::vector<Block>::size_type i = 0;
         i < blocks.size(); ++i)
      blocks[i] = i;
    Tests::append_block_range(a, blocks);
  }
  {
    boost::dynamic_bitset<Block> a(long_string);
    std::vector<Block> blocks(3);
    blocks[0] = static_cast<Block>(0);
    blocks[1] = static_cast<Block>(1);
    blocks[2] = ~Block(0);
    Tests::append_block_range(a, blocks);
  }
  //=====================================================================
  // Test bracket operator
  {
    boost::dynamic_bitset<Block> b1;
    std::vector<bool> bitvec1;
    Tests::operator_bracket(b1, bitvec1);
  }
  {
    boost::dynamic_bitset<Block> b(std::string("1"));
    std::vector<bool> bit_vec(1, true);
    Tests::operator_bracket(b, bit_vec);
  }
  {
    boost::dynamic_bitset<Block> b(long_string);
    std::size_t n = long_string.size();
    std::vector<bool> bit_vec(n);
    for (std::size_t i = 0; i < n; ++i)
      bit_vec[i] = long_string[n - 1 - i] == '0' ? 0 : 1;
    Tests::operator_bracket(b, bit_vec);
  }
}

int
test_main(int, char*[])
{
  run_test_cases<unsigned char>();
  run_test_cases<unsigned short>();
  run_test_cases<unsigned int>();
  run_test_cases<unsigned long>();
# ifdef BOOST_HAS_LONG_LONG
  run_test_cases<unsigned long long>();
# endif

  return EXIT_SUCCESS;
}
