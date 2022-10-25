
#ifndef _NUDF_VALUE_TYPE_HPP__
#define _NUDF_VALUE_TYPE_HPP__


#include <string>
#include <vector>
#include <array>
#include <memory>

namespace NX {


// Various forward declarations.
namespace generic_detail {
    class GENERIC_VALUE;
    class GENERIC_NULL;
    class GENERIC_BOOLEAN;
    class GENERIC_NUMBER;
    class GENERIC_STRING;
    class GENERIC_DATETIME;
}

class generic_null;
class generic_boolean;
class generic_number;
class generic_string;
class generic_datetime;

class generic_value
{
public:
    generic_value();
    virtual ~generic_value() {}

public:
    typedef enum VALUETYPE {
        ValueNull = 0,
        ValueBoolean,
        ValueNumber,
        ValueString,
        ValueDatetime
    } VALUETYPE;

    generic_value(int v);
    generic_value(__int64 v);
    generic_value(unsigned int v);
    generic_value(unsigned __int64 v);
    generic_value(double v);
    explicit generic_value(bool b);
    explicit generic_value(const std::wstring& v);
    explicit generic_value(const std::wstring& v, bool has_escape_character);
    explicit generic_value(const wchar_t* v);
    explicit generic_value(const wchar_t* v, bool has_escape_character);
    explicit generic_value(const SYSTEMTIME* system_time);
    explicit generic_value(const FILETIME* file_time);
    generic_value(const generic_value& other);
    generic_value(generic_value&& other) noexcept;
    generic_value& operator=(const generic_value& other);
    generic_value& operator=(generic_value&& other) noexcept;

private:
    // use internally
    explicit generic_value(const std::string& v);

public:
    bool is_null() const;
    bool is_boolean() const;
    bool is_number() const;
    bool is_string() const;
    bool is_datetime() const;

    bool as_boolean() const;
    std::wstring as_string() const;
    int as_int32() const;
    __int64 as_int64() const;
    unsigned int as_uint32() const;
    unsigned __int64 as_uint64() const;
    double as_double() const;
    generic_number as_number() const;
    generic_datetime as_datetime() const;

    std::wstring serialize() const;

    virtual bool operator == (const generic_value& other) const;
    virtual bool operator != (const generic_value& other) const;
    virtual bool operator > (const generic_value& other) const;
    virtual bool operator >= (const generic_value& other) const;
    virtual bool operator < (const generic_value& other) const;
    virtual bool operator <= (const generic_value& other) const;

private:
    explicit generic_value(std::unique_ptr<generic_detail::GENERIC_VALUE> v) : _value(std::move(v)) {}

    void format(std::basic_string<wchar_t>& s) const;
    void format(std::basic_string<char>& s) const;

private:
    std::unique_ptr<generic_detail::GENERIC_VALUE> _value;
};

class generic_null : public generic_value
{
public:
    generic_null();
    virtual ~generic_null();
};

class generic_boolean : public generic_value
{
private:
    generic_boolean();
    explicit generic_boolean(bool v);
public:
    virtual ~generic_boolean();

    virtual generic_boolean& as_boolean() { return *this; }

    operator bool() const { return _boolean; }
    generic_boolean& operator = (bool b) { _boolean = b; return *this; }

    virtual bool operator == (const generic_boolean& other) const;
    virtual bool operator != (const generic_boolean& other) const;
    virtual bool operator > (const generic_boolean& other) const;
    virtual bool operator >= (const generic_boolean& other) const;
    virtual bool operator < (const generic_boolean& other) const;
    virtual bool operator <= (const generic_boolean& other) const;

private:
    friend class GENERIC_BOOLEAN;
    // not copyable
    generic_boolean& operator = (const generic_boolean& other) { return *this; }

private:
    bool  _boolean;
};

class generic_number : public generic_value
{
private:
    generic_number();
    generic_number(int v);
    generic_number(unsigned int v);
    generic_number(__int64 v);
    generic_number(unsigned __int64 v);
    generic_number(double v);
public:
    virtual ~generic_number();


    virtual bool is_number() const { return true; }
    virtual generic_number& as_number() { return *this; }

    inline bool is_integer() const { return !is_double(); }
    inline bool is_double() const { return (_number_type == double_value); }
    inline bool is_signed() const { return (_number_type != unsigned_value); }

    bool is_int32() const;
    bool is_int64() const;
    bool is_uint32() const;
    bool is_uint64() const;

    int to_int32() const;
    unsigned int to_uint32() const;
    __int64 to_int64() const;
    unsigned __int64 to_uint64() const;
    double to_double() const;

