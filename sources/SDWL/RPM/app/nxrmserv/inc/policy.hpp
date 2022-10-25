

#ifndef __NXSERV_POLICY_HPP__
#define __NXSERV_POLICY_HPP__


#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>

#include <boost/algorithm/string.hpp>

#include <nudf/genericvalue.hpp>
#include <nudf/json.hpp>
#include <nudf/time.hpp>



// Forward class
class policy_bundle;
class obligation;

NX::generic_value json_to_generic(const NX::json_value& v);

#define EVAL_FLAG_USER              0x00000001
#define EVAL_FLAG_APP               0x00000002
#define EVAL_FLAG_HOST              0x00000004
#define EVAL_FLAG_RESOURCE          0x00000008
#define EVAL_FLAG_ENV               0x00000010
#define EVAL_FLAG_IGNORE_USER       (EVAL_FLAG_APP | EVAL_FLAG_HOST | EVAL_FLAG_RESOURCE | EVAL_FLAG_ENV)
#define EVAL_FLAG_IGNORE_APP        (EVAL_FLAG_USER | EVAL_FLAG_HOST | EVAL_FLAG_RESOURCE | EVAL_FLAG_ENV)
#define EVAL_FLAG_IGNORE_USER_APP   (EVAL_FLAG_HOST | EVAL_FLAG_RESOURCE | EVAL_FLAG_ENV)
#define EVAL_FLAG_ALL               (EVAL_FLAG_USER | EVAL_FLAG_APP | EVAL_FLAG_HOST | EVAL_FLAG_RESOURCE | EVAL_FLAG_ENV)

class eval_data
{
public:
    eval_data();
    eval_data(unsigned long flags);
    eval_data(const eval_data& other);
    eval_data(eval_data&& other);
    virtual ~eval_data();

    eval_data& operator = (const eval_data& other);
    eval_data& operator = (eval_data&& other);

    void push_data(const std::wstring& name, const NX::generic_value& value);

    inline const std::multimap<std::wstring, NX::generic_value>& get_values() const { return _values; }
    inline unsigned long get_flags() const { return _flags; }

private:
    std::multimap<std::wstring, NX::generic_value> _values;
    unsigned long   _flags;
};

class eval_result
{
public:
    eval_result();
    eval_result(unsigned __int64 rights);
    virtual ~eval_result();

    inline unsigned __int64 get_id() const { return _id; }
    inline unsigned __int64 get_rights() const { return _rights; }
    inline const std::map<std::wstring, std::shared_ptr<obligation>>& get_obligations() const { return _obligations; }
    inline void add_rights(unsigned __int64 rights) { _rights |= rights; }
    inline void remove_rights(unsigned __int64 rights) { _rights &= ~rights; }
    void add_obligation(const std::shared_ptr<obligation>& sp);

    bool has_obligation(const std::wstring& name) const;

    std::wstring get_rights_string() const;

private:
    const unsigned __int64 _id;
    unsigned __int64 _rights;
    std::map<std::wstring, std::shared_ptr<obligation>> _obligations;

    friend class policy_bundle;
};

class policy_expression : public boost::noncopyable
{
public:
    policy_expression() {}
    virtual ~policy_expression() {}

    virtual bool empty() const = 0;
    virtual bool is_logic_expression() const = 0;
    virtual bool is_condition_expression() const = 0;
    virtual bool evaluate(const eval_data& ed) const = 0;
};

typedef enum OPERATOR {
    LogicAnd = 0,
    LogicOr,
    Equal,
    NotEqual,
    GreaterThan,
    GreaterThanOrEqualTo,
    LessThan,
    LessThanOrEqualTo
} OPERATOR;

typedef enum ACTION {
    REVOKE = 0,
    GRANT
} ACTION;

typedef enum EXPRESSIONTYPE {
    LOGICEXPR = 0,
    CONDITIONEXPR
} EXPRESSIONTYPE;

