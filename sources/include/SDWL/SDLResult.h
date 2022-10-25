/*!
 * \file SDLResult.h
 *
 * \author hbwang
 * \date October 2017
 *
 * 
 */

#pragma once

#include "SDLError.h"

#include <string>

/*
SDWLResult: This class shows the function return status - success or failed with error code and message.
*/

class SDWLResult
{
public:
	SDWLResult() : _code(0), _line(0), _file(nullptr), _func(nullptr)
	{
	}
	explicit SDWLResult(int code, int line, const char* file, const char* func, const std::string& msg = std::string()) : _code(code), _line(line), _file(file), _func(func), _msg(msg)
	{
	}
	SDWLResult(const SDWLResult& rhs) : _code(rhs._code), _line(rhs._line), _file(rhs._file), _func(rhs._func), _msg(rhs._msg)
	{
	}
	virtual ~SDWLResult()
	{
	}

	inline int GetCode() const { return _code; }
	inline int GetLine() const { return _line; }
	inline const char* GetFile() const { return _file ? _file : ""; }
	inline const char* GetFunction() const { return _func ? _func : ""; }
	inline const std::string& GetMsg() const { return _msg; }

	inline operator bool() const { return (0 == _code); }
	inline bool operator == (int code) const { return (code == _code); }
	inline bool operator != (int code) const { return (code != _code); }

	virtual void Clear()
	{
		_code = 0;
		_line = 0;
		_file = nullptr;
		_func = nullptr;
		_msg.clear();
	}

	SDWLResult& operator = (const SDWLResult& rhs)
	{
		if (this != &rhs)
		{
			_code = rhs._code;
			_line = rhs._line;
			_file = rhs._file;
			_func = rhs._func;
			_msg = rhs._msg;
		}
		return *this;
	}

	std::wstring ToString(void) const {
        const std::wstring msgW(_msg.begin(), _msg.end());
		return std::wstring(L"{") + std::to_wstring(_code) + L", \"" + msgW + L"\"}";
	}

private:
	int _code;
	int _line;
	const char* _file;
	const char* _func;
	std::string _msg;
};

#define RESULT(c)       SDWLResult(c, __LINE__, __FILE__, __FUNCTION__, "")
#define RESULT2(c, m)   SDWLResult(c, __LINE__, __FILE__, __FUNCTION__, m)

