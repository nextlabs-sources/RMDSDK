
#include <Windows.h>
#include <assert.h>

#include <algorithm>

#include <nudf\genericvalue.hpp>
#include <nudf\string.hpp>
#include <nudf\conversion.hpp>
#include <nudf\time.hpp>


using namespace NX;



//
//  class generic_value
//

generic_value::generic_value() : _value(std::make_unique<NX::generic_detail::GENERIC_NULL>())
{
}

generic_value::generic_value(int v) : _value(std::make_unique<NX::generic_detail::GENERIC_NUMBER>(v))
{
}

generic_value::generic_value(__int64 v) : _value(std::make_unique<NX::generic_detail::GENERIC_NUMBER>(v))
{
}

generic_value::generic_value(unsigned int v) : _value(std::make_unique<NX::generic_detail::GENERIC_NUMBER>(v))
{
}

generic_value::generic_value(unsigned __int64 v) : _value(std::make_unique<NX::generic_detail::GENERIC_NUMBER>(v))
{
}

generic_value::generic_value(double v) : _value(std::make_unique<NX::generic_detail::GENERIC_NUMBER>(v))
{
}

generic_value::generic_value(bool b) : _value(std::make_unique<NX::generic_detail::GENERIC_BOOLEAN>(b))
{
}

generic_value::generic_value(const std::wstring& v) : _value(std::make_unique<NX::generic_detail::GENERIC_STRING>(v))
{
}

generic_value::generic_value(const std::wstring& v, bool has_escape_character) : _value(std::make_unique<NX::generic_detail::GENERIC_STRING>(v, has_escape_character))
{
}

generic_value::generic_value(const wchar_t* v) : _value(std::make_unique<NX::generic_detail::GENERIC_STRING>(v))
{
}

generic_value::generic_value(const wchar_t* v, bool has_escape_character) : _value(std::make_unique<NX::generic_detail::GENERIC_STRING>(v, has_escape_character))
{
}

generic_value::generic_value(const SYSTEMTIME* system_time) : _value(std::make_unique<NX::generic_detail::GENERIC_DATETIME>(system_time))
{
}

generic_value::generic_value(const FILETIME* file_time) : _value(std::make_unique<NX::generic_detail::GENERIC_DATETIME>(file_time))
{
}

generic_value::generic_value(const generic_value& other) : _value(other._value->_copy_value())
{
}

generic_value::generic_value(generic_value&& other) noexcept : _value(std::move(other._value))
{
}

generic_value& generic_value::operator=(const generic_value& other)
{
    if (this != &other) {
        _value = std::unique_ptr<NX::generic_detail::GENERIC_VALUE>(other._value->_copy_value());
    }
    return *this;
}

generic_value& generic_value::operator=(generic_value&& other) noexcept
{
    if (this != &other) {
        _value.swap(other._value);
    }
    return *this;
}

bool generic_value::is_null() const
{
    return (_value->type() == ValueNull);
}

bool generic_value::is_boolean() const
{
    return (_value->type() == ValueBoolean);
}

bool generic_value::is_number() const
{
    return (_value->type() == ValueNumber);
}

bool generic_value::is_string() const
{
    return (_value->type() == ValueString);
}

bool generic_value::is_datetime() const
{
    return (_value->type() == ValueDatetime);
}

bool generic_value::operator == (const generic_value& other) const
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
    else if (is_string()) {
        return (0 == _wcsicmp(as_string().c_str(), other.as_string().c_str()));
    }
    else if (is_datetime()) {
        return (as_datetime().as_int64_time() == other.as_datetime().as_int64_time());
    }
    else {
        return false;
    }
}

bool generic_value::operator != (const generic_value& other) const
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
    else if (is_string()) {
        return (0 != _wcsicmp(as_string().c_str(), other.as_string().c_str()));
    }
    else if (is_datetime()) {
        return (as_datetime().as_int64_time() != other.as_datetime().as_int64_time());
    }
    else {
        return false;
    }
}

bool generic_value::operator > (const generic_value& other) const
{
    if (_value->type() != other._value->type()) {
        return true;
    }

    if (is_null()) {
        return false;
    }
    else if (is_boolean()) {
        return (as_boolean() > other.as_boolean());
    }
    else if (is_number()) {
        return (as_number() > other.as_number());
    }
    else if (is_string()) {
        return (0 < _wcsicmp(as_string().c_str(), other.as_string().c_str()));
    }
    else if (is_datetime()) {
        return (as_datetime().as_int64_time() > other.as_datetime().as_int64_time());
    }
    else {
        return false;
    }
}

