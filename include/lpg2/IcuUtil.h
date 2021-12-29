#ifndef FILE_LOADER_H__
#define FILE_LOADER_H__

#include <string>
#include <utility>
#include <vector>

using std::string;
using std::pair;

namespace IcuUtil
{
	
	 std::string ws2s(std::wstring const& wstr);
	 std::wstring s2ws(std::string const& str);

// ̽���ļ��ı���
// @return : <��������, ���Ŷ�>
pair<string, int> detectFileEncoding(const wchar_t*);

// ̽��һ���ı��ı���
pair<string, int> detectTextEncoding(const char *data, int len);

// ��ȡϵͳĬ�ϵı�������
const string &getDefaultEncoding();

/* 
 * toConverterName,      ת������ַ����� 
 * fromConverterName,    ת��ǰ���ַ����� 
 * target,               �洢ת������ַ����� �������� 
 * targetCapacity,       �洢������target�Ĵ�С 
 * source��              ��Ҫת�����ַ��� 
 * sourceLength,         source�Ĵ�С 
**/  
int convert(const char *toConverterName,
	        const char *fromConverterName,  
            char *target,
			signed int targetCapacity, 
			const char *source, 
			signed int sourceLength,
			int& outlen
			) ;
// 
const string getCheckedEncoding(const pair<string, int> &encoding);

bool getFileUnicodeContent(const wchar_t* fileName, std::wstring& content);
bool getFileRawContent(const wchar_t* fileName, std::string& content);
}
#endif