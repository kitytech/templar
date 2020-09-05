#include "protocol.hpp"
#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>
using namespace std;


auto print_hex(unsigned b) -> void
{
  std::string s;
  auto add_nibble = [&](char c)
    {
      char n = c & 0xF;
      if(n < 0xA)
      {
        s += char('0' + n);
      }
      else if(n <= 0xF)
      {
        s += char('A' + n - 0xA);
      }
      else
      {
        throw std::runtime_error("too freakin' big!");
      }
    };
  auto add_byte = [&](char c)
    {
      add_nibble(c >> 4);
      add_nibble(c);
    };
  add_byte(b);
  std::cout << "x" << s;
}

auto print_hex(const std::string& s) -> void
{
  for(auto c : s)
  {
    print_hex(c);
  }
}
int main()
{
  enum class MessageType : uint8_t
      { SomeText
      , DrawLine
      , IsTrue
      , Crap1
      , Crap2
      , GoldenTurd
      , MAX
      };

  using protocol_t = pc::protocol<MessageType>;
  using protocol_definition = protocol_t::definition
    < protocol_t::Message<MessageType::SomeText, std::string>
    , protocol_t::Message<MessageType::DrawLine, short, int, long, uint8_t>
    , protocol_t::Message<MessageType::IsTrue,   bool>
    , protocol_t::Message<MessageType::GoldenTurd, std::string, bool>
    >;

  using protocol_visitor = protocol_definition::visitor;
  protocol_definition protocol;

  auto message_1 = protocol.make_message<MessageType::SomeText>("\001\002\003\004");
  auto message_2 = protocol.make_message<MessageType::DrawLine>(0, 1, 2, 3);
  auto message_3 = protocol.make_message<MessageType::IsTrue>(true);
  auto message_4 = protocol.make_message<MessageType::GoldenTurd>("Vincent Thacker", true);

  auto stream_1 = message_1.serialize();
  auto stream_2 = message_2.serialize();
  auto stream_3 = message_3.serialize();
  auto stream_4 = message_4.serialize();

  cout << stream_1.size() << ": "; print_hex(stream_1); cout << "\n";
  cout << stream_2.size() << ": "; print_hex(stream_2); cout << "\n";
  cout << stream_3.size() << ": "; print_hex(stream_3); cout << "\n";

  string combined;
  message_1.serialize(combined);
  message_2.serialize(combined);
  message_3.serialize(combined);
  message_4.serialize(combined);

  class A final : public protocol_visitor
  {
    using message_1_type = protocol_t::Message<MessageType::SomeText, std::string>;
    using message_2_type = protocol_t::Message<MessageType::DrawLine, short, int, long, uint8_t>;
    using message_3_type = protocol_t::Message<MessageType::IsTrue,   bool>;
    using message_4_type = protocol_t::Message<MessageType::GoldenTurd, std::string, bool>;
  public:
    A
    ( message_1_type& m1
    , message_2_type& m2
    , message_3_type& m3
    , message_4_type& m4
    )
      : message_1_ref_(m1)
      , message_2_ref_(m2)
      , message_3_ref_(m3)
      , message_4_ref_(m4)
      {}
  private:
    message_1_type& message_1_ref_;
    message_2_type& message_2_ref_;
    message_3_type& message_3_ref_;
    message_4_type& message_4_ref_;

    virtual auto visit(const protocol_t::Message<MessageType::SomeText, std::string>& arg) -> void override 
      {
        auto s = arg.serialize();
        cout << s.size() << ": "; print_hex(s); cout << "\n";
      }
    virtual auto visit(const protocol_t::Message<MessageType::DrawLine, short, int, long, uint8_t>& arg) -> void override 
      {
        auto s = arg.serialize();
        cout << s.size() << ": "; print_hex(s); cout << "\n";
      }
    virtual auto visit(const protocol_t::Message<MessageType::IsTrue,   bool>& arg) -> void override 
      {
        auto s = arg.serialize();
        cout << s.size() << ": "; print_hex(s); cout << "\n";
      }
    virtual auto visit(const protocol_t::Message<MessageType::GoldenTurd, std::string, bool>& arg) -> void override
      {
        auto s = arg.serialize();
        cout << s.size() << ": "; print_hex(s); cout << "\n";
        auto& fields = arg.fields();
        auto& name = std::get<0>(fields);
        bool  found = std::get<1>(fields);
        if(found)
        {
          cout << "Golden turd found by " << name << endl;
        }
        else
        {
          cout << name << " sought the golden turd, but it is lost forever!" << endl;
        }
      }
  };

  A a(message_1, message_2, message_3, message_4);
  a.accept(combined);
  return 0;
}