bool generic_value::operator >= (const generic_value& other) const
{
    if (_value->type() != other._value->type()) {
        return true;
    }

    if (is_null()) {
        return false;
    }
    else if (is_boolean()) {
        return (as_boolean() >= other.as_boolean());
    }
    else if (is_number()) {
        return (as_number() >= other.as_number());
    }
    else if (is_string()) {
        return (0 <= _wcsicmp(as_string().c_str(), other.as_string().c_str()));
    }
    else if (is_datetime()) {
        return (as_datetime().as_int64_time() >= other.as_datetime().as_int64_time());
    }
    else {
        return false;
    }
}

bool generic_value::operator < (const generic_value& other) const
{
    if (_value->type() != other._value->type()) {
        return true;
    }

    if (is_null()) {
        return false;
    }
    else if (is_boolean()) {
        return (as_boolean() < other.as_boolean());
    }
    else if (is_number()) {
        return (as_number() < other.as_number());
    }
    else if (is_string()) {
        return (0 > _wcsicmp(as_string().c_str(), other.as_string().c_str()));
    }
    else if (is_datetime()) {
        return (as_datetime().as_int64_time() < other.as_datetime().as_int64_time());
    }
    else {
        return false;
    }
}

bool generic_value::operator <= (const generic_value& other) const
{
    if (_value->type() != other._value->type()) {
        return true;
    }

    if (is_null()) {
        return false;
    }
    else if (is_boolean()) {
        return (as_boolean() <= other.as_boolean());
    }
    else if (is_number()) {
        return (as_number() <= other.as_number());
    }
    else if (is_string()) {
        return (0 >= _wcsicmp(as_string().c_str(), other.as_string().c_str()));
    }
    else if (is_datetime()) {
        return (as_datetime().as_int64_time() <= other.as_datetime().as_int64_time());
    }
    else {
        return false;
    }
}

void generic_value::format(std::basic_string<wchar_t>& s) const
{
    _value->format(s);
}

void generic_value::format(std::basic_string<char>& s) const
{
    _value->format(s);
}

bool generic_value::as_boolean() const
{
    return _value->as_bool();
}

std::wstring generic_value::as_string() const
{
    return _value->as_string();
}

int generic_value::as_int32() const
{
    return _value->as_number().to_int32();
}

__int64 generic_value::as_int64() const
{
    return _value->as_number().to_int64();
}

unsigned int generic_value::as_uint32() const
{
    return _value->as_number().to_uint32();
}

unsigned __int64 generic_value::as_uint64() const
{
    return _value->as_number().to_uint64();
}

double generic_value::as_double() const
{
    return _value->as_number().to_double();
}

generic_number generic_value::as_number() const
{
    return _value->as_number();
}

generic_datetime generic_value::as_datetime() const
{
    return _value->as_datetime();
}

std::wstring generic_value::serialize() const
{
    std::wstring s;
    _value->serialize_impl(s);
    return std::move(s);
}



//
//  class generic_null
//

generic_null::generic_null() : generic_value()
{
}

generic_null::~generic_null()
{
}


//
//  class generic_boolean
//

generic_boolean::generic_boolean() : generic_value(), _boolean(false)
{
}

generic_boolean::generic_boolean(bool v) : generic_value(), _boolean(v)
{
}

generic_boolean::~generic_boolean()
{
}

bool generic_boolean::operator == (const generic_boolean& other) const
{
    return (_boolean == other._boolean);
}

bool generic_boolean::operator != (const generic_boolean& other) const
{
    return (_boolean != other._boolean);
}

bool generic_boolean::operator > (const generic_boolean& other) const
{
    return (_boolean && (!other._boolean));
}

bool generic_boolean::operator >= (const generic_boolean& other) const
{
    return (_boolean || (_boolean == other._boolean));
}

bool generic_boolean::operator < (const generic_boolean& other) const
{
    return (_boolean && other._boolean);
}

bool generic_boolean::operator <= (const generic_boolean& other) const
{
    return (!_boolean || (_boolean == other._boolean));
}


//
//  class generic_number
//

generic_number::generic_number() : generic_value(), _uintval(0), _number_type(unsigned_value)
{
}

