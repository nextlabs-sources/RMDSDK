
#pragma once
#ifndef __NX_FUNCTION_DLL_HPP__
#define __NX_FUNCTION_DLL_HPP__


#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>


namespace NX {


class function_item
{
public:
    function_item();
    function_item(const function_item& other);
    function_item(unsigned int id, const std::string& name = std::string());
    virtual ~function_item();

    inline bool valid() const { return (-1 == _id); }
    inline bool empty() const { return (!valid() && nullptr != _fp); }
    inline unsigned int get_id() const { return _id; }
    inline const std::string& get_name() const { return _name; }
    inline void* get_function_pointer() const { return _fp; }

    function_item& operator = (const function_item& other);
    bool load(void* h);
    void clear();

private:
    unsigned int    _id;
    std::string     _name;
    void*           _fp;
};

template<unsigned long N>
class dll_instance
{
public:
    dll_instance() : _h(NULL)
    {
    }

    dll_instance(const std::vector<function_item>& items) : _h(NULL)
    {
        std::for_each(items.begin(), items.end(), [&](const function_item& item) {
            _functions[item.get_id()] = item;
        });
        assert(N == (unsigned long)items.size());
    }

    ~dll_instance()
    {
        unload();
    }

    virtual void load(const std::wstring& dll_file)
    {
        _h = ::LoadLibraryW(dll_file.c_str());
        if (NULL != _h) {
            for (auto pos = _functions.begin(); pos != _functions.end(); ++pos) {
                pos->second.load(_h);
            }
        }
    }

    virtual void unload()
    {
        for (auto pos = _functions.begin(); pos != _functions.end(); ++pos) {
            pos->second.clear();
        }
        if (NULL != _h) {
            FreeLibrary(_h);
            _h = NULL;
        }
    }

    inline bool is_loaded() const { return (NULL != _h); }
    inline void* operator[](unsigned long id) const
    {
        std::map<unsigned long, function_item>::const_iterator pos = _functions.find(id);
        return (pos == _functions.end()) ? nullptr : pos->second.get_function_pointer();
    }

    std::map<unsigned long, function_item>::iterator begin() { return _functions.begin(); }
    std::map<unsigned long, function_item>::iterator end() { return _functions.end(); }
    std::map<unsigned long, function_item>::const_iterator cbegin() const { return _functions.begin(); }
    std::map<unsigned long, function_item>::const_iterator cend() const { return _functions.end(); }

private:
    HMODULE _h;
    std::map<unsigned long, function_item> _functions;
};

#define EXECUTE(TYPE, INSTANCE, ID, ...)     (reinterpret_cast<TYPE>((INSTANCE)[ID]))(__VA_ARGS__)


}


#endif