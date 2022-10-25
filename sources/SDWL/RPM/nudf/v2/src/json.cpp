

#include <Windows.h>
#include <assert.h>

#include <algorithm>

#include <nudf\json.hpp>
#include <nudf\string.hpp>
#include <nudf\conversion.hpp>


using namespace NX;

namespace NX {
namespace json_impl {

    

template <typename CharType>
class json_parser
{
public:
    json_parser() : _current_parsing_depth(0), _current_line(0), _current_column(0)
    {
    }

    ~json_parser()
    {
    }

    json_value parse()
    {
        CharType ch = peek_next_nwspace_char();   // find first non-white-space character
        if (ch == '{') {
            return parse_object();
        }
        else if (ch == '[') {
            return parse_array();
        }
        else {
            SetLastError(ERROR_INVALID_DATA);
            throw std::exception("Input is not JSON string");
        }
    }

protected:
    virtual bool is_eof(CharType ch) = 0;
    virtual CharType read_next_char() = 0;
    virtual CharType peek_next_char() = 0;

    inline void increase_line() { ++_current_line; _current_column = 0; }
    inline void increase_column() { ++_current_column; }
    inline size_t current_line() const { return _current_line; }
    inline size_t current_column() const { return _current_column; }

protected:
    CharType read_next_nwspace_char()
    {
        CharType ch = read_next_char();
        while (!is_eof(ch) && iswspace((int)ch)) {
            ch = read_next_char();
        }
        return ch;
    }
    CharType peek_next_nwspace_char()
    {
        CharType ch = peek_next_char();
        while (!is_eof(ch) && iswspace((int)ch)) {
            read_next_char(); // move to next
            ch = peek_next_char();
        }
        return ch;
    }

    inline std::wstring to_utf16(const std::wstring& s) { return s; }
    inline std::wstring to_utf16(const std::string& s) { return NX::conversion::utf8_to_utf16(s); }

protected:
    json_value parse_object()
    {
        json_value new_object = json_value::create_object();

        CharType ch = read_next_char(); // caller need to assure that next character is '{'
        assert(ch == CharType('{'));
        if (ch != CharType('{')) {
            throw std::exception("missing '{' at object beginning");
        }

        while (true) {

            ch = peek_next_nwspace_char();

            // object finished
            if (ch == CharType('}')) {
                read_next_char();   // ignore '}'
                break;
            }

            if (ch == CharType(',')) {
                read_next_char();   // ignore ','
                continue;
            }

            assert(ch == CharType('\"'));
            if (ch != CharType('\"')) {
                throw std::exception("missing '\"' at object name");
            }

            const std::wstring& object_name = this->to_utf16(inter_parse_string());

            ch = read_next_nwspace_char();
            assert(ch == CharType(':'));
            if (ch != CharType(':')) {
                throw std::exception("missing ':' in object item");
            }

            // get value
            ch = peek_next_nwspace_char();
            switch (ch)
            {
            case CharType(','): // value is empty (Null)
            case CharType('N'):
            case CharType('n'):
                new_object[object_name] = parse_null();
                break;
            case CharType('{'): // value is an object
                new_object[object_name] = parse_object();
                break;
            case CharType('['): // value is an array
                new_object[object_name] = parse_array();
                break;
            case CharType('T'): // value is a boolean
            case CharType('t'):
            case CharType('F'):
            case CharType('f'):
                new_object[object_name] = parse_boolean();
                break;
            case CharType('\"'):// value is a string
                new_object[object_name] = parse_string();
                break;
            case CharType('-'): // value is a number
            case CharType('0'):
            case CharType('1'):
            case CharType('2'):
            case CharType('3'):
            case CharType('4'):
            case CharType('5'):
            case CharType('6'):
            case CharType('7'):
            case CharType('8'):
            case CharType('9'):
                new_object[object_name] = parse_number();
                break;
            default:            // value begins with unexpected character
                throw std::exception("unexpected character in object value");
                break;
            }

        }

        return new_object;
    }

