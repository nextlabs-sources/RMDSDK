

#ifndef __NX_COMMON_STRING_H__
#define __NX_COMMON_STRING_H__


#include <string>
#include <vector>


namespace NX {



template <typename T>
class basic_string_buffer
{
public:
    basic_string_buffer(std::basic_string<T>& str, size_t len) : _s(str)
    {
        // ctor
        _buf.resize(len + 1, 0);
    }


    ~basic_string_buffer()
    {
        _s = std::basic_string<T>(_buf.data());      // copy to string passed by ref at construction
    }

        // auto conversion to serve as windows function parameter
    inline operator T* () throw() { return _buf.data(); }

private:
    // No copy allowed
    basic_string_buffer(const basic_string_buffer<T>& c) {}
    // No assignment allowed
    basic_string_buffer& operator= (const basic_string_buffer<T>& c) { return *this; }

private:
    std::basic_string<T>&   _s;
    std::vector<T>          _buf;
};

typedef basic_string_buffer<char>       string_buffer;
typedef basic_string_buffer<wchar_t>    wstring_buffer;

template<typename T>
void toupper(std::basic_string<T>& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}

template<typename T>
void tolower(std::basic_string<T>& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

template<typename T>
std::basic_string<T> dupupper(const std::basic_string<T>& s)
{
    std::basic_string<T> s2(s);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);
    return s2;
}

template<typename T>
std::basic_string<T> duplower(const std::basic_string<T>& s)
{
    std::basic_string<T> s2(s);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
    return s2;
}

template<typename T>
int icompare(T c1, T c2)
{
    c1 = ::tolower(c1);
    c2 = ::tolower(c2);
    return (c1 == c2) ? 0 : ((c1 > c2) ? 1 : -1);
}

template<typename T>
int icompare(const std::basic_string<T>& s1, const std::basic_string<T>& s2)
{
    return icompare<T>(s1.c_str(), s2.c_str());
}

template<typename T>
int icompare(const T* s1, const T* s2)
{
    while (*s1 || *s2)
    {
        int ret = icompare(*s1, *s2);
        if (0 != ret)
            return ret;
        ++s1;
        ++s2;
    }
    return 0;
}

template<typename T>
int incompare(const T* s1, const T* s2, size_t n)
{
    while ((*s1 || *s2) && n)
    {
        int ret = icompare(*s1, *s2);
        if (0 != ret)
            return ret;

        ++s1;
        ++s2;
        --n;
    }
    return 0;
}

template<typename T>
bool ibegin_with(const std::basic_string<T>& s, const std::basic_string<T>& s2)
{
    if (s.length() < s2.length())
        return false;
    return (0 == incompare<T>(s.c_str(), s2.c_str(), s2.length()));
}

template<typename T>
bool iend_with(const std::basic_string<T>& s, const std::basic_string<T>& s2)
{
    if (s.length() < s2.length())
        return false;
    return (0 == icompare<T>(s.c_str() + (s.length() - s2.length()), s2.c_str()));
}

template<typename T>
std::basic_string<T> get_file_directory(const std::basic_string<T>& s)
{
    auto pos = s.find_last_of((T)'\\');
    if (pos == std::basic_string<T>::npos)
        return std::basic_string<T>();
    return s.substr(0, pos);
}

template<typename T>
std::basic_string<T> get_file_name(const std::basic_string<T>& s)
{
    auto pos = s.find_last_of((T)'\\');
    if (pos == std::basic_string<T>::npos)
        return s;
    return s.substr(pos + 1);
}

template<typename T>
std::basic_string<T> itos(int n)
{
    return i64tos<T>(static_cast<__int64>(n));
}

template<typename T>
std::basic_string<T> i64tos(__int64 n)
{
    static const char* digits = "0123456789";
    std::basic_string<T> s;

    if (0 == n) {
        s.push_back((T)digits[0]);
        return s;
    }

    bool negetive = false;
    if (n < 0) {
        negetive = true;
        n = 0 - n;
    }
    while (0 != n) {
        int d = n % 10;
        n = n / 10;
        s.push_back((T)digits[d]);
    }
    if (negetive)
        s.push_back((T)'-');

    return std::basic_string<T>(s.rbegin(), s.rend());
}

template<typename T>
bool isdigit(T c)
{
    const int v = static_cast<int>(c);
    return (v >= '0' && v <= '9');
}

template<typename T>
bool ishex(T c)
{
    const int v = static_cast<int>(c);
    return ((v >= '0' && v <= '9') || (v >= 'A' && v <= 'F') || (v >= 'a' && v <= 'f'));
}

template<typename T>
bool isalpha(T c)
{
    const int v = static_cast<int>(c);
    return ((v >= 'A' && v <= 'Z') || (v >= 'a' && v <= 'z'));
}

template<typename T>
bool isalphanum(T c)
{
    const int v = static_cast<int>(c);
    return ((v >= 'A' && v <= 'Z') || (v >= 'a' && v <= 'z') || (v >= '0' && v <= '9'));
}

template<typename T>
int ctoi(T c)
{
    if (c >= (T)'0' && c <= (T)'9')
        return (c - '0');
    else if (c >= (T)'a' && c <= (T)'f')
        return (c - 'a' + 10);
    else if (c >= (T)'A' && c <= (T)'F')
        return (c - 'A' + 10);
    else
        return -1;
}

template<typename T>
__int64 stoi64(const std::basic_string<T>& s)
{
    __int64 n = 1;
    bool negetive = false;
    const T* p = s.c_str();

    if (*p == (T)'-')
    {
        negetive = true;
        ++p;
    }

    while (*p) {
        if (!isdigit(*p))
            break;
        __int64 n2 = 10 * n;
        if (n2 < 0) // overflow
            break;
        n2 = n2 + ctoi(*p);
        if (n2 < 0) // overflow
            break;
        n = n2;
        ++p;
    }
    if (negetive)
        n *= -1;

    return n;
}

template<typename T>
unsigned __int64 hstoi64(const std::basic_string<T>& s)
{
    unsigned __int64 n = 0;
    const T* p = s.c_str();
    if (p[0] == (T)'0' && (p[1] == (T)'x' || p[1] == (T)'X')) {
        p += 2;
    }

    int i = 0;
    while (*p && i < 8)
    {
        if (!ishex(*p))
            break;
        n <<= 4;
        n += ctoi(*p);
        ++p;
        ++i;
    }

    return n;
}

template<typename T>
std::vector<unsigned char> hstobin(const std::basic_string<T>& s)
{
    std::vector<unsigned char> v;
    size_t len = s.length();
    const T* p = s.c_str();

    if (s.empty())
        return v;

    if (0 != (len % 2))
    {
        if (!ishex(*p))
            return v;
        v.push_back(static_cast<unsigned char>(ctoi(*p)));
        --len;
        ++p;
    }

    while (len != 0) {
        // assert(len >= 2);
        unsigned char u = static_cast<unsigned char>(ctoi(*p));
        --len;
        ++p;
        u <<= 4;
        u |= static_cast<unsigned char>(ctoi(*p));
        --len;
        ++p;
        v.push_back(u);
    }

    return v;
}

template<typename T>
std::basic_string<T> utohs(unsigned char u)
{
    static const char* digits = "0123456789ABCDEF";
    std::basic_string<T> s;
    s.push_back((T)digits[(u >> 4) & 0xF]);
    s.push_back((T)digits[(u & 0xF)]);
    return s;
}

template<typename T>
std::basic_string<T> bintohs(const unsigned char* p, size_t n)
{
    std::basic_string<T> s;
    while (0 != n) {
        s += utohs<T>(*(p++));
        --n;
    }
    return s;
}

template<typename T>
std::basic_string<T> itohs(int n)
{
    std::basic_string<T> s;
    const unsigned char* pb = (const unsigned char*)(&n);
    for (int i = 0; i < 4; i++) {
        s += utohs<T>(pb[3 - i]);
    }
    return s;
}

template<typename T>
std::basic_string<T> i64tohs(__int64 n)
{
    std::basic_string<T> s;
    const unsigned char* pb = (const unsigned char*)(&n);
    for (int i = 0; i < 8; i++) {
        s += utohs<T>(pb[7 - i]);
    }
    return s;
}

template<typename T>
bool iswspace(T c)
{
	return (c == T(' ') || c == T('\t') || c == T('\r') || c == T('\n') || c == T('\f') || c == T('\v') || c == T('\r'));
}

template<typename T>
void trim_left(std::basic_string<T>& s)
{
	while (!s.empty() && iswspace<T>(s[0])) {
		s = s.substr(1);
	}
}

template<typename T>
void trim_right(std::basic_string<T>& s)
{
	while (!s.empty() && iswspace<T>(s[s.length() - 1])) {
		s = s.substr(0, s.length() - 1);
	}
}

template<typename T>
void trim(std::basic_string<T>& s)
{
	trim_left<T>(s);
	trim_right<T>(s);
}

template<typename T, T SEPARATOR>
std::basic_string<T> remove_head(const std::basic_string<T>& s, std::basic_string<T>& remain)
{
    std::basic_string<T> rs;

    if (s.empty())
        return std::basic_string<T>();

    std::basic_string<T>::size_type pos = s.find(SEPARATOR, 0);
    if (pos == std::basic_string<T>::npos) {
        remain.clear();
        rs = s;
    }
    else {
        rs = s.substr(0, pos);
        remain = s.substr(pos + 1);
    }

    return rs;
}

template<typename T, T SEPARATOR>
std::basic_string<T> remove_head(std::basic_string<T>& s)
{
    std::basic_string<T> rs;

    if (s.empty())
        return std::basic_string<T>();

    std::basic_string<T>::size_type pos = s.find(SEPARATOR, 0);
    if (pos == std::basic_string<T>::npos) {
        rs = s;
        s.clear();
    }
    else {
        rs = s.substr(0, pos);
        s = s.substr(pos + 1);
    }

    return rs;
}

template<typename T, T SEPARATOR>
std::basic_string<T> remove_tail(const std::basic_string<T>& s, std::basic_string<T>& remain)
{
    std::basic_string<T> rs;

    if (s.empty())
        return std::basic_string<T>();

    
    std::basic_string<T>::size_type pos = s.rfind(SEPARATOR);
    if (pos == std::basic_string<T>::npos) {
        remain.clear();
        rs = s;
    }
    else {
        rs = s.substr(pos + 1);
        remain = s.substr(0, pos);
    }

    return rs;
}

template<typename T, T SEPARATOR>
std::basic_string<T> remove_tail(std::basic_string<T>& s)
{
    std::basic_string<T> rs;

    if (s.empty())
        return std::basic_string<T>();

    std::basic_string<T>::size_type pos = s.rfind(SEPARATOR);
    if (pos == std::basic_string<T>::npos) {
        rs = s;
        s.clear();
    }
    else {
        rs = s.substr(pos + 1);
        s = s.substr(0, pos);
    }

    return rs;
}

template<typename T, T SEPARATOR>
std::vector<std::basic_string<T>> split(const std::basic_string<T>& s, bool needTrim=true)
{
    std::vector<std::basic_string<T>> vec;

    if (s.empty())
        return vec;

    std::basic_string<T> remain(s);
    do {
        std::basic_string<T> subs = remove_head<T, SEPARATOR>(remain);
		if (needTrim)
			trim<T>(subs);
        if (!subs.empty()) {
            vec.push_back(subs);
        }
    } while (!remain.empty());

    return vec;
}

template<typename T, T SEPARATOR>
std::basic_string<T> merge(const std::vector<std::basic_string<T>>& vec)
{
    std::basic_string<T> s;

    for (std::vector<std::basic_string<T>>::size_type i = 0; i < vec.size(); i++) {
        if (i > 0) {
            s += SEPARATOR;
        }
        s += vec[i];
    }

    return s;
}


}

#endif