generic_number::generic_number(int v) : generic_value(), _intval(v), _number_type(v < 0 ? signed_value : unsigned_value)
{
}

generic_number::generic_number(unsigned int v) : generic_value(), _uintval(v), _number_type(unsigned_value)
{
}

generic_number::generic_number(__int64 v) : generic_value(), _intval(v), _number_type(v < 0 ? signed_value : unsigned_value)
{
}

generic_number::generic_number(unsigned __int64 v) : generic_value(), _uintval(v), _number_type(unsigned_value)
{
}

generic_number::generic_number(double v) : generic_value(), _dblval(v), _number_type(double_value)
{
}

generic_number::~generic_number()
{
}


bool generic_number::is_int32() const
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

bool generic_number::is_int64() const
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

bool generic_number::is_uint32() const
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

bool generic_number::is_uint64() const
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

int generic_number::to_int32() const
{
    if (is_double())
        return static_cast<int>(_dblval);
    else
        return static_cast<int>(_intval);
}

unsigned int generic_number::to_uint32() const
{
    if (is_double())
        return static_cast<unsigned int>(_dblval);
    else
        return static_cast<unsigned int>(_intval);
}

__int64 generic_number::to_int64() const
{
    if (is_double())
        return static_cast<__int64>(_dblval);
    else
        return static_cast<__int64>(_intval);
}

unsigned __int64 generic_number::to_uint64() const
{
    if (is_double())
        return static_cast<unsigned __int64>(_dblval);
    else
        return static_cast<unsigned __int64>(_intval);
}

double generic_number::to_double() const
{
    switch (_number_type)
    {
    case double_value: return _dblval;
    case signed_value: return static_cast<double>(_intval);
    case unsigned_value: return static_cast<double>(_uintval);
    default: return false;
    }
}

bool generic_number::operator==(const generic_number &other) const
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

bool generic_number::operator!=(const generic_number &other) const
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

bool generic_number::operator > (const generic_number& other) const
{
    if (_number_type != other._number_type)
        return false;

    switch (_number_type)
    {
    case signed_value:
        return (_intval > other._intval);
    case unsigned_value:
        return (_uintval > other._uintval);
    case double_value:
        return (_dblval > other._dblval);
    }
    __assume(0);
}

bool generic_number::operator >= (const generic_number& other) const
{
    if (_number_type != other._number_type)
        return false;

    switch (_number_type)
    {
    case signed_value:
        return (_intval >= other._intval);
    case unsigned_value:
        return (_uintval >= other._uintval);
    case double_value:
        return (_dblval >= other._dblval);
    }
    __assume(0);
}

bool generic_number::operator < (const generic_number& other) const
{
    if (_number_type != other._number_type)
        return false;

    switch (_number_type)
    {
    case signed_value:
        return (_intval < other._intval);
    case unsigned_value:
        return (_uintval < other._uintval);
    case double_value:
        return (_dblval < other._dblval);
    }
    __assume(0);
}

bool generic_number::operator <= (const generic_number& other) const
{
    if (_number_type != other._number_type)
        return false;

    switch (_number_type)
    {
    case signed_value:
        return (_intval <= other._intval);
    case unsigned_value:
        return (_uintval <= other._uintval);
    case double_value:
        return (_dblval <= other._dblval);
    }
    __assume(0);
}

//
//  class generic_string
//

generic_string::generic_string() : generic_value()
{
}

generic_string::generic_string(const std::wstring& s) : generic_value(), _s(s)
{
}

generic_string::~generic_string()
{
}

std::wstring generic_string::serialize() const
{
    // quota
    std::wstring s(L"\"");
    s += _s;
    s += L"\"";
    return std::move(s);
}



//
//  class generic_datetime
//

generic_datetime::generic_datetime() : _t(0)
{
}

generic_datetime::generic_datetime(const std::wstring& s)
{
    NX::time::datetime tm(s);
    _t = (__int64)tm;
}

generic_datetime::generic_datetime(__int64 v) : _t(v)
{
}

generic_datetime::generic_datetime(const SYSTEMTIME* v)
{
    NX::time::datetime tm(v);
    _t = (__int64)tm;
}

generic_datetime::generic_datetime(const FILETIME* v)
{
    NX::time::datetime tm(v);
    _t = (__int64)tm;
}

