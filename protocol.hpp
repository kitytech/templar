#ifndef protocol_hpp_20200903_133809_PDT
#define protocol_hpp_20200903_133809_PDT

#include <cpp/tuple.hpp>
#include <cpp/type_traits.hpp>
#include <cpp/cpp.hpp>
#include <string>
#include <vector>
#include <limits>

namespace pc
{

  namespace detail {
    auto throw_error_past_end() -> void
      {
        throw cpp::runtime_error("deserialization past end of stream");
      }
    template<typename BeginT, typename EndT>
    auto validate_iterators(BeginT&& b, EndT&& e) -> void
      {
        if(b == e)
        {
          throw_error_past_end();
        }
      }
    /*!
     * \brief The SFINAE base class!  W00t.
     */
    template<typename FT, typename EnableT = void>
    class Field;

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
          validate_iterators(begin, end);
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
    class Field<FT, cpp::enable_if_t<cpp::is_integral<FT>::value && sizeof(FT) != 1>>
    {
      using string_iter = cpp::string::const_iterator;
      template<size_t N, typename = cpp::enable_if_t<N == sizeof(FT), void>>
      static auto endian_neutral_serialize(cpp::string& s, FT value) -> void
        {
        }
      template<size_t N, typename = cpp::enable_if_t<N != sizeof(FT), int>>
      static auto endian_neutral_serialize(cpp::string& s, FT value, void* = nullptr) -> void
        {
          constexpr size_t shift_amount = N * 8;
          char ch = (value >> shift_amount);
          s += ch;
          endian_neutral_serialize<N + 1>(s, value); 
        }
    public:
      static auto serialize(cpp::string& s, FT value) -> void
        {
          endian_neutral_serialize<0>(s, value); 
        }
      static auto deserialize(string_iter& begin, const string_iter& end) -> FT
        {
          FT result = 0;
          for(size_t i = 0; i < sizeof(FT); ++i)
          {
            validate_iterators(begin, end);
            FT byte = *begin;    
            size_t shift_amount = i * 8;
            result |= (byte << shift_amount);
            ++begin;
          }
          return result;
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
          cpp::string result;
          for(size_t i = 0; i < size; ++i)
          {
            validate_iterators(begin, end);
            result += *begin;
            ++begin;
          }
          return result;
        }
    };
    /*!
     * \brief Class of Field objects which are one byte wide, and need no endian conversion.
     */
    template<typename FT>
    class Field<FT, cpp::enable_if_t<cpp::is_integral<FT>::value && sizeof(FT) == 1>>
    {
      using string_iter = cpp::string::const_iterator;
    public:
      static auto serialize(cpp::string& s, FT value) -> void
        {
          char c = value;
          s += c;
        }
      static auto deserialize(string_iter& begin, const string_iter& end) -> FT
        {
          validate_iterators(begin, end);
          FT result = *begin;
          ++begin;
          return result;
        }
    };

    template<size_t IDX, typename TupleT, typename EnableT = void>
    class MessageSerializer {};

    template<size_t IDX, typename TupleT>
    class MessageSerializer<IDX, TupleT, cpp::enable_if_t<cpp::tuple_size<TupleT>::value == IDX>>
    {
    public:
      static auto serialize(cpp::string& s, const TupleT& t) -> void
        {
                
        }
    };

    template<size_t IDX, typename TupleT>
    class MessageSerializer<IDX, TupleT, cpp::enable_if_t<cpp::tuple_size<TupleT>::value != IDX>>
    {
    public:
      static auto serialize(cpp::string& s, const TupleT& t) -> void
        {
          using type = cpp::tuple_element_t<IDX, TupleT>;
          Field<type>::serialize(s, cpp::get<IDX>(t));
          MessageSerializer<IDX + 1, TupleT>::serialize(s, t);
        }
    };

    namespace impl
    {
      template<size_t ID, size_t IndexN, typename T, typename En = void>
      class matching_message_type_from;

