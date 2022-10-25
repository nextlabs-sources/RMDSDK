

#pragma once
#ifndef __NUDF_JSON_HPP__
#define __NUDF_JSON_HPP__


#include <string>
#include <vector>
#include <array>
#include <memory>

namespace NX {



// Various forward declarations.
namespace json_detail {
    class JSON_VALUE;
    class JSON_NULL;
    class JSON_BOOLEAN;
    class JSON_NUMBER;
    class JSON_STRING;
    class JSON_OBJECT;
    class JSON_ARRAY;
}

class json_null;
class json_boolean;
class json_number;
class json_string;
class json_array;
class json_object;

class json_exception : public std::exception
{
public:
    json_exception() {}
    json_exception(char const* const _Message) : std::exception(_Message) {}
    virtual ~json_exception() {}
};

class json_value
{
public:
    typedef enum ValueType {
        ValueNull = 0,
        ValueBoolean,
        ValueNumber,
        ValueString,
        ValueObject,
        ValueArray
    } ValueType;

    json_value(int v);
    json_value(__int64 v);
    json_value(unsigned int v);
    json_value(unsigned __int64 v);
    json_value(double v);
    explicit json_value(bool b);
    explicit json_value(const std::wstring& v);
    explicit json_value(const std::wstring& v, bool has_escape_character);
    explicit json_value(const wchar_t* v);
    explicit json_value(const wchar_t* v, bool has_escape_character);
    json_value(const json_value& other);
    json_value(json_value&& other) noexcept;
    json_value& operator=(const json_value& other);
    json_value& operator=(json_value&& other) noexcept;
    virtual ~json_value() {}

private:
    // use internally
    explicit json_value(const std::string& v);

public:
    static json_value parse(const std::wstring& s);
    static json_value parse(const std::string& s);
    static json_value create_null();
    static json_value create_array();
    static json_value create_array(size_t size);
    static json_value create_array(std::vector<json_value> elements);
    static json_value create_object();
    static json_value create_object(bool keep_order);
    static json_value create_object(std::vector<std::pair<std::wstring, json_value>> fields, bool keep_order);

    bool is_null() const;
    bool is_boolean() const;
    bool is_number() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    bool as_boolean() const;
    std::wstring as_string() const;
    int as_int32() const;
    __int64 as_int64() const;
    unsigned int as_uint32() const;
    unsigned __int64 as_uint64() const;
    double as_double() const;

    json_number as_number() const;
    json_array& as_array();
    const json_array& as_array() const;
    json_object& as_object();
    const json_object& as_object() const;

    std::wstring serialize() const;

    bool operator == (const json_value& other) const;
    bool operator != (const json_value& other) const;

    json_value& operator [](const std::wstring& key);
    json_value& operator [](size_t index);
    const json_value& operator [](size_t index) const;
    
//protected:
    json_value();
        
private:
    friend class json_detail::JSON_OBJECT;
    friend class json_detail::JSON_ARRAY;

    explicit json_value(std::unique_ptr<json_detail::JSON_VALUE> v) : _value(std::move(v)) {}

    void format(std::basic_string<wchar_t>& s) const;
    void format(std::basic_string<char>& s) const;

private:
    std::unique_ptr<json_detail::JSON_VALUE> _value;
};

class json_null : public json_value
{
public:
    json_null();
    virtual ~json_null();
};

class json_boolean : public json_value
{
private:
    json_boolean();
    explicit json_boolean(bool v);
public:
    virtual ~json_boolean();
    
    virtual json_boolean& as_boolean() { return *this; }

    operator bool() const { return _boolean; }
    json_boolean& operator = (bool b) { _boolean = b; return *this; }
    
private:
    friend class JSON_BOOLEAN;
    // not copyable
    json_boolean& operator = (const json_boolean& other) { return *this; }

private:
    bool  _boolean;
};

class json_number : public json_value
{
private:
    json_number();
    json_number(int v);
    json_number(unsigned int v);
    json_number(__int64 v);
    json_number(unsigned __int64 v);
    json_number(double v);
public:
    virtual ~json_number();


    virtual bool is_number() const { return true; }
    virtual json_number& as_number() { return *this; }

    inline bool is_integer() const { return !is_double(); }
    inline bool is_double() const { return (_number_type == double_value); }
    inline bool is_signed() const {return (_number_type != unsigned_value); }

    bool is_int32() const;
    bool is_int64() const;
    bool is_uint32() const;
    bool is_uint64() const;

    int to_int32() const;
    unsigned int to_uint32() const;
    __int64 to_int64() const;
    unsigned __int64 to_uint64() const;
    double to_double() const;