    virtual bool operator == (const generic_number& other) const;
    virtual bool operator != (const generic_number& other) const;
    virtual bool operator > (const generic_number& other) const;
    virtual bool operator >= (const generic_number& other) const;
    virtual bool operator < (const generic_number& other) const;
    virtual bool operator <= (const generic_number& other) const;

private:
    // not copyable
    generic_number& operator = (const generic_number& other) { return *this; }

private:
    union {
        __int64 _intval;
        unsigned __int64 _uintval;
        double  _dblval;
    };

    enum number_type {
        signed_value = 0, unsigned_value, double_value
    } _number_type;

    friend class generic_detail::GENERIC_NUMBER;
};

class generic_string : public generic_value
{
private:
    generic_string();
    generic_string(const std::wstring& s);
public:
    virtual ~generic_string();

    virtual bool is_string() const { return true; }
    virtual generic_string& as_string() { return *this; }

    inline const std::wstring& get_string() const { return _s; }
    inline void set_string(const std::wstring& s) { _s = s; }

    virtual size_t length() const { return _s.length(); }
    virtual bool empty() const { return _s.empty(); }
    virtual void clear() { _s.clear(); }

    virtual std::wstring serialize() const;

private:
    // not copyable
    generic_string& operator = (const generic_string& other) { return *this; }

private:
    std::wstring    _s;
};


class generic_datetime : public generic_value
{
private:
    generic_datetime();
    generic_datetime(const std::wstring& s);
    generic_datetime(__int64 v);
    generic_datetime(const SYSTEMTIME* v);
    generic_datetime(const FILETIME* v);
public:
    virtual ~generic_datetime() {}

    virtual bool is_datetime() const { return true; }
    virtual generic_datetime& as_datetime() { return *this; }

    virtual bool empty() const { return (_t == 0); }
    virtual void clear() { _t = 0; }

    virtual std::wstring serialize() const;

    inline __int64 as_int64_time() const { return _t; }
    void as_file_time(FILETIME* ft, bool local) const;
    void as_system_time(SYSTEMTIME* st, bool local) const;

private:
    // not copyable
    generic_datetime& operator = (const generic_datetime& other) { return *this; }

private:
    __int64    _t;
    friend class generic_detail::GENERIC_DATETIME;
};

namespace generic_detail {

class generic_exception : public std::exception
{
public:
    generic_exception() {}
    generic_exception(char const* const _Message) : std::exception(_Message) {}
    virtual ~generic_exception() {}
};

class GENERIC_VALUE
{
public:
    virtual std::unique_ptr<GENERIC_VALUE> _copy_value() = 0;
    
    // Common function used for serialization to strings and streams.
    virtual void serialize_impl(std::string& str) const
    {
        format(str);
    }

    virtual void serialize_impl(std::wstring& str) const
    {
        format(str);
    }

    virtual std::wstring to_string() const
    {
        std::wstring str;
        serialize_impl(str);
        return str;
    }

    virtual generic_value::VALUETYPE type() const { return generic_value::ValueNull; }

    virtual bool is_integer() const { throw generic_exception("not a number"); }
    virtual bool is_double() const { throw generic_exception("not a number"); }

    virtual generic_number as_number() { throw generic_exception("not a number"); }
    virtual double to_double() const { throw generic_exception("not a number"); }
    virtual int to_int32() const { throw generic_exception("not a number"); }
    virtual __int64 to_int64() const { throw generic_exception("not a number"); }
    virtual unsigned int to_uint32() const { throw generic_exception("not a number"); }
    virtual unsigned __int64 to_uint64() const { throw generic_exception("not a number"); }
    virtual bool as_bool() const { throw generic_exception("not a boolean"); }
    virtual std::wstring as_string() const { throw generic_exception("not a string"); }
    virtual generic_datetime as_datetime() const { throw generic_exception("not a datetime"); }

    virtual ~GENERIC_VALUE() {}

protected:
    GENERIC_VALUE() {}

    virtual void format(std::basic_string<char>& stream) const
    {
        stream.append("null");
    }

    virtual void format(std::basic_string<wchar_t>& stream) const
    {
        stream.append(L"null");
    }

private:
    friend class NX::generic_value;
};

class GENERIC_NULL : public GENERIC_VALUE
{
public:
    virtual std::unique_ptr<GENERIC_VALUE> _copy_value()
    {
        return std::make_unique<GENERIC_NULL>();
    }

    virtual generic_value::VALUETYPE type() const { return generic_value::ValueNull; }

    GENERIC_NULL() {}
};

class GENERIC_BOOLEAN : public GENERIC_VALUE
{
public:
    virtual std::unique_ptr<GENERIC_VALUE> _copy_value()
    {
        return std::make_unique<GENERIC_BOOLEAN>(*this);
    }

    virtual generic_value::VALUETYPE type() const { return generic_value::ValueBoolean; }

    virtual bool as_bool() const { return _bvalue; }

protected:
    virtual void format(std::basic_string<char>& stream) const
    {
        stream.append(_bvalue ? "true" : "false");
    }

