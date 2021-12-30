#include "lpg2/IcuUtil.h"


#include <cassert>
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>

#include <cstdio>
#include <utility>
#include <string>
#include <vector>
#include <fstream>
#include "utf8.h"
using namespace  std;
namespace IcuUtil
{

	pair<string, int> detectTextEncoding(const char* data, int len)
	{
		string matchMost;
		int confidence = 0;
		int matchCount = 0;
		const UCharsetMatch** csm;
		UErrorCode status = U_ZERO_ERROR;
		UCharsetDetector* csd = ucsdet_open(&status);
		if (status != U_ZERO_ERROR)
			return pair<string, int>("", 0);
		
		do {

			ucsdet_setText(csd, data, len, &status);
			if (U_FAILURE(status))
				break;


			csm = ucsdet_detectAll(csd, &matchCount, &status);
			if (U_FAILURE(status))
				break;

			if (matchCount <= 0)
				break;

			confidence = ucsdet_getConfidence(csm[0], &status);
			if (U_FAILURE(status))
				break;

			matchMost = ucsdet_getName(csm[0], &status);
			if (U_FAILURE(status))
				break;
			ucsdet_close(csd);
			return pair<string, int>(matchMost, confidence);
			
	} while (false);
	ucsdet_close(csd);
	return pair<string, int>({}, 0);
	}

    std::string ws2s(std::wstring const& wstr) {
        if(sizeof(wchar_t) == 2){
            std::string narrow;
            utf8::utf16to8(wstr.begin(), wstr.end(), std::back_inserter(narrow));
            return narrow;
        }else{
            std::string narrow;
            utf8::utf32to8(wstr.begin(), wstr.end(), std::back_inserter(narrow));
            return narrow;
        }

    }
    std::wstring s2ws(const std::string& str) {
        std::wstring wide;
        if(sizeof(wchar_t) == 2){
            utf8::utf8to16(str.begin(), str.end(), std::back_inserter(wide));
            return wide;
        }else{
            utf8::utf8to32(str.begin(), str.end(), std::back_inserter(wide));
            return wide;
        }
    }
    bool getFileRawContent(const wchar_t* fileName, std::string& content)
    {

#ifdef _MSC_VER
        std::ifstream stream(fileName, std::ios::binary);
#else
        std::ifstream stream(fileName, std::ios::binary);

#endif
        if (!stream.good() || stream.eof()) // No fail, bad or EOF.
            return false;
        std::string s((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        content.swap(s);

        return true;
    }

    std::pair<std::string, int> detectFileEncoding(const wchar_t* fileName)
	{
		pair<string, int> encoding;
        std::string content;
        if(getFileRawContent(fileName,content)){
            encoding = detectTextEncoding(content.data(), content.size());
            return encoding;
        }else{
            return {"",0};
        }

	}
    bool getFileUnicodeContent(const wchar_t* fileName, std::wstring& context)
    {
        std::string holder;
        if(!getFileRawContent(fileName,holder)){
            return false;
        }
        std::u16string out_temp;
        const pair<string, int> detectResult = detectTextEncoding(&holder[0], holder.size());
        std::string strEncoding;
        if (detectResult.second > 70)
            strEncoding = detectResult.first;
        else
        {
            strEncoding = getDefaultEncoding();
        }
        out_temp.resize(holder.size());

        bool flush = false;
        UErrorCode errcode = U_ZERO_ERROR;
        UConverter* pFromCnv = ucnv_open(strEncoding.data(), &errcode);
        if (U_FAILURE(errcode)) return false;
        UErrorCode inerr = U_ZERO_ERROR;


        const char* in_source = &holder[0];
        UChar* tmp_target = out_temp.data();
        const auto tmp_buf = tmp_target;

        ucnv_toUnicode(pFromCnv, &tmp_target, tmp_target + out_temp.size(), &in_source, in_source + holder.size(), NULL, flush, &inerr);
        ucnv_close(pFromCnv);
        if (U_FAILURE(inerr))
            return false;
        auto count = tmp_target - tmp_buf;
        out_temp.resize(count);

        if(sizeof(wchar_t ) == 2)
            context.assign(out_temp.begin(),out_temp.end());
        else{
            std::string narrow;
            utf8::utf16to8(out_temp.begin(), out_temp.end(), std::back_inserter(narrow));
            context= s2ws(narrow);
        }
        return true;
    }


	/*
	 * toConverterName,      转换后的字符编码
	 * fromConverterName,    转换前的字符编码
	 * target,               存储转换后的字符串， 传出参数
	 * targetCapacity,       存储容量，target的大小
	 * source，              需要转换的字符串
	 * sourceLength,         source的大小
	**/
	int convert(const char* toConverterName,
		const char* fromConverterName,
		char* target,
		int32_t targetCapacity,
		const char* source,
		int32_t sourceLength,  int& outlen)
	{
		UErrorCode error = U_ZERO_ERROR;
		outlen = ucnv_convert(toConverterName, fromConverterName, target, targetCapacity, source, sourceLength, &error);

		return error;
	}
	const string& getDefaultEncoding()
	{
		static string defaultEncoding;
		if (defaultEncoding.empty()) {
			UErrorCode errCode = U_ZERO_ERROR;
			UConverter* dftCnv = ucnv_open(NULL, &errCode);
			const char* defEncodingName = ucnv_getName(dftCnv, &errCode);//ucnv_getDefaultName();
			if (defEncodingName) {
				const char* standarName = ucnv_getStandardName(defEncodingName, "IANA", &errCode);//name of the standard governing the names; MIME and IANA are such standard 
				if (standarName == NULL) {
					standarName = ucnv_getStandardName(defEncodingName, "MIME", &errCode);
				}
				if (standarName == NULL)
					defaultEncoding = defEncodingName;
				else
					defaultEncoding = standarName;
			}

			ucnv_close(dftCnv);
		}

		return defaultEncoding;
	}

	const string getCheckedEncoding(const pair<string, int>& encoding)
	{
		string encode;
		if (!encoding.first.empty() && encoding.second > 50)
			encode = encoding.first;
		else
			encode = getDefaultEncoding();

		return encode;
	}

}