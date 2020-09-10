#ifndef field_hpp_20200905_224327_PDT
#define field_hpp_20200905_224327_PDT

#include <string>
#include <stdexcept>
#include <cpp/cpp.hpp>
#include <cpp/type_traits.hpp>

namespace tpl {
/*!
 * \brief The SFINAE base class!  W00t.
 */
template<typename FT, typename EnableT = void>
class Field;

/* 
 * Class for preventing use of const types for message fields.
 */
template<typename FT>
class Field<FT, cpp::enable_if_t<!cpp::is_same<cpp::remove_const_t<FT>, FT>::value>>
{
  static_assert(cpp::is_same<FT, cpp::remove_reference_t<FT>>::value, "Field may not be a const type.");
};
/* 
 * Class for preventing use of reference types for message fields.
 */
template<typename FT>
class Field<FT, cpp::enable_if_t<!cpp::is_same<cpp::remove_reference_t<FT>, FT>::value>>
{
  static_assert(cpp::is_same<FT, cpp::remove_reference_t<FT>>::value, "Field may not be a reference type.");
};
/*!
 * \brief Boolean Field objects.
 */
template<>
class Field<bool>
{
  using string_iter = cpp::string::const_iterator;
public:
  static auto serialize(cpp::string& s, bool b) -> void
    {
      constexpr char ch_true  = '\01';
      constexpr char ch_false = '\00';
      s += (b == true? ch_true : ch_false);
    }
  static auto deserialize(string_iter& begin, const string_iter& end) -> bool
    {
      if(begin >= end)
      {
        throw std::length_error("read past end");
      }
      auto byte = *begin;
      bool result = byte == '\00'? false : true;
      ++begin;
      return result;
    }
};
/*!
 * \brief Class of Field objects which require endian conversion.
 */
template<typename FT>
class Field<FT, cpp::enable_if_t<cpp::is_integral<FT>::value>>
{
  using string_iter       = cpp::string::const_iterator;
  using unsigned_value_t  = cpp::make_unsigned_t<FT>;

  template<size_t N, typename = cpp::enable_if_t<N == sizeof(FT), void>>
  static auto endian_neutral_serialize(cpp::string& s, const unsigned_value_t& value) -> void
    {
    }
  template<size_t N, typename = cpp::enable_if_t<N != sizeof(FT), int>>
  static auto endian_neutral_serialize(cpp::string& s, const unsigned_value_t& value, void* = nullptr) -> void
    {
      constexpr size_t shift_amount = N * 8;
      uint8_t ch = (value >> shift_amount);
      s.append(1, ch);
      endian_neutral_serialize<N + 1>(s, value); 
    }
  template<size_t N, typename = cpp::enable_if_t<N == sizeof(FT), void>>
  static auto endian_neutral_deserialize(string_iter& begin, const string_iter& end) -> unsigned_value_t
    {
      return 0;
    }
  template<size_t N, typename = cpp::enable_if_t<N != sizeof(FT), int>>
  static auto endian_neutral_deserialize(string_iter& begin, const string_iter& end, void* = nullptr) -> unsigned_value_t
    {
      constexpr size_t shift_amount = N * 8;
      unsigned_value_t result = static_cast<uint8_t>(*begin); // cast to uint8_t from char necessary to avoid sign error 
                                                              // when converting to unsigned_value_t
      result <<= shift_amount;
      return result | endian_neutral_deserialize<N + 1>(++begin, end); 
    }
public:
  static auto serialize(cpp::string& s, FT value) -> void
    {
      endian_neutral_serialize<0>(s, static_cast<unsigned_value_t>(value));
    }
  static auto deserialize(string_iter& begin, const string_iter& end) -> FT
    {
      endian_neutral_deserialize<0>(begin, end);
    }
};
/*!
 * \brief String Field objects.
 */
template<>
class Field<cpp::string>
{
  using string_iter = cpp::string::const_iterator;
public:
  static auto serialize(cpp::string& s, const cpp::string& source) -> void
    {
      Field<cpp::string::size_type>::serialize(s, source.size());
      s += source;
    }
  static auto deserialize(string_iter& begin, const string_iter& end) -> cpp::string
    {
      auto size = Field<cpp::string::size_type>::deserialize(begin, end);
      if(end - begin < size)
      {
        throw std::length_error("encoded string length greater than remaining stream length");
      }
      cpp::string result;
      for(size_t i = 0; i < size; ++i)
      {
        result += *begin;
        ++begin;
      }
      return result;
    }
};
///*!
// * \brief Class of Field objects which are one byte wide, and need no endian conversion.
// */
//template<typename FT>
//class Field<FT, cpp::enable_if_t<cpp::is_integral<FT>::value && sizeof(FT) == 1>>
//{
//  using string_iter = cpp::string::const_iterator;
//public:
//  static auto serialize(cpp::string& s, FT value) -> void
//    {
//      char c = value;
//      s += c;
//    }
//  static auto deserialize(string_iter& begin, const string_iter& end) -> FT
//    {
//      if(begin >= end)
//      {
//        throw std::length_error("read past end");
//      }
//      FT result = *begin;
//      ++begin;
//      return result;
//    }
//};
} /* namespace tpl */

#endif//field_hpp_20200905_224327_PDT