std::wstring generic_datetime::serialize() const
{
    if (empty()) {
        return std::wstring();
    }

    NX::time::datetime tm(_t);
    return tm.serialize(false, false);
}

void generic_datetime::as_file_time(FILETIME* ft, bool local) const
{
    NX::time::datetime tm(_t);
    tm.to_filetime(ft, local);
}

void generic_datetime::as_system_time(SYSTEMTIME* st, bool local) const
{
    NX::time::datetime tm(_t);
    tm.to_systemtime(st, local);
}


//
//  NX::detail
//

void generic_detail::GENERIC_NUMBER::format(std::basic_string<char>& stream) const
{
    if (is_double()) {
        stream.append(NX::string_formater("%f", _number.to_double()));
    }
    else {
        stream.append(NX::string_formater("%I64d", _number.to_int64()));
    }
}

void generic_detail::GENERIC_NUMBER::format(std::basic_string<wchar_t>& stream) const
{
    if (is_double()) {
        stream.append(NX::string_formater(L"%f", _number.to_double()));
    }
    else {
        stream.append(NX::string_formater(L"%I64d", _number.to_int64()));
    }
}


generic_detail::GENERIC_STRING::GENERIC_STRING(std::wstring value) : _string(std::move(value))
{
    _has_escape_char = has_escape_chars(*this);
}

generic_detail::GENERIC_STRING::GENERIC_STRING(std::wstring value, bool escaped_chars)
    : _string(std::move(value)), _has_escape_char(escaped_chars)
{
}

generic_detail::GENERIC_STRING::GENERIC_STRING(std::string &&value) : _string(NX::conversion::utf8_to_utf16(std::move(value)))
{
    _has_escape_char = has_escape_chars(*this);
}

generic_detail::GENERIC_STRING::GENERIC_STRING(std::string &&value, bool escape_chars)
    : _string(NX::conversion::utf8_to_utf16(std::move(value))), _has_escape_char(escape_chars)
{
}

generic_detail::GENERIC_STRING::GENERIC_STRING(const generic_detail::GENERIC_STRING& other) : generic_detail::GENERIC_VALUE(other)
{
    copy_from(other);
}

template<typename CharType>
void generic_detail::append_escape_string(std::basic_string<CharType>& str, const std::basic_string<CharType>& escaped)
{
    for (auto iter = escaped.begin(); iter != escaped.end(); ++iter) {

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

void generic_detail::GENERIC_STRING::format(std::basic_string<char>& str) const
{
    str.push_back('\"');
    if (_has_escape_char) {
        generic_detail::append_escape_string<char>(str, NX::conversion::utf16_to_utf8(_string));
    }
    else {
        str.append(NX::conversion::utf16_to_utf8(_string.c_str()));
    }
    str.push_back('\"');
}

void generic_detail::GENERIC_STRING::format(std::basic_string<wchar_t>& str) const
{
    str.push_back(L'\"');
    if (_has_escape_char) {
        generic_detail::append_escape_string<wchar_t>(str, _string);
    }
    else {
        str.append(_string.c_str());
    }
    str.push_back(L'\"');
}

bool generic_detail::GENERIC_STRING::has_escape_chars(const GENERIC_STRING &str)
{
    static const auto escapes = L"\"\\\b\f\r\n\t";
    return (str._string.find_first_of(escapes) != std::wstring::npos);
}


generic_detail::GENERIC_DATETIME::GENERIC_DATETIME(__int64 value) : _tvalue(value)
{
}

generic_detail::GENERIC_DATETIME::GENERIC_DATETIME(const std::wstring& s) : _tvalue(NX::time::datetime(s))
{
}

generic_detail::GENERIC_DATETIME::GENERIC_DATETIME(const SYSTEMTIME* value) : _tvalue(NX::time::datetime(value))
{
}

generic_detail::GENERIC_DATETIME::GENERIC_DATETIME(const FILETIME* value) : _tvalue(NX::time::datetime(value))
{
}

void generic_detail::GENERIC_DATETIME::format(std::basic_string<char>& stream) const
{
    NX::time::datetime tm(_tvalue.as_int64());
    stream.append(NX::conversion::utf16_to_utf8(tm.serialize(false, false)));
}

void generic_detail::GENERIC_DATETIME::format(std::basic_string<wchar_t>& stream) const
{
    NX::time::datetime tm(_tvalue.as_int64());
    stream.append(tm.serialize(false, false));
}