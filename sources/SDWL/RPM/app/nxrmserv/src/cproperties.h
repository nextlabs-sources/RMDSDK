#pragma once
#ifndef CProperties_H
#define CProperties_H

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <windows.h>
using namespace std;

class CProperties {
private:
	string path;
	vector<string> vLine;
	multimap<string, string> msKV;
	bool mulremark;
public:
	CProperties();
	virtual ~CProperties();
	bool open(const char* path);
	void close();
	vector<string> read(const char *k);
	bool write(const char *k, const char* v);
	bool modify(const char *k, const char* v);
	void trim_first(string &s);
	void trim_end(string &s);
	void trim(string &s);
};

#endif