

#include <Windows.h>

#include <nudf/eh.hpp>
#include <nudf/string.hpp>
#include <nudf/conversion.hpp>
#include <nudf/time.hpp>
#include <nudf/json.hpp>
#include <nudf/handyutil.hpp>
#include <nudf/shared/rightsdef.h>
#include <nudf\dbglog.hpp>

#include "policy.hpp"


NX::generic_value json_to_generic(const NX::json_value& v)
{
    if (v.is_boolean()) {
        // Boolean
        return NX::generic_value(v.as_boolean());
    }
    else if (v.is_string()) {
        // String
        const std::wstring& str = v.as_string();
        if (NX::time::is_iso_8601_time_string(str)) {
            NX::time::datetime tm(str);
            SYSTEMTIME st = { 0 };
            tm.to_systemtime(&st, false);
            return NX::generic_value(&st);
        }
        else {
            return NX::generic_value(str);
        }
    }
    else if (v.is_number()) {
        // Number
        if (v.as_number().is_integer()) {
            if (v.as_number().is_signed()) {
                return NX::generic_value(v.as_int64());
            }
            else {
                return NX::generic_value(v.as_uint64());
            }
        }
        else {
            return NX::generic_value(v.as_double());
        }
    }
    else {
        ; // Not vaild
    }

    return NX::generic_value();
}

//
//  class policy_bundle
//

policy_bundle::policy_bundle()
{
	_grant_rights = 0;
	_enddate = 0;
}

policy_bundle::~policy_bundle()
{
}

std::shared_ptr<policy_object> policy_bundle::parse_policy_object(const NX::json_object& object)
{
    std::shared_ptr<policy_object> sp(new policy_object());

    try {
        
        sp->_id = object.at(L"id").as_int32();
        sp->_name = object.at(L"name").as_string();
        const int action = object.at(L"action").as_int32();
        if (REVOKE != action && GRANT != action) {
            throw std::exception("bad policy action");
        }
        sp->_action = (0 == action) ? REVOKE : GRANT;

        // Check rights
        const NX::json_array& rights_array = object.at(L"rights").as_array();
        std::for_each(rights_array.begin(), rights_array.end(), [&](const NX::json_value& v) {
            const std::wstring& rights_name = v.as_string();
            if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_VIEW)) {
                sp->_rights |= BUILTIN_RIGHT_VIEW;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_EDIT)) {
                sp->_rights |= BUILTIN_RIGHT_EDIT;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_PRINT)) {
                sp->_rights |= BUILTIN_RIGHT_PRINT;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_CLIPBOARD)) {
                sp->_rights |= BUILTIN_RIGHT_CLIPBOARD;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_SAVEAS)) {
                sp->_rights |= BUILTIN_RIGHT_SAVEAS;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_DECRYPT)) {
                sp->_rights |= BUILTIN_RIGHT_DECRYPT;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_SCREENCAP)) {
                sp->_rights |= BUILTIN_RIGHT_SCREENCAP;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_SEND)) {
                sp->_rights |= BUILTIN_RIGHT_SEND;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_CLASSIFY)) {
                sp->_rights |= BUILTIN_RIGHT_CLASSIFY;
            }
            else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_SHARE)) {
                sp->_rights |= BUILTIN_RIGHT_SHARE;
            }
			else if (0 == _wcsicmp(rights_name.c_str(), RIGHT_ACTION_DOWNLOAD)) {
				sp->_rights |= BUILTIN_RIGHT_DOWNLOAD;
			}
            else {
                ; // NOTHING
            }
        });

        sp->_rights &= BUILTIN_RIGHT_ALL;

		try {
			if (object.has_field(L"conditions"))
			{
				if (object.at(L"conditions").as_object().has_field(L"subject")) {
					sp->_subject_condition = parse_policy_expression(object.at(L"conditions").as_object().at(L"subject").as_object());
				}

				if (object.at(L"conditions").as_object().has_field(L"resource")) {
					sp->_resource_condition = parse_policy_expression(object.at(L"conditions").as_object().at(L"resource").as_object());
				}

				if (object.at(L"conditions").as_object().has_field(L"environment")) 
				{
					sp->_environment_condition = parse_policy_expression(object.at(L"conditions").as_object().at(L"environment").as_object());
					const NX::json_object& environment = object.at(L"conditions").as_object().at(L"environment").as_object();
					uint32_t type = environment.at(L"type").as_int32();
					if (1 == type) {   // AbsoluteExpire
						const std::wstring logic_operator = environment.at(L"operator").as_string();
						sp->_enddate = environment.at(L"value").as_uint64();
						LOGDEBUG(NX::string_formater(L" _enddate: AbsoluteExpire (%llu)", sp->_enddate));
					}
					else if (0 == type)
					{  // RangeExpire
						if (environment.has_field(L"expressions")) {
							const NX::json_array& expr = environment.at(L"expressions").as_array();
							std::for_each(expr.begin(), expr.end(), [&](const NX::json_value& v) {
								const std::wstring logic_operator = v.as_object().at(L"operator").as_string();
								unsigned __int64 date = v.as_object().at(L"value").as_uint64();
								if (logic_operator.compare(L"<=") == 0)
								{
									sp->_enddate = date;
									LOGDEBUG(NX::string_formater(L" _enddate: RangeExpire (%llu)", sp->_enddate));
								}
								else if (logic_operator.compare(L">=") == 0)
								{
									sp->_startdate = date;
									LOGDEBUG(NX::string_formater(L" _startdate: RangeExpire (%llu)", sp->_startdate));
								}
							});
						}
					}
				}
			}
		}
		catch (const std::exception& e) {
			UNREFERENCED_PARAMETER(e);
		}

        if (object.has_field(L"obligations")) {
            const NX::json_array& obligations = object.at(L"obligations").as_array();
            std::for_each(obligations.begin(), obligations.end(), [&](const NX::json_value& item) {
                try {
                    std::shared_ptr<obligation> ob = policy_bundle::parse_obligation(item.as_object());
                    if (ob != nullptr) {
                        sp->_obligations[ob->get_name()] = ob;
                    }
                }
                catch (const std::exception& e) {
                    UNREFERENCED_PARAMETER(e);
                }
            });
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        //sp.reset();
    }

    return sp;
}