    virtual void format(std::basic_string<wchar_t>& stream) const
    {
        stream.append(_bvalue ? L"true" : L"false");
    }

public:
    GENERIC_BOOLEAN(bool value) : _bvalue(value) { }

private:
    bool _bvalue;
};

class GENERIC_NUMBER : public GENERIC_VALUE
{
public:
    explicit GENERIC_NUMBER(double value) : _number(value) { }
    explicit GENERIC_NUMBER(int value) : _number(value) { }
    explicit GENERIC_NUMBER(unsigned int value) : _number(value) { }
    explicit GENERIC_NUMBER(__int64 value) : _number(value) { }
    explicit GENERIC_NUMBER(unsigned __int64 value) : _number(value) { }

    virtual std::unique_ptr<GENERIC_VALUE> _copy_value()
    {
        return std::make_unique<GENERIC_NUMBER>(*this);
    }

    virtual generic_value::VALUETYPE type() const { return generic_value::ValueNumber; }

    virtual bool is_integer() const { return _number.is_integer(); }
    virtual bool is_double() const { return !_number.is_integer(); }

    virtual double to_double() const
    {
        return _number.to_double();
    }

    virtual int to_int32() const
    {
        return _number.to_int32();
    }

    virtual __int64 to_int64() const
    {
        return _number.to_int64();
    }

    virtual unsigned int to_uint32() const
    {
        return _number.to_uint32();
    }

    virtual unsigned __int64 to_uint64() const
    {
        return _number.to_uint64();
    }

    virtual generic_number as_number() { return _number; }

protected:
    virtual void format(std::basic_string<char>& stream) const;
    virtual void format(std::basic_string<wchar_t>& stream) const;

private:
    generic_number _number;
};

template<typename CharType>
void append_escape_string(std::basic_string<CharType>& str, const std::basic_string<CharType>& escaped);

class GENERIC_STRING : public GENERIC_VALUE
{
public:
    GENERIC_STRING(std::wstring value);
    GENERIC_STRING(std::wstring value, bool escaped_chars);
    GENERIC_STRING(std::string &&value);
    GENERIC_STRING(std::string &&value, bool escape_chars);
    GENERIC_STRING(const GENERIC_STRING& other);

    GENERIC_STRING& operator=(const GENERIC_STRING& other)
    {
        if (this != &other) {
            copy_from(other);
        }
        return *this;
    }

    virtual std::unique_ptr<GENERIC_VALUE> _copy_value()
    {
        return std::make_unique<GENERIC_STRING>(*this);
    }

    virtual generic_value::VALUETYPE type() const { return generic_value::ValueString; }

    virtual std::wstring as_string() const { return _string; };

    virtual void serialize_impl(std::string& str) const
    {
        serialize_impl_char_type(str);
    }

    virtual void serialize_impl(std::wstring& str) const
    {
        serialize_impl_char_type(str);
    }


protected:
    virtual void format(std::basic_string<char>& str) const;
    virtual void format(std::basic_string<wchar_t>& str) const;

private:
    size_t get_reserve_size() const
    {
        return _string.size() + 2;
    }

    template <typename CharType>
    void serialize_impl_char_type(std::basic_string<CharType>& str) const
    {
        // To avoid repeated allocations reserve some space all up front.
        // size of string + 2 for quotes
        str.reserve(get_reserve_size());
        format(str);
    }

    inline void copy_from(const GENERIC_STRING& other)
    {
        _string = other._string;
        _has_escape_char = other._has_escape_char;
    }

    // There are significant performance gains that can be made by knowning whether
    // or not a character that requires escaping is present.
    static bool has_escape_chars(const GENERIC_STRING &str);

private:
    std::wstring _string;
    bool _has_escape_char;
};


class GENERIC_DATETIME : public GENERIC_VALUE
{
public:
    virtual std::unique_ptr<GENERIC_VALUE> _copy_value()
    {
        return std::make_unique<GENERIC_DATETIME>(*this);
    }

    virtual generic_value::VALUETYPE type() const { return generic_value::ValueBoolean; }

    virtual generic_datetime as_datetime() const { return _tvalue; }

protected:
    virtual void format(std::basic_string<char>& stream) const;
    virtual void format(std::basic_string<wchar_t>& stream) const;

public:
    explicit GENERIC_DATETIME(const std::wstring& s);
    explicit GENERIC_DATETIME(__int64 value);
    explicit GENERIC_DATETIME(const SYSTEMTIME* value);
    explicit GENERIC_DATETIME(const FILETIME* value);

private:
    generic_datetime _tvalue;
};

}   // namespace NX::generic_detail

}   // namespace NX

#endif  // _NUDF_VALUE_TYPE_HPP__