    json_value parse_array()
    {
        json_value new_array = json_value::create_array();

        CharType ch = read_next_char(); // caller need to assure that next character is '['
        assert(ch == CharType('['));
        if (ch != CharType('[')) {
            throw std::exception("missing '[' at array beginning");
        }

        while (true) {
            
            ch = peek_next_nwspace_char();

            // array finished
            if (ch == CharType(']')) {
                read_next_char();   // ignore ']'
                break;
            }

            if (ch == CharType(',')) {
                read_next_char();   // ignore ','
                continue;
            }

            // array item
            ch = peek_next_nwspace_char();
            switch (ch)
            {
            case CharType(','): // item is empty (Null)
            case CharType('N'):
            case CharType('n'):
                new_array.as_array().push_back(parse_null());
                break;
            case CharType('{'): // item is an object
                new_array.as_array().push_back(parse_object());
                break;
            case CharType('['): // item is an array
                new_array.as_array().push_back(parse_array());
                break;
            case CharType('T'): // item is a boolean
            case CharType('t'):
            case CharType('F'):
            case CharType('f'):
                new_array.as_array().push_back(parse_boolean());
                break;
            case CharType('\"'):// item is a string
                new_array.as_array().push_back(parse_string());
                break;
            case CharType('-'): // item is a number
            case CharType('0'):
            case CharType('1'):
            case CharType('2'):
            case CharType('3'):
            case CharType('4'):
            case CharType('5'):
            case CharType('6'):
            case CharType('7'):
            case CharType('8'):
            case CharType('9'):
                new_array.as_array().push_back(parse_number());
                break;
            default:            // item begins with unexpected character
                throw std::exception("unexpected character in object value");
                break;
            }

        }

        return new_array;
    }