    bool operator == (const json_number &other) const;
    bool operator != (const json_number &other) const;

private:
    // not copyable
    json_number& operator = (const json_number& other) { return *this; }

private:
    union {
        __int64 _intval;
        unsigned __int64 _uintval;
        double  _dblval;
    };

    enum number_type {
        signed_value = 0, unsigned_value, double_value
    } _number_type;

    friend class json_detail::JSON_NUMBER;
};

class json_string : public json_value
{
private:
    json_string();
    json_string(const std::wstring& s);
public:
    virtual ~json_string();

    virtual bool is_string() const { return true; }
    virtual json_string& as_string() { return *this; }

    inline const std::wstring& get_string() const { return _s; }
    inline void set_string(const std::wstring& s) { _s = s; }

    virtual size_t length() const { return _s.length(); }
    virtual bool empty() const { return _s.empty(); }
    virtual void clear() { _s.clear(); }

    virtual std::wstring serialize() const;

private:
    // not copyable
    json_string& operator = (const json_string& other) { return *this; }

private:
    std::wstring    _s;
};

class json_array : public json_value
{
    typedef std::vector<json_value> storage_type;

public:
    typedef storage_type::iterator iterator;
    typedef storage_type::const_iterator const_iterator;
    typedef storage_type::reverse_iterator reverse_iterator;
    typedef storage_type::const_reverse_iterator const_reverse_iterator;
    typedef storage_type::size_type size_type;

public:
    json_array();
    json_array(size_type size) : _elements(size) {}
    json_array(storage_type elements) : _elements(std::move(elements)) { }
    virtual ~json_array();

    virtual bool is_array() const { return true; }
    virtual json_array& as_array() { return *this; }

    virtual size_t size() const { return _elements.size(); }
    virtual bool empty() const { return _elements.empty(); }
    virtual void clear() { _elements.clear(); }

    iterator begin() { return _elements.begin(); }
    iterator end() { return _elements.end(); }
    const_iterator begin() const { return _elements.cbegin(); }
    const_iterator end() const { return _elements.cend(); }
    const_iterator cbegin() const { return _elements.cbegin(); }
    const_iterator cend() const { return _elements.cend(); }
    reverse_iterator rbegin() { return _elements.rbegin(); }
    reverse_iterator rend() { return _elements.rend(); }
    const_reverse_iterator rbegin() const { return _elements.crbegin(); }
    const_reverse_iterator rend() const { return _elements.crend(); }
    const_reverse_iterator crbegin() const { return _elements.crbegin(); }
    const_reverse_iterator crend() const { return _elements.crend(); }

    virtual void push_back(const json_value& v) { _elements.push_back(v); }

    virtual void remove(size_t index);
    virtual json_value& operator [](size_t index);
    virtual const json_value& operator [](size_t index) const;

    virtual std::wstring serialize() const;

