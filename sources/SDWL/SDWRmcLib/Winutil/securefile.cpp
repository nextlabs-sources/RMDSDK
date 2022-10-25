#include "..\stdafx.h"

#include "nudf/ntapi.hpp"
#include "..\common\macros.h"
#include "..\crypt\aes.h"
#include "..\crypt\md5.h"
#include "securefile.h"

using namespace NX;



static const char MagicCode[8] = { 'n', 'X', 'r', 'M', 'S', 'e', 'c', 'F' };


static_assert(32 == sizeof(SECURE_FILE_HEADER), "Header size should be 32 bytes");

RmSecureFile::RmSecureFile()
{
}

RmSecureFile::RmSecureFile(const std::wstring& file, const std::vector<unsigned char>& key) : _key(key)
{
	if (file.size() >= 3 && file[1] == L':' && file[2] == L'\\')
		_file = L"\\??\\" + file;
	else
		_file = file;
}

RmSecureFile::~RmSecureFile()
{
}

SDWLResult RmSecureFile::Read(std::vector<unsigned char>& content)
{
	HANDLE h = NT::CreateFile(_file.c_str(), GENERIC_READ, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, 0, NULL);
	if (NULL == h)
		return RESULT(GetLastError());

	SDWLResult res = RESULT(0);

	do {

		SECURE_FILE_HEADER header = { 0 };
		ULONG bytesRead = 0;

		LARGE_INTEGER fileSize;
		if (!NT::GetFileSize(h, &fileSize)) {
			res = RESULT(GetLastError());
			break;
		}
		if(fileSize.QuadPart < sizeof(header)) {
			res = RESULT(ERROR_INVALID_DATA);
			break;
		}
		const ULONG cipherSize = (ULONG) (fileSize.QuadPart - sizeof(header));
		if (!IS_ALIGNED(cipherSize, 16)) {
			res = RESULT(ERROR_INVALID_DATA);
			break;
		}

		if (!NT::ReadFile(h, &header, sizeof(header), &bytesRead, NULL)) {
			res = RESULT(GetLastError());
			break;
		}

		if (0 != memcmp(header.MagicCode, MagicCode, sizeof(MagicCode)) || header.ContentSize > cipherSize) {
			res = RESULT(ERROR_INVALID_DATA);
			break;
		}

		NX::crypt::AesKey key;
		res = key.Import(_key.data(), (ULONG)_key.size());
		if (!res)
		{
			NT::CloseHandle(h); h = NULL;
			return res;
		}

		unsigned long outSize = cipherSize;
		content.resize(outSize, 0);
		if (!NT::ReadFile(h, content.data(), (ULONG)content.size(), &bytesRead, NULL)) {
			res = RESULT(GetLastError());
			break;
		}
		if(bytesRead < outSize) {
			res = RESULT(GetLastError());
			if (res) res = RESULT(ERROR_INVALID_DATA);
			break;
		}

		res = NX::crypt::AesBlockDecrypt(key, content.data(), cipherSize, content.data(), &outSize, NULL, 0, 512);
		if (!res)
		{
			NT::CloseHandle(h); h = NULL;
			return res;
		}

		if (header.ContentSize < cipherSize) {
			content.resize(header.ContentSize);
		}

		// Check hash
		UCHAR hash[16] = {0};
		res = CalculateHash(content.data(), header.ContentSize, hash);
		if (!res)
			break;

		if(0 != memcmp(hash, header.Hash, 16)) {
			res = RESULT(ERROR_INVALID_DATA);
			break;
		}

	} while (FALSE);

	NT::CloseHandle(h); h = NULL;

	if (!res)
		content.clear();

	return res ? RESULT(0) : res;
}

SDWLResult RmSecureFile::Read(std::string& content)
{
	std::vector<unsigned char> buf;
	SDWLResult res = Read(buf);
	if (res) {
		content = std::string(buf.begin(), buf.end());
	}
	return res;
}

SDWLResult RmSecureFile::Write(const std::vector<unsigned char>& content)
{
	const unsigned int contentSize = (unsigned int)content.size();
	if (0 == contentSize)
		return RESULT(ERROR_INVALID_PARAMETER);
	return Write(content.data(), contentSize);
}

SDWLResult RmSecureFile::Write(const unsigned char* content, unsigned int contentSize)
{
	SECURE_FILE_HEADER header = { 0 };

	if (content == NULL || 0 == contentSize)
		return RESULT(ERROR_INVALID_PARAMETER);

	SDWLResult res = PrepareHeader(content, contentSize, &header);
	if (!res)
		return res;

	NX::crypt::AesKey key;
	res = key.Import(_key.data(), (ULONG)_key.size());
	if (!res)
		return res;

	const unsigned long bufSize = ROUND_TO_SIZE(contentSize, 16);
	unsigned long outSize = bufSize;
	std::vector<unsigned char> buf;
	buf.resize(bufSize, 0);
	memcpy(buf.data(), content, contentSize);
	res = NX::crypt::AesBlockEncrypt(key, buf.data(), bufSize, buf.data(), &outSize, NULL, 0, 512);
	if (!res)
		return res;

	HANDLE h = NT::CreateFileW(_file.c_str(), GENERIC_READ | GENERIC_WRITE, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OVERWRITE_IF, 0, NULL);
	if (NULL == h)
		return RESULT(GetLastError());

	ULONG bytesWritten = 0;
	if (!NT::WriteFile(h, &header, (ULONG)sizeof(header), &bytesWritten, NULL)) {
		NT::CloseHandle(h); h = NULL;
		return RESULT(GetLastError());
	}
	if (!NT::WriteFile(h, buf.data(), (ULONG)buf.size(), &bytesWritten, NULL)) {
		NT::CloseHandle(h); h = NULL;
		return RESULT(GetLastError());
	}
	
	NT::CloseHandle(h); h = NULL;
	return RESULT(0);
}

SDWLResult RmSecureFile::CalculateHash(const unsigned char* content, unsigned int contentSize, unsigned char* hash)
{
	return NX::crypt::CreateHmacMd5(content, contentSize, (unsigned char*)(&contentSize), (unsigned long)sizeof(unsigned int), hash);
}

SDWLResult RmSecureFile::PrepareHeader(const unsigned char* content, unsigned int contentSize, PSECURE_FILE_HEADER header)
{
	memset(header, 0, sizeof(SECURE_FILE_HEADER));
	memcpy(header->MagicCode, MagicCode, 8);
	header->ContentSize = contentSize;
	return CalculateHash(content, contentSize, header->Hash);
}