    json_value parse_null()
    {
        CharType ch = read_next_char(); // caller need to assure that next character is ('N' or 'n')

        if (ch == CharType(',')) {
            return json_value::create_null();
        }
        else if (ch == CharType('N') || ch == CharType('n')) {
            ch = read_next_char();
            if (CharType('U') != ch && (CharType('u') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            ch = read_next_char();
            if (CharType('L') != ch && (CharType('l') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            ch = read_next_char();
            if (CharType('L') != ch && (CharType('l') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            return json_value::create_null();
        }
        else {
            throw std::exception("malformed null literal");
        }
    }

    json_value parse_boolean()
    {
        CharType ch = read_next_char(); // caller need to assure that next character is (")

        if (ch == CharType('T') || ch == CharType('t')) {
            ch = read_next_char();
            if (CharType('R') != ch && (CharType('r') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            ch = read_next_char();
            if (CharType('U') != ch && (CharType('u') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            ch = read_next_char();
            if (CharType('E') != ch && (CharType('e') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            return json_value(true);
        }
        else if (ch == CharType('F') || ch == CharType('f')) {
            ch = read_next_char();
            if (CharType('A') != ch && (CharType('a') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            ch = read_next_char();
            if (CharType('L') != ch && (CharType('l') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            ch = read_next_char();
            if (CharType('S') != ch && (CharType('s') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            ch = read_next_char();
            if (CharType('E') != ch && (CharType('e') != ch)) {
                throw std::exception("malformed boolean literal");
            }
            return json_value(false);
        }
        else {
            throw std::exception("malformed boolean literal");
        }
    }

    json_value parse_string()
    {
        std::basic_string<CharType> s = inter_parse_string();
        return json_value(NX::conversion::to_utf16(s));
    }

    std::basic_string<CharType> inter_parse_string()
    {
        std::basic_string<CharType> s;
        CharType ch = read_next_char(); // caller need to assure that next character is (")

        assert(ch == CharType('\"'));

        while (CharType('\"') != (ch = read_next_char())) 
		{
			if (ch == 0)
			{
				throw std::exception("error: unexpected file end");
			}

            if (CharType('\\') == ch) {

                ch = read_next_char();
                switch (ch)
                {
                case CharType('\"'):
                    s.push_back(CharType('\"'));
                    break;
                case CharType('\\'):
                    s.push_back(CharType('\\'));
                    break;
                case CharType('/'):
                    s.push_back(CharType('/'));
                    break;
                case CharType('b'):
                    s.push_back(CharType('\b'));
                    break;
                case CharType('f'):
                    s.push_back(CharType('\f'));
                    break;
                case CharType('r'):
                    s.push_back(CharType('\r'));
                    break;
                case CharType('n'):
                    s.push_back(CharType('\n'));
                    break;
                case CharType('t'):
                    s.push_back(CharType('\t'));
                    break;
                case CharType('u'):
                    // A four-hexdigit Unicode character.
                    // Transform into a 16 bit code point.
                    {
                        CharType hex_str[4] = {0, 0, 0, 0};
                        for (int i = 0; i < 4; i++) {
                            hex_str[i] = read_next_char();
                            if (!NX::utility::is_hex<CharType>(hex_str[i])) {
                                throw std::exception("bad hex value");
                            }
                        }
                        s += handle_hex_unicode(hex_str);
                    }
                    break;
                default:
                    throw std::exception("unsupported escaped character");
                    break;
                }
            }
            else {
                s.push_back(ch);
            }
        }

        return std::move(s);
    }

    json_value parse_hex_number()
    {
        std::basic_string<CharType> s;

        while (true) {

            CharType ch = peek_next_char();

            if (NX::utility::is_space(static_cast<int>(ch)) || CharType(',') == ch || CharType('}') == ch || CharType(']') == ch) {
                break;
            }

            // not white space nor token character, do real read
            read_next_char();

            if (!NX::utility::is_hex<CharType>(ch)) {
                throw std::exception("unexpected character in hex value");
            }

            s.push_back(ch);
        }

        return json_value(s.empty() ? 0ULL : std::stoull(s, 0, 16));
    }

    json_value parse_decimal(CharType cb)
    {
        std::basic_string<CharType> s;
        bool    is_float = false;
        bool    is_exponent = false;
        bool    is_signed = false;

        if (cb == CharType('-')) {
            is_signed = true;
        }
        if (cb == CharType('.')) {
            s.push_back(CharType('0'));
            is_float = true;
        }
        s.push_back(cb);
        while (true) {

            CharType ch = peek_next_char();

            if (NX::utility::is_space(static_cast<int>(ch)) || CharType(',') == ch || CharType('}') == ch || CharType(']') == ch) {
                break;
            }

            // not white space nor token character, do real read
            read_next_char();

            switch (ch)
            {
            case CharType('.'):
                if (is_float) {
                    throw std::exception("number syntax error: duplicate '.'");
                }
                is_float = true;
                s.push_back(ch);
                break;
            case CharType('E'):
            case CharType('e'):
                if (is_exponent) {
                    throw std::exception("number syntax error: duplicate 'E'");
                }
                is_exponent = true;
                s.push_back(ch);
                break;
            case CharType('0'):
            case CharType('1'):
            case CharType('2'):
            case CharType('3'):
            case CharType('4'):
            case CharType('5'):
            case CharType('6'):
            case CharType('7'):
            case CharType('8'):
            case CharType('9'):
                s.push_back(ch);
                break;
            default:
                throw std::exception("number syntax error: unexpected character");
                break;
            }
        }

        return (is_float || is_exponent) ? json_value(std::stod(s)) : json_value(std::stoull(s));
    }

    json_value parse_number()
    {
        CharType ch = 0;

        // check first character
        ch = read_next_nwspace_char();
        if (ch == CharType('0')) {
            CharType ch2 = peek_next_char();
            if (CharType('X') == ch2 || CharType('x') == ch2) {
                // hex
                return parse_hex_number();
            }
        }

        // not hex
        return parse_decimal(ch);
    }

protected:
    std::wstring handle_hex_unicode(const wchar_t* hex_str)
    {
        std::wstring s;
        wchar_t c = 0;
        for (int i = 0; i < 4; i++) {
            c <<= 4;
            c |= (wchar_t)NX::utility::hex_to_int<wchar_t>(hex_str[i]);
        }
        s.push_back(c);
        return std::move(s);
    }

    std::string handle_hex_unicode(const char* hex_str)
    {
        std::wstring s;
        wchar_t c = 0;
        for (int i = 0; i < 4; i++) {
            c <<= 4;
            c |= (wchar_t)NX::utility::hex_to_int<char>(hex_str[i]);
        }
        s.push_back(c);
        return std::move(NX::conversion::utf16_to_utf8(s));
    }

protected:
    size_t  _current_parsing_depth;
    size_t  _current_line;
    size_t  _current_column;
};


template <typename CharType>
class json_string_parser : public json_parser<CharType>
{
public:
    json_string_parser() : json_parser<CharType>()
    {
    }

    json_string_parser(const std::basic_string<CharType>& s) : json_parser<CharType>(), _start_pos(s.c_str()), _end_pos(s.c_str() + s.length()), _p(s.c_str())
    {
    }

    ~json_string_parser()
    {
    }

protected:
    virtual bool is_eof(CharType ch)
    {
        return (0 == ch);
    }

    virtual CharType read_next_char()
    {
        CharType ch = 0;

        if (_p == _end_pos) {
            return 0;
        }

        ch = *(_p++);
        if (ch == CharType('\n')) {
            ++_current_line;
            _current_column = 0;
        }
        else {
            ++_current_column;
        }

        return ch;
    }

    virtual CharType peek_next_char()
    {
        if (_p == _end_pos) {
            return 0;
        }
        return *_p;
    }

private:
    const CharType* _start_pos;
    const CharType* _end_pos;
    const CharType* _p;
};


}
}


using namespace NX::json_impl;

//
//  class json_value
//

json_value::json_value() : _value(std::make_unique<NX::json_detail::JSON_NULL>())
{
}

json_value::json_value(int v) : _value(std::make_unique<NX::json_detail::JSON_NUMBER>(v))
{
}

json_value::json_value(__int64 v) : _value(std::make_unique<NX::json_detail::JSON_NUMBER>(v))
{
}

json_value::json_value(unsigned int v) : _value(std::make_unique<NX::json_detail::JSON_NUMBER>(v))
{
}

json_value::json_value(unsigned __int64 v) : _value(std::make_unique<NX::json_detail::JSON_NUMBER>(v))
{
}

json_value::json_value(double v) : _value(std::make_unique<NX::json_detail::JSON_NUMBER>(v))
{
}

json_value::json_value(bool b) : _value(std::make_unique<NX::json_detail::JSON_BOOLEAN>(b))
{
}

json_value::json_value(const std::wstring& v) : _value(std::make_unique<NX::json_detail::JSON_STRING>(v))
{
}

json_value::json_value(const std::wstring& v, bool has_escape_character) : _value(std::make_unique<NX::json_detail::JSON_STRING>(v, has_escape_character))
{
}

json_value::json_value(const wchar_t* v) : _value(std::make_unique<NX::json_detail::JSON_STRING>(v))
{
}

json_value::json_value(const wchar_t* v, bool has_escape_character) : _value(std::make_unique<NX::json_detail::JSON_STRING>(v, has_escape_character))
{
}

json_value::json_value(const json_value& other) : _value(other._value->_copy_value())
{
}

json_value::json_value(json_value&& other) noexcept : _value(std::move(other._value))
{
}

json_value& json_value::operator=(const json_value& other)
{
    if (this != &other) {
        _value = std::unique_ptr<NX::json_detail::JSON_VALUE>(other._value->_copy_value());
    }
    return *this;
}

json_value& json_value::operator=(json_value&& other) noexcept
{
    if (this != &other) {
        _value.swap(other._value);
    }
    return *this;
}


json_value json_value::parse(const std::wstring& s)
{
    json_string_parser<wchar_t> parser(s);
    return parser.parse();
}

json_value json_value::parse(const std::string& s)
{
    json_string_parser<char> parser(s);
    return parser.parse();
}

json_value json_value::create_null()
{
    return json_null();
}

json_value json_value::create_array()
{
    std::unique_ptr<json_detail::JSON_VALUE> ptr = std::make_unique<json_detail::JSON_ARRAY>();
    return json_value(std::move(ptr));
}

json_value json_value::create_array(size_t size)
{
    std::unique_ptr<json_detail::JSON_VALUE> ptr = std::make_unique<json_detail::JSON_ARRAY>(size);
    return json_value(std::move(ptr));
}

json_value json_value::create_array(std::vector<json_value> elements)
{
    std::unique_ptr<json_detail::JSON_VALUE> ptr = std::make_unique<json_detail::JSON_ARRAY>(elements);
    return json_value(std::move(ptr));
}

json_value json_value::create_object()
{
    std::unique_ptr<json_detail::JSON_VALUE> ptr = std::make_unique<json_detail::JSON_OBJECT>(false);
    return json_value(std::move(ptr));
}

json_value json_value::create_object(bool keep_order)
{
    std::unique_ptr<json_detail::JSON_VALUE> ptr = std::make_unique<json_detail::JSON_OBJECT>(keep_order);
    return json_value(std::move(ptr));
}

json_value json_value::create_object(std::vector<std::pair<std::wstring, json_value>> fields, bool keep_order)
{
    std::unique_ptr<json_detail::JSON_VALUE> ptr = std::make_unique<json_detail::JSON_OBJECT>(fields, keep_order);
    return json_value(std::move(ptr));
}

bool json_value::is_null() const
{
    return (_value->type() == ValueNull);
}

bool json_value::is_boolean() const
{
    return (_value->type() == ValueBoolean);
}

bool json_value::is_number() const
{
    return (_value->type() == ValueNumber);
}

bool json_value::is_string() const
{
    return (_value->type() == ValueString);
}

bool json_value::is_array() const
{
    return (_value->type() == ValueArray);
}

bool json_value::is_object() const
{
    return (_value->type() == ValueObject);
}

bool json_value::operator == (const json_value& other) const
{
    if (_value->type() != other._value->type()) {
        return false;
    }

    if (is_null()) {
        return true;
    }
    else if (is_boolean()) {
        return (as_boolean() == other.as_boolean());
    }
    else if (is_number()) {
        return (as_number() == other.as_number());
    }
    else if (is_array()) {
        return (as_array() == other.as_array());
    }
    else if (is_object()) {
        return (as_object() == other.as_object());
    }
    else {
        return false;
    }
}

bool json_value::operator != (const json_value& other) const
{
    if (_value->type() != other._value->type()) {
        return true;
    }

    if (is_null()) {
        return false;
    }
    else if (is_boolean()) {
        return (as_boolean() != other.as_boolean());
    }
    else if (is_number()) {
        return (as_number() != other.as_number());
    }
    else if (is_array()) {
        return (as_array() != other.as_array());
    }
    else if (is_object()) {
        return (as_object() != other.as_object());
    }
    else {
        return false;
    }
}

json_value& json_value::operator [](const std::wstring& key)
{
    return as_object()[key];
}

json_value& json_value::operator [](size_t index)
{
    return as_array()[index];
}

const json_value& json_value::operator [](size_t index) const
{
    return as_array()[index];
}

void json_value::format(std::basic_string<wchar_t>& s) const
{
    _value->format(s);
}

void json_value::format(std::basic_string<char>& s) const
{
    _value->format(s);
}

bool json_value::as_boolean() const
{
    return _value->as_bool();
}

std::wstring json_value::as_string() const
{
    return _value->as_string();
}

int json_value::as_int32() const
{
    return _value->as_number().to_int32();
}

__int64 json_value::as_int64() const
{
    return _value->as_number().to_int64();
}

unsigned int json_value::as_uint32() const
{
    return _value->as_number().to_uint32();
}

unsigned __int64 json_value::as_uint64() const
{
    return _value->as_number().to_uint64();
}

double json_value::as_double() const
{
    return _value->as_number().to_double();
}

json_number json_value::as_number() const
{
    return _value->as_number();
}

json_array& json_value::as_array()
{
    return _value->as_array();
}

const json_array& json_value::as_array() const
{
    return _value->as_array();
}

json_object& json_value::as_object()
{
    return _value->as_object();
}

const json_object& json_value::as_object() const
{
    return _value->as_object();
}

std::wstring json_value::serialize() const
{
    std::wstring s;
    _value->serialize_impl(s);
    return std::move(s);
}


//
//  class json_null
//

json_null::json_null() : json_value()
{
}

json_null::~json_null()
{
}


//
//  class json_boolean
//

json_boolean::json_boolean() : json_value(), _boolean(false)
{
}

json_boolean::json_boolean(bool v) : json_value(), _boolean(v)
{
}

json_boolean::~json_boolean()
{
}

//
//  class json_number
//

json_number::json_number() : json_value(), _uintval(0), _number_type(unsigned_value)
{
}

json_number::json_number(int v) : json_value(), _intval(v), _number_type(v < 0 ? signed_value: unsigned_value)
{
}

json_number::json_number(unsigned int v) : json_value(), _uintval(v), _number_type(unsigned_value)
{
}

json_number::json_number(__int64 v) : json_value(), _intval(v), _number_type(v < 0 ? signed_value : unsigned_value)
{
}

json_number::json_number(unsigned __int64 v) : json_value(), _uintval(v), _number_type(unsigned_value)
{
}

json_number::json_number(double v) : json_value(), _dblval(v), _number_type(double_value)
{
}

json_number::~json_number()
{
}

bool json_number::is_int32() const
{
#pragma push_macro ("max")
#pragma push_macro ("min")
#undef max
#undef min
    switch (_number_type)
    {
    case signed_value: return (_intval >= std::numeric_limits<int32_t>::min() && _intval <= std::numeric_limits<int32_t>::max());
    case unsigned_value: return (_uintval <= std::numeric_limits<int32_t>::max());
    case double_value:
    default:
        return false;
    }
#pragma pop_macro ("min")
#pragma pop_macro ("max")
}

bool json_number::is_int64() const
{
#pragma push_macro ("max")
#pragma push_macro ("min")
#undef max
#undef min
    switch (_number_type)
    {
    case signed_value: return true;
    case unsigned_value: return _uintval <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    case double_value:
    default:
        return false;
    }
#pragma pop_macro ("min")
#pragma pop_macro ("max")
}

bool json_number::is_uint32() const
{
#pragma push_macro ("max")
#pragma push_macro ("min")
#undef max
#undef min
    switch (_number_type)
    {
    case signed_value: return _intval >= 0 && _intval <= std::numeric_limits<uint32_t>::max();
    case unsigned_value: return _uintval <= std::numeric_limits<uint32_t>::max();
    case double_value:
    default:
        return false;
    }
#pragma pop_macro ("min")
#pragma pop_macro ("max")
}

bool json_number::is_uint64() const
{
    switch (_number_type)
    {
    case signed_value: return _intval >= 0;
    case unsigned_value: return true;
    case double_value:
    default:
        return false;
    }
}

int json_number::to_int32() const
{
    if (is_double())
        return static_cast<int>(_dblval);
    else
        return static_cast<int>(_intval);
}

unsigned int json_number::to_uint32() const
{
    if (is_double())
        return static_cast<unsigned int>(_dblval);
    else
        return static_cast<unsigned int>(_intval);
}

__int64 json_number::to_int64() const
{
    if (is_double())
        return static_cast<__int64>(_dblval);
    else
        return static_cast<__int64>(_intval);
}

unsigned __int64 json_number::to_uint64() const
{
    if (is_double())
        return static_cast<unsigned __int64>(_dblval);
    else
        return static_cast<unsigned __int64>(_intval);
}

double json_number::to_double() const
{
    switch (_number_type)
    {
    case double_value: return _dblval;
    case signed_value: return static_cast<double>(_intval);
    case unsigned_value: return static_cast<double>(_uintval);
    default: return false;
    }
}

bool json_number::operator==(const json_number &other) const
{
    if (_number_type != other._number_type)
        return false;

    switch (_number_type)
    {
    case signed_value:
        return (_intval == other._intval);
    case unsigned_value:
        return (_uintval == other._uintval);
    case double_value:
        return (_dblval == other._dblval);
    }
    __assume(0);
}

bool json_number::operator!=(const json_number &other) const
{
    if (_number_type != other._number_type)
        return false;

    switch (_number_type)
    {
    case signed_value:
        return (_intval != other._intval);
    case unsigned_value:
        return (_uintval != other._uintval);
    case double_value:
        return (_dblval != other._dblval);
    }
    __assume(0);
}


//
//  class json_string
//

json_string::json_string() : json_value()
{
}

json_string::json_string(const std::wstring& s) : json_value(), _s(s)
{
}

json_string::~json_string()
{
}

std::wstring json_string::serialize() const
{
    // quota
    std::wstring s(L"\"");
    json_detail::append_escape_string<wchar_t>(s, _s);
    s += L"\"";
    return std::move(s);
}



//
//  class json_array
//

json_array::json_array() : json_value()
{
}

json_array::~json_array()
{
}

void json_array::remove(size_t index)
{
    if (index >= size()) {
        throw std::out_of_range(NX::string_formater("Id (%d) out of array range", index));
    }
    _elements.erase(begin() + index);
}

json_value& json_array::operator [](size_t index)
{
    if (index >= size()) {
        throw std::out_of_range(NX::string_formater("Id (%d) out of array range", index));
    }
    return _elements[index];
}

const json_value& json_array::operator [](size_t index) const
{
    if (index >= size()) {
        throw std::out_of_range(NX::string_formater("Id (%d) out of array range", index));
    }
    return _elements[index];
}

std::wstring json_array::serialize() const
{
    std::wstring s(L"[");
    std::for_each(cbegin(), cend(), [&](const json_value& v) {
        if (s.length() > 1) {
            s += L",";
        }
        s += v.serialize();
    });
    s += L"]";
    return std::move(s);
}



//
//  class json_object
//

json_object::json_object(bool keep_order) : json_value(), _elements(), _keep_order(keep_order)
{
}

json_object::json_object(storage_type elements, bool keep_order) : json_value(), _elements(std::move(elements)), _keep_order(keep_order)
{
    if (!keep_order) {
        std::sort(_elements.begin(), _elements.end(), [](const std::pair<std::wstring, json_value>& it1, const std::pair<std::wstring, json_value>& it2) -> bool {
            return (0 > _wcsicmp(it1.first.c_str(), it2.first.c_str()));
        });
    }
}

json_object::json_object(const json_object& obj)
{
    assert(false);
}

json_object::~json_object()
{
}

json_object& json_object::operator=(const json_object& obj)
{
    assert(false);
    return *this;
}

bool json_object::has_field(const std::wstring& key) const
{
    auto pos = find_by_key(key);
    return (pos != _elements.end() && 0 == _wcsicmp(pos->first.c_str(), key.c_str()));
}

json_value& json_object::at(const std::wstring& key)
{
    auto pos = find_by_key(key);
    if (pos != _elements.end() && 0 == _wcsicmp(pos->first.c_str(), key.c_str())) {
        return pos->second;
    }
    throw std::out_of_range(NX::string_formater("element (%s) not found", key.c_str()));
}

const json_value& json_object::at(const std::wstring& key) const
{
    auto pos = find_by_key(key);
    if (pos != _elements.end() && 0 == _wcsicmp(pos->first.c_str(), key.c_str())) {
        return pos->second;
    }
    throw std::out_of_range(NX::string_formater("element (%s) not found", key.c_str()));
}

json_value& json_object::operator [](const std::wstring& key)
{
    auto pos = find_by_key(key);
    if (pos != _elements.end() && 0 == _wcsicmp(key.c_str(), pos->first.c_str())) {
        return pos->second;
    }
    return _elements.insert(pos, std::pair<std::wstring, json_value>(key, json_value::create_null()))->second;
}

json_object::const_iterator json_object::find(const std::wstring& key) const
{
    auto pos = find_by_key(key);
    return (pos != _elements.end() && 0 != _wcsicmp(key.c_str(), pos->first.c_str())) ? _elements.end() : pos;
}

json_object::const_iterator json_object::find_by_key(const std::wstring& key) const
{
    if (_keep_order) {
        return std::find_if(_elements.begin(), _elements.end(), [&](const std::pair<std::wstring, json_value>& it) -> bool {
            return (0 == _wcsicmp(it.first.c_str(), key.c_str()));
        });
    }
    else {
        return std::lower_bound(_elements.begin(), _elements.end(), key, [](const std::pair<std::wstring, json_value>& it, const std::wstring& key) -> bool {
            return (0 > _wcsicmp(it.first.c_str(), key.c_str()));
        });
    }
}

json_object::iterator json_object::find_by_key(const std::wstring& key)
{
    if (_keep_order) {
        return std::find_if(_elements.begin(), _elements.end(), [&](const std::pair<std::wstring, json_value>& it) -> bool {
            return (0 == _wcsicmp(it.first.c_str(), key.c_str()));
        });
    }
    else {
        return std::lower_bound(_elements.begin(), _elements.end(), key, [](const std::pair<std::wstring, json_value>& it, const std::wstring& key) -> bool {
            return (0 > _wcsicmp(it.first.c_str(), key.c_str()));
        });
    }
}

void json_object::remove(const std::wstring& key)
{
    auto pos = find(key);
    if (pos != _elements.end()) {
        _elements.erase(pos);
    }
 }

std::wstring json_object::serialize() const
{
    std::wstring s(L"{");
    std::for_each(cbegin(), cend(), [&](const std::pair<std::wstring, json_value>& item) {
        if (s.length() > 1) {
            s += L",";
        }
        s += L"\"";
        json_detail::append_escape_string<wchar_t>(s, item.first);
        s += L"\": ";
        s += item.second.serialize();
    });
    s += L"}";
    return std::move(s);
}



void json_detail::JSON_NUMBER::format(std::basic_string<char>& stream) const
{
    if (is_double()) {
        stream.append(NX::string_formater("%f", _number.to_double()));
    }
    else {
        stream.append(NX::string_formater("%I64d", _number.to_int64()));
    }
}

void json_detail::JSON_NUMBER::format(std::basic_string<wchar_t>& stream) const
{
    if (is_double()) {
        stream.append(NX::string_formater(L"%f", _number.to_double()));
    }
    else {
        stream.append(NX::string_formater(L"%I64d", _number.to_int64()));
    }
}


json_detail::JSON_STRING::JSON_STRING(std::wstring value) : _string(std::move(value))
{
    _has_escape_char = has_escape_chars(*this);
}

json_detail::JSON_STRING::JSON_STRING(std::wstring value, bool escaped_chars)
    : _string(std::move(value)), _has_escape_char(escaped_chars)
{
}

json_detail::JSON_STRING::JSON_STRING(std::string &&value) : _string(NX::conversion::utf8_to_utf16(std::move(value)))
{
    _has_escape_char = has_escape_chars(*this);
}

json_detail::JSON_STRING::JSON_STRING(std::string &&value, bool escape_chars)
    : _string(NX::conversion::utf8_to_utf16(std::move(value))), _has_escape_char(escape_chars)
{
}

json_detail::JSON_STRING::JSON_STRING(const json_detail::JSON_STRING& other) : json_detail::JSON_VALUE(other)
{
    copy_from(other);
}

template<typename CharType>
void json_detail::append_escape_string(std::basic_string<CharType>& str, const std::basic_string<CharType>& escaped)
{
    for (auto iter = escaped.begin(); iter != escaped.end(); ++iter) {

        if (*iter == 0) {
            break;
        }

        switch (*iter)
        {
        case '\"':
            str += '\\';
            str += '\"';
            break;
        case '\\':
            str += '\\';
            str += '\\';
            break;
        case '\b':
            str += '\\';
            str += 'b';
            break;
        case '\f':
            str += '\\';
            str += 'f';
            break;
        case '\r':
            str += '\\';
            str += 'r';
            break;
        case '\n':
            str += '\\';
            str += 'n';
            break;
        case '\t':
            str += '\\';
            str += 't';
            break;
        default:
            str += *iter;
        }
    }
}

void json_detail::JSON_STRING::format(std::basic_string<char>& str) const
{
    str.push_back('\"');
    if (_has_escape_char) {
        json_detail::append_escape_string<char>(str, NX::conversion::utf16_to_utf8(_string));
    }
    else {
        str.append(NX::conversion::utf16_to_utf8(_string.c_str()));
    }
    str.push_back('\"');
}

void json_detail::JSON_STRING::format(std::basic_string<wchar_t>& str) const
{
    str.push_back(L'\"');
    if (_has_escape_char) {
        json_detail::append_escape_string<wchar_t>(str, _string);
    }
    else {
        str.append(_string.c_str());
    }
    str.push_back(L'\"');
}

bool json_detail::JSON_STRING::has_escape_chars(const JSON_STRING &str)
{
    static const auto escapes = L"\"\\\b\f\r\n\t";
    return (str._string.find_first_of(escapes) != std::wstring::npos);
}

bool json_detail::JSON_OBJECT::has_field(const std::wstring& key) const
{
    return (_object.cend() != _object.find(key));
}

json_value& json_detail::JSON_OBJECT::at(const std::wstring &key)
{
    return _object.at(key);
}

void json_detail::JSON_OBJECT::format(std::basic_string<char>& str) const
{
    bool first_element = true;
    str.push_back('{');
    std::for_each(_object.begin(), _object.end(), [&](const std::pair<std::wstring, json_value>& it) {
        if (first_element) {
            first_element = false;
        }
        else {
            str.push_back(',');
        }
        str.push_back('\"');
        json_detail::append_escape_string<char>(str, NX::conversion::utf16_to_utf8(it.first));
        str.push_back('\"');
        str.push_back(':');
        it.second.format(str);
    });
    str.push_back('}');
}

void json_detail::JSON_OBJECT::format(std::basic_string<wchar_t>& str) const
{
    bool first_element = true;
    str.push_back(L'{');
    std::for_each(_object.begin(), _object.end(), [&](const std::pair<std::wstring, json_value>& it) {
        if (first_element) {
            first_element = false;
        }
        else {
            str.push_back(L',');
        }
        str.push_back(L'\"');
        json_detail::append_escape_string<wchar_t>(str, it.first);
        str.push_back(L'\"');
        str.push_back(L':');
        it.second.format(str);
    });
    str.push_back(L'}');
}


void json_detail::JSON_ARRAY::format(std::basic_string<char>& str) const
{
    bool first_element = true;
    str.push_back('[');
    std::for_each(_array.begin(), _array.end(), [&](const json_value& it) {
        if (first_element) {
            first_element = false;
        }
        else {
            str.push_back(',');
        }
        it.format(str);
    });
    str.push_back(']');
}

void json_detail::JSON_ARRAY::format(std::basic_string<wchar_t>& str) const
{
    bool first_element = true;
    str.push_back(L'[');
    std::for_each(_array.begin(), _array.end(), [&](const json_value& it) {
        if (first_element) {
            first_element = false;
        }
        else {
            str.push_back(L',');
        }
        it.format(str);
    });
    str.push_back(L']');
}