std::shared_ptr<policy_expression> policy_bundle::parse_logic_expression(const NX::json_object& object)
{
    std::shared_ptr<logic_expression> sp;

    try {

        assert(LOGICEXPR == object.at(L"type").as_int32());
		uint32_t type = object.at(L"type").as_int32();
        const std::wstring logic_operator = object.at(L"operator").as_string();
        if (logic_operator == L"&&") {
            sp = std::shared_ptr<logic_expression>(new logic_expression(OPERATOR::LogicAnd));
        }
        else if (logic_operator == L"||") {
            sp = std::shared_ptr<logic_expression>(new logic_expression(OPERATOR::LogicOr));
        }
        else {
            throw std::exception("bad operator");
        }

        if (object.has_field(L"expressions")) {

            const NX::json_array& expression_array = object.at(L"expressions").as_array();
            for (NX::json_array::const_iterator it = expression_array.begin(); it != expression_array.end(); ++it) {

                std::shared_ptr<policy_expression> subexpr = parse_policy_expression((*it).as_object());
                if (subexpr != nullptr) {
                    sp->_expressions.push_back(subexpr);
                }
            }
        }

    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        sp.reset();
    }

    return sp;
}

std::shared_ptr<policy_expression> policy_bundle::parse_condition_expression(const NX::json_object& object)
{
    std::shared_ptr<condition_expression> sp;

    try {

        assert(CONDITIONEXPR == object.at(L"type").as_int32());
        const std::wstring condition_operator = object.at(L"operator").as_string();
        if (condition_operator == L"=") {
            sp = std::shared_ptr<condition_expression>(new condition_expression(OPERATOR::Equal));
        }
        else if (condition_operator == L"!=") {
            sp = std::shared_ptr<condition_expression>(new condition_expression(OPERATOR::NotEqual));
        }
        else if (condition_operator == L">") {
            sp = std::shared_ptr<condition_expression>(new condition_expression(OPERATOR::GreaterThan));
        }
        else if (condition_operator == L">=") {
            sp = std::shared_ptr<condition_expression>(new condition_expression(OPERATOR::GreaterThanOrEqualTo));
        }
        else if (condition_operator == L"<") {
            sp = std::shared_ptr<condition_expression>(new condition_expression(OPERATOR::LessThan));
        }
        else if (condition_operator == L"<=") {
            sp = std::shared_ptr<condition_expression>(new condition_expression(OPERATOR::LessThanOrEqualTo));
        }
        else {
            throw std::exception("bad operator");
        }

        sp->_name = NX::conversion::lower_str<wchar_t>(object.at(L"name").as_string());
        sp->_value = json_to_generic(object.at(L"value"));
        if (boost::algorithm::istarts_with(sp->_name, L"user.")) {
            sp->_flags = EVAL_FLAG_USER;
        }
        else if (boost::algorithm::istarts_with(sp->_name, L"application.")) {
            sp->_flags = EVAL_FLAG_APP;
        }
        else if (boost::algorithm::istarts_with(sp->_name, L"host.")) {
            sp->_flags = EVAL_FLAG_HOST;
        }
        else if (boost::algorithm::istarts_with(sp->_name, L"resource.")) {
            sp->_flags = EVAL_FLAG_RESOURCE;
        }
        else if (boost::algorithm::istarts_with(sp->_name, L"environment.")) {
            sp->_flags = EVAL_FLAG_ENV;
        }
        else {
            ; // Nothing
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        sp.reset();
    }

    return sp;
}

std::shared_ptr<policy_expression> policy_bundle::parse_policy_expression(const NX::json_object& object)
{
    std::shared_ptr<policy_expression> sp;

    try {

        const int expression_type = object.at(L"type").as_int32();

        if (LOGICEXPR == expression_type) {
            sp = parse_logic_expression(object);
        }
        else if (CONDITIONEXPR == expression_type) {
            sp = parse_condition_expression(object);
        }
        else {
            ; // bad
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        sp.reset();
    }

    return sp;
}

std::shared_ptr<obligation> policy_bundle::parse_obligation(const NX::json_object& object)
{
    std::shared_ptr<obligation> sp(new obligation());
    try {
        sp->_name = object.at(L"name").as_string();
        std::transform(sp->_name.begin(), sp->_name.end(), sp->_name.begin(), toupper);
        if (object.has_field(L"parameters")) {
            const NX::json_object& parameters = object.at(L"parameters").as_object();
            std::for_each(parameters.begin(), parameters.end(), [&](const std::pair<std::wstring, NX::json_value>& item) {
                sp->_parameters[NX::conversion::lower_str(item.first)] = json_to_generic(item.second);
            });
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
    return sp;
}

std::shared_ptr<policy_bundle> policy_bundle::parse(const std::wstring& s)
{
    std::shared_ptr<policy_bundle> sp;

    try {

        NX::json_value v = NX::json_value::parse(s);
        sp = policy_bundle::parse(v.as_object());
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        sp.reset();
    }

    return sp;
}

std::shared_ptr<policy_bundle> policy_bundle::parse(const NX::json_object& bundle_object)
{
    std::shared_ptr<policy_bundle> sp(new policy_bundle());

    try {

		sp->_enddate = 0;
		sp->_startdate = 0;
		sp->_version = bundle_object.at(L"version").as_string();
        sp->_issuer = bundle_object.at(L"issuer").as_string();
        sp->_issue_time = bundle_object.at(L"issueTime").as_string();

        const NX::json_array& policy_array = bundle_object.at(L"policies").as_array();
        for (NX::json_array::const_iterator it = policy_array.begin(); it != policy_array.end(); ++it) {

            std::shared_ptr<policy_object> policy = parse_policy_object((*it).as_object());
            if (policy != nullptr) {
                sp->_policies.push_back(policy);
				if (policy->_rights > 0)				
					sp->_grant_rights = policy->_rights;
				
				if (policy->_enddate > 0)
					sp->_enddate = policy->_enddate;
				if (policy->_startdate > 0)
				{
					sp->_startdate = policy->_startdate;
					LOGDEBUG(NX::string_formater(L" policy_bundle::parse: _startdate: (%llu)", sp->_startdate));
				}
            }
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        sp.reset();
    }

    return sp;
}

std::wstring policy_bundle::serialize() const
{
    return std::wstring();
}

void policy_bundle::clear()
{
    _version.clear();
    _issuer.clear();
    _issue_time.clear();
    _policies.clear();
}

void policy_bundle::evaluate(const eval_data& ed, eval_result* result) 
{
    unsigned __int64 grant_rights  = 0;
    unsigned __int64 revoke_rights = 0;
    std::map<std::wstring, std::shared_ptr<obligation>> obligations;

    for (auto it = _policies.begin(); it != _policies.end(); ++it) {
        if ((*it)->evaluate(ed)) {
            // match this policy
            if ((*it)->grant()) {
                grant_rights |= (*it)->_rights;
                std::for_each((*it)->_obligations.begin(), (*it)->_obligations.end(), [&](const std::pair<std::wstring, std::shared_ptr<obligation>>& item) {
                    obligations[item.first] = item.second;
                });
            }
            else {
                revoke_rights |= (*it)->_rights;
            }

            if (revoke_rights == BUILTIN_RIGHT_ALL) {
                grant_rights = 0;
                break;
            }
        }
    }

    grant_rights &= (~revoke_rights);
	_grant_rights = grant_rights;
    if (0 != grant_rights) {
        result->add_rights(grant_rights);
        std::for_each(obligations.begin(), obligations.end(), [&](const std::pair<std::wstring, std::shared_ptr<obligation>>& item) {
            result->add_obligation(item.second);
        });
    }
}


//
//  class logic_expression
//
bool logic_expression::evaluate(const eval_data& ed) const
{
    if (_expressions.empty()) {
        return true;    // No limitation
    }

    for (auto it = _expressions.begin(); it != _expressions.end(); ++it) {

        const bool b = (*it)->evaluate(ed);
        if (logic_and()) {
            if (!b) {   // Any not match
                return false;
            }
        }
        else {
            if (b) {    // Match any
                return true;
            }
        }
    }

    return logic_and() ? true : false;
}

//
//  class condition_expression
//
bool condition_expression::evaluate(const eval_data& ed) const
{
    if (get_condition_type() != 0 && (0 == (get_condition_type() & ed.get_flags()))) {
        // this condition is ignored
        return true;
    }

    auto range = ed.get_values().equal_range(_name);
    if (range.first == ed.get_values().end()) {
        return false;
    }
    for (auto it = range.first; it != range.second; ++it) {

        bool result = false;
        if (_operator == Equal) {
            if ((*it).second == _value) {
                return true;
            }
        }
        else if (_operator == NotEqual) {
            if ((*it).second != _value) {
                return true;
            }
        }
        else if (_operator == GreaterThan) {
            if ((*it).second > _value) {
                return true;
            }
        }
        else if (_operator == GreaterThanOrEqualTo) {
            if ((*it).second >= _value) {
                return true;
            }
        }
        else if (_operator == LessThan) {
            if ((*it).second < _value) {
                return true;
            }
        }
        else if (_operator == LessThanOrEqualTo) {
            if ((*it).second <= _value) {
                return true;
            }
        }
        else {
            ;
        }
    }

    return false;
}



//
//  class policy_object
//

policy_object::policy_object() : _rights(0x0UL), _enddate(0), _startdate(0)
{
}

policy_object::~policy_object()
{
}

bool policy_object::evaluate(const eval_data& ed)
{
    if (_subject_condition != nullptr && ! _subject_condition->evaluate(ed)) {
        return false;
    }
    if (_resource_condition != nullptr && !_resource_condition->evaluate(ed)) {
        return false;
    }
    if (_environment_condition != nullptr && !_environment_condition->evaluate(ed)) {
        return false;
    }
    return true;
}


//
//  class eval_data
//

eval_data::eval_data() : _flags(EVAL_FLAG_ALL)
{
}

eval_data::eval_data(unsigned long flags) : _flags(flags)
{
}

eval_data::eval_data(const eval_data& other) : _values(other._values), _flags(other._flags)
{
}

eval_data::eval_data(eval_data&& other) : _values(std::move(other._values)), _flags(std::move(other._flags))
{
}

eval_data::~eval_data()
{
}

void eval_data::push_data(const std::wstring& name, const NX::generic_value& value)
{
    _values.insert(std::pair<std::wstring, NX::generic_value>(NX::conversion::lower_str<wchar_t>(name), value));
}

eval_data& eval_data::operator = (const eval_data& other)
{
    if (this != &other) {
        _flags = other._flags;
        _values = other._values;
    }
    return *this;
}

eval_data& eval_data::operator = (eval_data&& other)
{
    if (this != &other) {
        _flags = std::move(other._flags);
        _values = std::move(other._values);
    }
    return *this;
}


//
//  class eval_result
//

namespace {
class unique_id_gen
{
public:
    unique_id_gen() : _id(0)
    {
        ::InitializeCriticalSection(&_lock);
    }
    ~unique_id_gen()
    {
        ::DeleteCriticalSection(&_lock);
    }

    unsigned __int64 gen()
    {
        unsigned __int64 id = 0;
        ::EnterCriticalSection(&_lock);
        id = _id;
        _id = (0xFFFFFFFFFFFFFFFF == id) ? 0 : (id + 1);
        ::LeaveCriticalSection(&_lock);
        return id;
    }

private:
    unsigned __int64 _id;
    CRITICAL_SECTION _lock;
};
}

static unique_id_gen IDGEN;

eval_result::eval_result() : _id(IDGEN.gen()), _rights(0)
{
}

eval_result::eval_result(unsigned __int64 rights) : _id(IDGEN.gen()), _rights(rights)
{
}

eval_result::~eval_result()
{
}

void eval_result::add_obligation(const std::shared_ptr<obligation>& sp)
{
    _obligations[sp->get_name()] = sp;
}

bool eval_result::has_obligation(const std::wstring& name) const
{
    return (_obligations.end() != _obligations.find(L"WATERMARK"));
}

std::wstring eval_result::get_rights_string() const
{
    std::wstring s;

    if (!flags64_on(_rights, BUILTIN_RIGHT_VIEW)) {
        return s;
    }
    s = RIGHT_DISP_VIEW;

    if (flags64_on(_rights, BUILTIN_RIGHT_EDIT)) {
        s += L",";
        s += RIGHT_DISP_EDIT;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_PRINT)) {
        s += L",";
        s += RIGHT_DISP_PRINT;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_CLIPBOARD)) {
        s += L",";
        s += RIGHT_DISP_CLIPBOARD;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_SAVEAS)) {
        s += L",";
        s += RIGHT_DISP_SAVEAS;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_DECRYPT)) {
        s += L",";
        s += RIGHT_DISP_DECRYPT;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_SCREENCAP)) {
        s += L",";
        s += RIGHT_DISP_SCREENCAP;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_SEND)) {
        s += L",";
        s += RIGHT_DISP_SEND;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_CLASSIFY)) {
        s += L",";
        s += RIGHT_DISP_CLASSIFY;
    }

    if (flags64_on(_rights, BUILTIN_RIGHT_SHARE)) {
        s += L",";
        s += RIGHT_DISP_SHARE;
    }

	if (flags64_on(_rights, BUILTIN_RIGHT_DOWNLOAD)) {
		s += L",";
		s += RIGHT_DISP_DOWNLOAD;
	}
    return std::move(s);
}