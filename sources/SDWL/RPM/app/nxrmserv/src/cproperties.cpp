#include "cproperties.h"
//***************************************************************************************************
//***************************************************************************************************
//*This tool class implements C++ read and write operations on properties files,
//*1. Support single-line comments, such as: remove the space at the beginning of the line and start with //, #, <!--Content-->
//* //This is a single line comment
//* #This is a single line comment
//* <!--This is a single line comment-->
//*2.Support nodes, but nodes do not make sense [section]
//*[section0]
//*key=value
//*[section1]
//*key=value
//*Multimap is used, so there are two values for the key key, and the node does not play a role in distinguishing. This is also in line with the requirements and specifications of the properties configuration file.
//*3.Support multi-line comments /*The first line, the second line...*/
//*Case (1) The peer has /*..*/ such as: key=123/*oooooo*/b=890 The result is reserved key=123
//*Case (2) There are multiple lines /*..*/ such as: key=123/*ooo
//*0000000000000000000
//*ooo*/b=890  The result remains key=123
//*Case (3) There is /*..*/ at the beginning and end of the line, such as: /*ooooo*/key=123 The result is not retained
//*
//*/
/************************************************************************************************/

CProperties::CProperties() { mulremark = false; };
CProperties::~CProperties() {};

bool CProperties::open(const char* path) {
	if (nullptr == path)
	{
		return false;
	}
	this->path = path;
	ifstream ifs;
	ifs.open(path, ios::in);
	if (!ifs)
	{
		return false;
	}
	string sLine;
	while (!ifs.eof()) {
		sLine = "";
		getline(ifs, sLine);
		if (mulremark)//The multi-line comment switch has been turned on. It is necessary to determine whether the line has a close switch
		{
			if (sLine.find("*/") != string::npos) {
				mulremark = false;
			}
			continue;
		}
		else
		{
			string::size_type pos = sLine.find("/*"); string sSubLine;
			if (pos != string::npos)//This line has a multi-line comment switch that needs to be turned on
			{
				string::size_type epos = sLine.rfind("*/");
				mulremark = epos == string::npos || epos < pos ? true : false;
				sSubLine = sLine.substr(0, pos);
			}
			else {
				sSubLine = sLine;
			}
			trim(sSubLine);
			if (sSubLine.length() <= 0) continue;
			if (sSubLine[0] == '#')continue;
			if (sSubLine[0] == '[')continue;
			if (sSubLine.length() > 2 && sSubLine[0] == '/' && sSubLine[1] == '/')continue;
			if (sSubLine.length() > 4 && sSubLine[0] == '<' && sSubLine[1] == '!')continue;
			vLine.push_back(sSubLine);
		}
	}
	if (ifs.is_open())
	{
		ifs.close();
	}

	string key, value; 
	string sSubStr;
	for (int i = 0; i < vLine.size(); i++)
	{
		string::size_type pos = vLine.at(i).find_first_of("=");
		if (pos == string::npos)
		{
			continue;
		}
		key = vLine.at(i).substr(0, pos);
		value = vLine.at(i).substr(pos + 1, vLine.at(i).size() - pos);
		msKV.insert(make_pair(key, value));
	}

	return true;
}

vector<string> CProperties::read(const char *k) {
	vector<string> vVauleCol;
	if (msKV.size() > 0)
	{
		multimap<string, string>::size_type  cnt = msKV.count(k);
		multimap<string, string>::iterator  iter = msKV.find(k);
		for (; cnt > 0; cnt--, iter++)
		{
			vVauleCol.push_back(iter->second);
		}
	}
	return vVauleCol;
}

/*
*Append key=value to the end of the text, update the multimap mapping insert
*/
bool CProperties::write(const char *k, const char* v) {
	if (nullptr == k || nullptr == v)
	{
		return false;
	}
	ofstream ofs;
	ofs.open(this->path.c_str(), ios::app);
	if (!ofs)
	{
		return false;
	}
	char sStr[1024] = {};
	sprintf(sStr, "%s=%s", k, v);
	ofs << endl << sStr;
	msKV.insert(make_pair(k, v));
	if (ofs.is_open())
	{
		ofs.close();
	}
	return true;
}

bool CProperties::modify(const char *k, const char* v) {
	mulremark = false;
	bool notexit = true;
	ifstream ifs;
	ifs.open(this->path, ios::in);
	string temp = "";
	if (!ifs)
	{
		return false;
	}
	string sLine;
	char sStr[1024] = {};
	sprintf_s(sStr, 1024, "%s=", k);
	char sModify[1024] = {};
	sprintf_s(sModify, 1024, "%s=%s", k, v);
	while (!ifs.eof()) {
		sLine = "";
		getline(ifs, sLine);
		if (mulremark)
		{
			temp.append(sLine.c_str());
			temp.append("\n");
			if (sLine.find("*/") != string::npos) {
				mulremark = false;
			}
			continue;
		}
		else
		{
			string::size_type pos = sLine.find("/*"); 
			string sSubLine;
			if (pos != string::npos)
			{
				string::size_type epos = sLine.rfind("*/");
				mulremark = epos == string::npos || epos < pos ? true : false;
				sSubLine = sLine.substr(0, pos);
			}
			else {
				sSubLine = sLine;
			}
			trim(sSubLine);
			if (sSubLine.length() <= 0) { temp.append(sLine.c_str()); temp.append("\n"); continue; }
			if (sSubLine[0] == '#') { temp.append(sLine.c_str()); temp.append("\n"); continue; }
			if (sSubLine[0] == '[') { temp.append(sLine.c_str()); temp.append("\n"); continue; }
			if (sSubLine.length() > 2 && sSubLine[0] == '/' && sSubLine[1] == '/') { temp.append(sLine.c_str()); temp.append("\n"); continue; }
			if (sSubLine.length() > 4 && sSubLine[0] == '<' && sSubLine[1] == '!') { temp.append(sLine.c_str()); temp.append("\n"); continue; }

			if (sSubLine.find(sStr) == string::npos) { temp.append(sLine.c_str()); temp.append("\n"); continue; }

			{
				notexit = false;
				temp.append(sModify);
				if (pos != string::npos)
				{
					temp.append(sLine.substr(pos, sLine.length()));
				}
				temp.append("\n");
				continue;
			}
		}
	}
	if (notexit)
	{
		temp.append(sModify);
	}
	if (ifs.is_open())
	{
		ifs.close();
	}
	ofstream ofs;
	ofs.open(this->path.c_str(), ios::trunc);
	if (!ofs)
	{
		return false;
	}
	ofs << temp.c_str();
	if (ofs.is_open())
	{
		ofs.close();
	}
	msKV.erase(k);
	msKV.insert(make_pair(k, v));
	return true;
}

void CProperties::close() {
	vLine.erase(vLine.begin(), vLine.end());
	msKV.erase(msKV.begin(), msKV.end());
}

void CProperties::trim_first(string &s) {
	if (!s.empty())
		s.erase(0, s.find_first_not_of(" "));
}
void CProperties::trim_end(string &s) {
	if (!s.empty())
		s.erase(s.find_last_not_of(" ") + 1);
}
void CProperties::trim(string &s)
{
	int index = 0;
	if (!s.empty())
	{
		while ((index = s.find(' ', index)) != string::npos)
		{
			s.erase(index, 1);
		}
	}
}