      template<size_t ID, size_t IndexN, typename T>
      class matching_message_type_from<ID, IndexN, T, cpp::enable_if_t<cpp::tuple_size<T>::value == IndexN>>
      {
        struct invalid
        {
          invalid()               = delete;
          invalid(const invalid&) = delete;
          invalid(invalid&&)      = delete;
        };
      public:
        //static_assert(false, "ID not found in tuple T");
        using type = invalid;
      };

      template<size_t ID, size_t IndexN, typename T>
      class matching_message_type_from<ID, IndexN, T, cpp::enable_if_t<cpp::tuple_size<T>::value != IndexN>>
      {
        using type_at_index = cpp::tuple_element_t<IndexN, T>;
      public:
        using type = typename cpp::conditional<ID == static_cast<size_t>(type_at_index::message_type_id()), type_at_index, typename matching_message_type_from<ID, IndexN + 1, T>::type>::type;
        //using type = int;
      };
    } // namespace impl
    template<size_t ID, typename TupleT>
      using matching_message_type_from = typename impl::matching_message_type_from<ID, 0, TupleT>::type;

      template<size_t I, typename T, typename EnableT = void>
      class MessageDeserializer;

      template<size_t I, typename T>
      class MessageDeserializer<I, T, cpp::enable_if_t<I == cpp::tuple_size<T>::value>>
      {
          using tuple_type = T;
          using string_iter = cpp::string::const_iterator;
      public:
          static auto deserialize(string_iter& begin, const string_iter& end) -> tuple_type
          {
            return tuple_type {};
          }
        template<typename...ArgTs>
          static auto deserialize(string_iter& begin, const string_iter& end, ArgTs&&...args) -> tuple_type
          {
            return tuple_type { cpp::forward<ArgTs>(args)... };
          }
      };

      template<size_t I, typename T>
      class MessageDeserializer<I, T, cpp::enable_if_t<I != 0 && I != cpp::tuple_size<T>::value>>
      {
        using tuple_type = T;
        using string_iter = cpp::string::const_iterator;
      public:
        template<typename...ArgTs>
          static auto deserialize(string_iter& begin, const string_iter& end, ArgTs&&...args) -> tuple_type
          {
            using field_type = cpp::tuple_element_t<I, tuple_type>;
            auto field = Field<field_type>::deserialize(begin, end);
            return MessageDeserializer<I + 1, T>::deserialize(begin, end, cpp::forward<ArgTs>(args)..., field);
          }
      };

      template<typename T>
      class MessageDeserializer<0, T, cpp::enable_if_t<cpp::tuple_size<T>::value != 0>>
      {
        using tuple_type = T;
        using string_iter = cpp::string::const_iterator;
      public:
        static auto deserialize(string_iter& begin, const string_iter& end) -> tuple_type
          {
            using field_type = cpp::tuple_element_t<0, tuple_type>;
            auto field = Field<field_type>::deserialize(begin, end);
            return MessageDeserializer<1, T>::deserialize(begin, end, field);
          }
      };

      template<size_t I, typename MT, typename EnableT = void>
      class protocol_visitor;