class logic_expression : public policy_expression
{
public:
    logic_expression(OPERATOR op) : _operator(op) {}
    virtual ~logic_expression() {}

    virtual bool empty() const { return _expressions.empty(); }
    virtual bool is_logic_expression() const { return true; }
    virtual bool is_condition_expression() const { return false; }
    virtual bool evaluate(const eval_data& ed) const;

    bool logic_and() const { return (LogicAnd == _operator); }

private:
    OPERATOR _operator;
    std::vector<std::shared_ptr<policy_expression>> _expressions;

    friend class policy_bundle;
};

class condition_expression : public policy_expression
{
public:
    condition_expression(OPERATOR op) : _operator(op) {}
    virtual ~condition_expression() {}

    virtual bool empty() const { return _name.empty(); }
    virtual bool is_logic_expression() const { return false; }
    virtual bool is_condition_expression() const { return true; }
    virtual bool evaluate(const eval_data& ed) const;

    inline unsigned long get_condition_type() const { return _flags; }

private:
    condition_expression();

private:
    OPERATOR          _operator;
    std::wstring      _name;
    unsigned long     _flags;
    NX::generic_value _value;

    friend class policy_bundle;
};

class obligation
{
public:
    obligation() {}
    ~obligation() {}

    inline const std::wstring& get_name() const { return _name; }
    inline const std::map<std::wstring, NX::generic_value>& get_parameters() const { return _parameters; }

private:
    std::wstring    _name;
    std::map<std::wstring, NX::generic_value> _parameters;

    friend class policy_bundle;
};


class policy_object
{
public:
    policy_object();
    ~policy_object();

    bool evaluate(const eval_data& ed);

    inline bool grant() const { return (_action == GRANT); }
    inline bool revoke() const { return (_action == REVOKE); }
	unsigned __int64 _rights;
	unsigned __int64 _startdate;
	unsigned __int64 _enddate;

private:
    int              _id;
    std::wstring     _name;
    ACTION           _action;
    std::shared_ptr<policy_expression>  _subject_condition;
    std::shared_ptr<policy_expression>  _resource_condition;
    std::shared_ptr<policy_expression>  _environment_condition;
    std::map<std::wstring, std::shared_ptr<obligation>>  _obligations;

    friend class policy_bundle;
};

class policy_bundle : public boost::noncopyable
{
public:
    ~policy_bundle();

    inline bool empty() const { return _policies.empty(); }
    inline const std::wstring& get_version() const { return _version; }
    inline const std::wstring& get_issuer() const { return _issuer; }
    inline const std::wstring& get_issue_time() const { return _issue_time; }
    inline const std::vector<std::shared_ptr<policy_object>>& get_policies() const { return _policies; }

    static std::shared_ptr<policy_bundle> parse(const std::wstring& s);
    static std::shared_ptr<policy_bundle> parse(const NX::json_object& bundle_object);

    std::wstring serialize() const;
    void clear();
    void evaluate(const eval_data& ed, eval_result* result);
	unsigned __int64 get_rights() { return _grant_rights; }
	unsigned __int64 get_enddate() { return _enddate; }
	unsigned __int64 get_startdate() { return _startdate; }

protected:
    policy_bundle();

    static std::shared_ptr<policy_object> parse_policy_object(const NX::json_object& object);
    static std::shared_ptr<policy_expression> parse_policy_expression(const NX::json_object& object);
    static std::shared_ptr<policy_expression> parse_logic_expression(const NX::json_object& object);
    static std::shared_ptr<policy_expression> parse_condition_expression(const NX::json_object& object);
    static std::shared_ptr<obligation> parse_obligation(const NX::json_object& object);

private:
	unsigned __int64 _grant_rights;
	unsigned __int64 _enddate;
	unsigned __int64 _startdate;
    std::wstring    _version;
    std::wstring    _issuer;
    std::wstring    _issue_time;
    std::vector<std::shared_ptr<policy_object>> _policies;
};



#endif