    bool operator == (const json_array& other) const
    {
        if (_elements.size() != other._elements.size()) {
            return false;
        }
        for (size_t i = 0; i < _elements.size(); i++) {
            if (_elements[i] != other._elements[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator != (const json_array& other) const
    {
        if (_elements.size() != other._elements.size()) {
            return true;
        }
        for (size_t i = 0; i < _elements.size(); i++) {
            if (_elements[i] != other._elements[i]) {
                return true;
            }
        }
        // everything is equal
        return false;
    }

private:
    // not copyable
    json_array& operator = (const json_array& other) { return *this; }

private:
    storage_type _elements;
    friend class json_detail::JSON_ARRAY;
};

class json_object : public json_value
{
    typedef std::vector<std::pair<std::wstring, json_value>> storage_type;

public:
    typedef storage_type::iterator iterator;
    typedef storage_type::const_iterator const_iterator;
    typedef storage_type::reverse_iterator reverse_iterator;
    typedef storage_type::const_reverse_iterator const_reverse_iterator;
    typedef storage_type::size_type size_type;

private:
    json_object(bool keep_order = false);
    json_object(storage_type elements, bool keep_order = false);
    json_object(const json_object& obj); // non copyable
    json_object& operator=(const json_object& obj); // non copyable

public:
    virtual ~json_object();

    virtual bool is_object() const { return true; }
    virtual json_object& as_object() { return *this; }

    virtual size_type size() const { return _elements.size(); }
    virtual bool empty() const { return _elements.empty(); }
    virtual void clear() { _elements.clear(); _keep_order = false; }

    iterator begin() { return _elements.begin(); }
    iterator end() { return _elements.end(); }
    const_iterator begin() const { return _elements.cbegin(); }
    const_iterator end() const { return _elements.cend(); }
    const_iterator cbegin() const { return _elements.cbegin(); }
    const_iterator cend() const { return _elements.cend(); }
    reverse_iterator rbegin() { return _elements.rbegin(); }
    reverse_iterator rend() { return _elements.rend(); }
    const_reverse_iterator rbegin() const { return _elements.crbegin(); }
    const_reverse_iterator rend() const { return _elements.crend(); }
    const_reverse_iterator crbegin() const { return _elements.crbegin(); }
    const_reverse_iterator crend() const { return _elements.crend(); }

    bool has_field(const std::wstring& key) const;
    json_value& at(const std::wstring& key);
    const json_value& at(const std::wstring& key) const;
    json_value& operator [](const std::wstring& key);

    const_iterator find(const std::wstring& key) const;

    void remove(const std::wstring& key);

    virtual std::wstring serialize() const;

    bool operator == (const json_object& other) const
    {
        if (_elements.size() != other._elements.size()) {
            return false;
        }
        if (_keep_order != other._keep_order) {
            return false;
        }

        for (size_t i = 0; i < _elements.size(); i++) {
            if (_elements[i] != other._elements[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator != (const json_object& other) const
    {
        if (_elements.size() != other._elements.size()) {
            return true;
        }
        if (_keep_order != other._keep_order) {
            return true;
        }
        for (size_t i = 0; i < _elements.size(); i++) {
            if (_elements[i] != other._elements[i]) {
                return true;
            }
        }
        // everything is equal
        return false;
    }


private:
    const_iterator find_by_key(const std::wstring& key) const;
    iterator find_by_key(const std::wstring& key);

private:
    storage_type _elements;
    bool _keep_order;
    friend class json_detail::JSON_OBJECT;
};


namespace json_detail {


class JSON_VALUE
{
public:
    virtual std::unique_ptr<JSON_VALUE> _copy_value() = 0;

    virtual bool has_field(const std::wstring& key) const { return false; }
    virtual json_value get_field(const std::wstring& key) const { throw json_exception("not an object"); }
    virtual json_value get_element(json_array::size_type) const { throw json_exception("not an array"); }

    virtual json_value& at(const std::wstring& key) { throw json_exception("not an object"); }
    virtual json_value& at(json_array::size_type index) { throw json_exception("not an array"); }

    virtual const json_value& at(const std::wstring& key) const { throw json_exception("not an object"); }
    virtual const json_value& at(json_array::size_type) const { throw json_exception("not an array"); }
                             
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

    virtual json_value::ValueType type() const { return json_value::ValueNull; }

    virtual bool is_integer() const { throw json_exception("not a number"); }
    virtual bool is_double() const { throw json_exception("not a number"); }

    virtual json_number as_number() { throw json_exception("not a number"); }
    virtual double to_double() const { throw json_exception("not a number"); }
    virtual int to_int32() const { throw json_exception("not a number"); }
    virtual __int64 to_int64() const { throw json_exception("not a number"); }
    virtual unsigned int to_uint32() const { throw json_exception("not a number"); }
    virtual unsigned __int64 to_uint64() const { throw json_exception("not a number"); }
    virtual bool as_bool() const { throw json_exception("not a boolean"); }
    virtual json_array& as_array() { throw json_exception("not an array"); }
    virtual const json_array& as_array() const { throw json_exception("not an array"); }
    virtual json_object& as_object() { throw json_exception("not an object"); }
    virtual const json_object& as_object() const { throw json_exception("not an object"); }
    virtual std::wstring as_string() const { throw json_exception("not a string"); }

    virtual size_t size() const { return 0; }
    virtual ~JSON_VALUE() {}

protected:
    JSON_VALUE() {}

    virtual void format(std::basic_string<char>& stream) const
    {
        stream.append("null");
    }

    virtual void format(std::basic_string<wchar_t>& stream) const
    {
        stream.append(L"null");
    }

private:

    friend class NX::json_value;
};

class JSON_NULL : public JSON_VALUE
{
public:
    virtual std::unique_ptr<JSON_VALUE> _copy_value()
    {
        return std::make_unique<JSON_NULL>();
    }

    virtual json_value::ValueType type() const { return json_value::ValueNull; }

    JSON_NULL() {}
};

class JSON_BOOLEAN : public JSON_VALUE
{
public:
    virtual std::unique_ptr<JSON_VALUE> _copy_value()
    {
        return std::make_unique<JSON_BOOLEAN>(*this);
    }

    virtual json_value::ValueType type() const { return json_value::ValueBoolean; }

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
    JSON_BOOLEAN(bool value) : _bvalue(value) { }

private:
    bool _bvalue;
};

class JSON_NUMBER : public JSON_VALUE
{
public:
    explicit JSON_NUMBER(double value) : _number(value) { }
    explicit JSON_NUMBER(int value) : _number(value) { }
    explicit JSON_NUMBER(unsigned int value) : _number(value) { }
    explicit JSON_NUMBER(__int64 value) : _number(value) { }
    explicit JSON_NUMBER(unsigned __int64 value) : _number(value) { }

    virtual std::unique_ptr<JSON_VALUE> _copy_value()
    {
        return std::make_unique<JSON_NUMBER>(*this);
    }

    virtual json_value::ValueType type() const { return json_value::ValueNumber; }

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

    virtual json_number as_number() { return _number; }

protected:
    virtual void format(std::basic_string<char>& stream) const;
    virtual void format(std::basic_string<wchar_t>& stream) const;

private:
    json_number _number;
};

template<typename CharType>
void append_escape_string(std::basic_string<CharType>& str, const std::basic_string<CharType>& escaped);

class JSON_STRING : public JSON_VALUE
{
public:
    JSON_STRING(std::wstring value);
    JSON_STRING(std::wstring value, bool escaped_chars);
    JSON_STRING(std::string &&value);
    JSON_STRING(std::string &&value, bool escape_chars);
    JSON_STRING(const JSON_STRING& other);

    JSON_STRING& operator=(const JSON_STRING& other)
    {
        if (this != &other) {
            copy_from(other);
        }
        return *this;
    }

    virtual std::unique_ptr<JSON_VALUE> _copy_value()
    {
        return std::make_unique<JSON_STRING>(*this);
    }

    virtual json_value::ValueType type() const { return json_value::ValueString; }

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
    friend class JSON_OBJECT;
    friend class JSON_ARRAY;

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

    inline void copy_from(const JSON_STRING& other)
    {
        _string = other._string;
        _has_escape_char = other._has_escape_char;
    }
        
    // There are significant performance gains that can be made by knowning whether
    // or not a character that requires escaping is present.
    static bool has_escape_chars(const JSON_STRING &str);

private:
    std::wstring _string;
    bool _has_escape_char;
};

class JSON_OBJECT : public JSON_VALUE
{
public:
    JSON_OBJECT(bool keep_order) : _object(keep_order) { }

    JSON_OBJECT(json_object::storage_type fields, bool keep_order) : _object(std::move(fields), keep_order) { }

    JSON_OBJECT(const JSON_OBJECT& other) :
        JSON_VALUE(other), _object(other._object._elements, other._object._keep_order) {}

    virtual ~JSON_OBJECT() {}

    virtual std::unique_ptr<JSON_VALUE> _copy_value()
    {
        return std::make_unique<JSON_OBJECT>(*this);
    }

    virtual json_object& as_object() { return _object; }
    virtual const json_object& as_object() const { return _object; }

    virtual json_value::ValueType type() const { return json_value::ValueObject; }

    virtual bool has_field(const std::wstring &) const;

    virtual json_value& at(const std::wstring &key);

    bool is_equal(const JSON_OBJECT* other) const
    {
        if (_object.size() != other->_object.size())
            return false;

        return std::equal(std::begin(_object), std::end(_object), std::begin(other->_object));
    }

    size_t size() const { return _object.size(); }

protected:
    virtual void format(std::basic_string<char>& str) const;
    virtual void format(std::basic_string<wchar_t>& str) const;
    
private:
    json_object _object;
};

class JSON_ARRAY : public JSON_VALUE
{
public:
    JSON_ARRAY() {}
    JSON_ARRAY(json_array::size_type size) : _array(size) {}
    JSON_ARRAY(json_array::storage_type elements) : _array(std::move(elements)) { }

    virtual std::unique_ptr<JSON_VALUE> _copy_value()
    {
        return std::make_unique<JSON_ARRAY>(*this);
    }

    virtual json_value::ValueType type() const { return json_value::ValueArray; }

    virtual json_array& as_array() { return _array; }
    virtual const json_array& as_array() const { return _array; }

    virtual json_value& at(json_array::size_type index)
    {
        return _array[index];
    }

    bool is_equal(const JSON_ARRAY* other) const
    {
        if (_array.size() != other->_array.size())
            return false;

        auto iterT = _array.cbegin();
        auto iterO = other->_array.cbegin();
        auto iterTe = _array.cend();
        auto iterOe = other->_array.cend();

        for (; iterT != iterTe && iterO != iterOe; ++iterT, ++iterO) {
            if (*iterT != *iterO)
                return false;
        }

        return true;
    }

    size_t size() const { return _array.size(); }

protected:
    virtual void format(std::basic_string<char>& str) const;
    virtual void format(std::basic_string<wchar_t>& str) const;
    
private:
    json_array _array;
};

}
}


#endif