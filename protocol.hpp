#ifndef protocol_hpp_20200903_133809_PDT
#define protocol_hpp_20200903_133809_PDT

#include "field.hpp"
#include <cpp/tuple.hpp>
#include <cpp/type_traits.hpp>
#include <cpp/cpp.hpp>
#include <string>
#include <vector>
#include <limits>

namespace tpl
{

/*********************************************************************************************************************
* Implementation of `MessageSerializer` class, which packs the data from a `Message` into a string.
*********************************************************************************************************************/
  namespace detail {

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
  } // namespace detail
/*********************************************************************************************************************
* Metafunction for retrieving a `Message` whose definition is stored in a tuple.
*
* Explanation: If the tuple were constructed in such a way that the `Message` classes were sorted in numerical order
* according to their associated IDs, then we could just use `std::get` to access the stored class.  Since there is
* no guarantee that IDs are sorted, or that even all IDs which are provided by the underlying `enum` even exist as
* `Message` definitions, we need a way to (at compile-time) iterate through the tuple in search of the matching ID.
*
* Since this is a compile-time rather than run-time search, there is no performance penalty for the ID lookup.
*
* Note: This class is actually not bound to `Message` objects, per se, but can be used for any tuple which stores
* tuples of arbitrary type in another tuple, to be accessed by a `size_t` index.
*********************************************************************************************************************/
  namespace detail {
    namespace impl
    {
      template<size_t ID, size_t IndexN, typename T, typename En = void>
      class matching_message_type_from;

      template<size_t ID, size_t IndexN, typename T>
      class matching_message_type_from<ID, IndexN, T, cpp::enable_if_t<cpp::tuple_size<T>::value == IndexN>>
      {
        struct invalid
        {
        /*
         * Getting an error here means you tried to access a message type using an invalid message ID,
         * e.g. an value that exists in an enumerator definition but wasn't supplied to the Protocol class.
         */
        private:
          invalid()               = delete;
          invalid(const invalid&) = delete;
          invalid(invalid&&)      = delete;
        };
      public:
        using type = invalid;
      };

      template<size_t ID, size_t IndexN, typename T>
      class matching_message_type_from<ID, IndexN, T, cpp::enable_if_t<cpp::tuple_size<T>::value != IndexN>>
      {
        using type_at_index = cpp::tuple_element_t<IndexN, T>;
      public:
        using type = typename cpp::conditional<ID == static_cast<size_t>(type_at_index::message_type_id()), type_at_index, typename matching_message_type_from<ID, IndexN + 1, T>::type>::type;
      };
    } // namespace impl

    template<size_t ID, typename TupleT>
      using matching_message_type_from = typename impl::matching_message_type_from<ID, 0, TupleT>::type;

  }// namespace detail
/*********************************************************************************************************************
* Implementation of `MessageDeserializer` class, which unpacks the data from a string into a `Message` object.
*********************************************************************************************************************/
  namespace detail {
    
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

  } /* namespace detail */
/*********************************************************************************************************************
* Implementation of `protocol::definition::visitor` class.
*********************************************************************************************************************/
  namespace detail {
    
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
            if(begin >= end)
            {
              throw std::length_error("read past end");
            }
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
            while(begin < end)
            {
              auto ch = *begin;
              do_accept(ch, begin, end);
            }
          }
      protected:
        using string_iter = cpp::string::const_iterator;
        auto do_accept(char id, string_iter& begin, const string_iter& end) -> void
          {
            if(begin >= end)
            {
              throw std::length_error("read past end");
            }
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

  /*!
   * \brief The `protocol` class associates a message protocol with an id-type, which can be any type which can be
   *        used--either directly or via conversion--as a tuple-index and is deterministic at compile-time.
   *
   *        This class provides the `definition` sub-class, which permits assignment of concrete `Message` definitions
   *        that are used to implement serialization methods.
   */
  template<typename ET>
  class protocol
  {
    using message_id_type = ET;
  public:
    /*!
     * \brief The `Message` sub-class describes the data types which are stored in a message; it is used by the
     *        serializer methods to pack message objects into their binary representations, which are stored in a 
     *        standard string.
     */
    template<message_id_type MessageTypeID, typename...MessageFieldTypes>
    class Message
    {
    public:
      using message_id_type = ET;
      using field_tuple_type  = cpp::tuple<MessageFieldTypes...>;
      using char_type         = cpp::string::value_type;
      static constexpr message_id_type message_type_id() { return MessageTypeID; }
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
    public:
      template<size_t I>
        auto get() const -> decltype(std::get<I>(this->fields_))
        {
          return std::get<I>(fields_);
        }
      template<size_t I>
        auto get() -> decltype(std::get<I>(this->fields_))
        {
          return std::get<I>(fields_);
        }
    private:
      auto serialize_message_type_id(cpp::string& s) const -> void
        {
          Field<char>::serialize(s, static_cast<char>(message_type_id()));
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
      using message_tuple = cpp::tuple<MessageTs...>;
      using message_id_type = ET;
    public:
      template<message_id_type E>
        using message = detail::matching_message_type_from<static_cast<size_t>(E), message_tuple>;
      template<message_id_type MT_ID, typename...ArgTs>
      auto make_message(ArgTs&&...args) -> detail::matching_message_type_from<static_cast<size_t>(MT_ID), message_tuple>
        {
          using message_type = detail::matching_message_type_from<static_cast<size_t>(MT_ID), message_tuple>;
          typename message_type::field_tuple_type fields(cpp::forward<ArgTs>(args)...);
          return message_type { cpp::move(fields) };
        }
      using visitor = detail::protocol_visitor<0, message_tuple>;
    };
    
  };
} /* namespace pt */
#endif//protocol_hpp_20200903_133809_PDT