      template<size_t I, typename MT>
      class protocol_visitor<I, MT, cpp::enable_if_t<I == cpp::tuple_size<MT>::value>>
      {
      protected:
        using string_iter = cpp::string::const_iterator;
        auto do_accept(char id, string_iter& begin, const string_iter& end) -> void {}
      };
      template<size_t I, typename MT>
      class protocol_visitor<I, MT, cpp::enable_if_t<I != 0 && I != cpp::tuple_size<MT>::value>> 
        : public protocol_visitor<I + 1, MT>
      {
        using message_type = cpp::tuple_element_t<I, MT>;
        virtual auto visit(const message_type&) -> void = 0;
        using string_iter = cpp::string::const_iterator;
      public:
        auto do_accept(char id, string_iter& begin, const string_iter& end) -> void
          {
            validate_iterators(begin, end);
            if(id == static_cast<char>(message_type::message_type_id()))
            {
              visit(message_type { MessageDeserializer<0, typename message_type::field_tuple_type>::deserialize(++begin, end) });
            }
            else
            {
              protocol_visitor<I + 1, MT>::do_accept(id, begin, end);
            }
          }
      };
      template<typename MT>
      class protocol_visitor<0, MT> 
        : public protocol_visitor<1, MT>
      {
        using message_type = cpp::tuple_element_t<0, MT>;
      public:
        auto accept(const cpp::string& s) -> void
          {
            if(s.empty()) return;
            auto begin = s.begin();
            auto end   = s.end();
            while(begin != end)
            {
              auto ch = *begin;
              do_accept(ch, begin, end);
            }
          }
      protected:
        using string_iter = cpp::string::const_iterator;
        auto do_accept(char id, string_iter& begin, const string_iter& end) -> void
          {
            validate_iterators(begin, end);
            if(id == static_cast<char>(message_type::message_type_id()))
            {
              visit(message_type { MessageDeserializer<0, typename message_type::field_tuple_type>::deserialize(++begin, end) });
            }
            else
            {
              protocol_visitor<1, MT>::do_accept(id, begin, end);
            }
          }
      private:
        virtual auto visit(const message_type&) -> void = 0;
      };
  } /* namespace detail */
  template<typename ET>
  class protocol
  {
    using message_type_enum = ET;
  public:
    template<message_type_enum MessageTypeID, typename...MessageFieldTypes>
    class Message
    {
    public:
      using message_type_enum = ET;
      using field_tuple_type  = cpp::tuple<MessageFieldTypes...>;
      using char_type         = cpp::string::value_type;
      static constexpr message_type_enum message_type_id() { return MessageTypeID; }
      static_assert(static_cast<size_t>(message_type_id()) < cpp::numeric_limits<char_type>::max(), "Message ID value too large for storage in type 'char'");
      explicit Message(field_tuple_type&& ft) 
      : fields_(cpp::move(ft))
      {}
      auto serialize() const -> cpp::string
        {
          cpp::string result;
          serialize(result);
          return result;
        }
      auto serialize(cpp::string& s) const -> void
        {
          static_assert(0 != cpp::tuple_size<field_tuple_type>::value, "");
          serialize_message_type_id(s);
          serialize_fields(s);
        }
      auto operator==(const Message& other) const -> bool { return is_equal_to(other); }
      auto fields() const -> const field_tuple_type& { return fields_; }
    private:
      field_tuple_type fields_;
      
      auto serialize_message_type_id(cpp::string& s) const -> void
        {
          detail::Field<char>::serialize(s, static_cast<char>(message_type_id()));
        }
      auto serialize_fields(cpp::string& s) const -> void
        {
          detail::MessageSerializer<0, field_tuple_type>::serialize(s, fields_);    
        }
      auto is_equal_to(const Message& other) const -> bool
        {
          return fields_ == other.fields_;
        }
    };

    template<typename...MessageTs>
    class definition 
    {
      using message_tuple_type = cpp::tuple<MessageTs...>;
      using message_type_enum = ET;
    public:
      template<message_type_enum MT_ID, typename...ArgTs>
      auto make_message(ArgTs&&...args) -> detail::matching_message_type_from<static_cast<size_t>(MT_ID), message_tuple_type>
        {
          using message_type = detail::matching_message_type_from<static_cast<size_t>(MT_ID), message_tuple_type>;
          typename message_type::field_tuple_type fields(cpp::forward<ArgTs>(args)...);
          return message_type { cpp::move(fields) };
        }
      using visitor = detail::protocol_visitor<0, message_tuple_type>;
    };
  
  };
} /* namespace pt */
#endif//protocol_hpp_20200903_133809_PDT
