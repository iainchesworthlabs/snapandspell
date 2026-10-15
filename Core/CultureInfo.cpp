#include "pch.h"

#include "CultureInfo.h"
#include "DebugStream.h"
#include "TracedThrow.h"

#include <cstdint>

CultureInfo::CultureInfo()
{
}

CultureInfo::~CultureInfo()
{
}

CultureInfo::Countries CultureInfo::CurrentCountry()
{
	static const uint32_t ISO3166_LENGTH = 5;
	WCHAR szISOCountry[ISO3166_LENGTH] = { 0 };
	uint32_t iLen = 0;

	if (0 == ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, szISOCountry, sizeof(szISOCountry) / sizeof(WCHAR)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to get ISO-3166 country name") << std::endl;
		Diagnostics::GetErrorString(::GetLastError());
		wdebugstream() << std::endl;
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Retrieved the country code: ") << szISOCountry << std::endl;
	}

	return DecodeISO639_CountryCode(szISOCountry);
}

CultureInfo::Languages CultureInfo::CurrentLanguage()
{
	static const uint32_t ISO639_LENGTH = 5;
	WCHAR szISOLang[ISO639_LENGTH] = { 0 };
	uint32_t iLen = 0;

	if (0 == (::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, szISOLang, sizeof(szISOLang) / sizeof(WCHAR))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to get ISO-639 language name") << std::endl;
		Diagnostics::GetErrorString(::GetLastError());
		wdebugstream() << std::endl;
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Retrieved the language code: ") << szISOLang << std::endl;
	}

	return DecodeISO639_LanguageCode(szISOLang);
}

CultureInfo::Languages CultureInfo::DecodeISO639_LanguageCode(WCHAR pszISOLanguage[])
{
	CultureInfo::Languages decodedLanguage(CultureInfo::UnknownLanguage);

	if (nullptr == pszISOLanguage)
	{
		DiagnosticsTracedThrow(std::logic_error("DEBUG: Passed null pointer when decoding ISO-639 language code"));
	}
	else
	{
		static const uint32_t MAX_LANGUAGE_CODE_LENGTH = 2;
		static const uint32_t MIN_LANGUAGE_CODE_LENGTH = 2;
		const std::wstring sLanguageCode(pszISOLanguage);

		if ((MIN_LANGUAGE_CODE_LENGTH > sLanguageCode.length()) || (MAX_LANGUAGE_CODE_LENGTH < sLanguageCode.length()))
		{
			DiagnosticsTracedThrow(std::logic_error("DEBUG: Passed invalid language code for decoding"));
		}
		else if (0 == CultureInfo::getLanguageArray()[Afrikaans].compare(sLanguageCode))  decodedLanguage = CultureInfo::Afrikaans;
		else if (0 == CultureInfo::getLanguageArray()[Arabic].compare(sLanguageCode))	  decodedLanguage = CultureInfo::Arabic;
		else if (0 == CultureInfo::getLanguageArray()[Bulgarian].compare(sLanguageCode))  decodedLanguage = CultureInfo::Bulgarian;
		else if (0 == CultureInfo::getLanguageArray()[Catalan].compare(sLanguageCode))	  decodedLanguage = CultureInfo::Catalan;
		else if (0 == CultureInfo::getLanguageArray()[Chinese].compare(sLanguageCode))	  decodedLanguage = CultureInfo::Chinese;
		else if (0 == CultureInfo::getLanguageArray()[Croatian].compare(sLanguageCode))	  decodedLanguage = CultureInfo::Croatian;
		else if (0 == CultureInfo::getLanguageArray()[Czech].compare(sLanguageCode))	  decodedLanguage = CultureInfo::Czech;
		else if (0 == CultureInfo::getLanguageArray()[English].compare(sLanguageCode))	  decodedLanguage = CultureInfo::English;
		else if (0 == CultureInfo::getLanguageArray()[Portuguese].compare(sLanguageCode)) decodedLanguage = CultureInfo::Portuguese;
		else if (0 == CultureInfo::getLanguageArray()[Spanish].compare(sLanguageCode))	  decodedLanguage = CultureInfo::Spanish;
		else
		{
			wdebugstream() << TEXT("DEBUG: Failed to decode ISO-639 language name: ") << sLanguageCode << std::endl;
		}
	}

	return decodedLanguage;
}

CultureInfo::Countries CultureInfo::DecodeISO639_CountryCode(WCHAR pszISOCountry[])
{
	return CultureInfo::UnknownCountry;
}

std::wstring CultureInfo::GetISO639_ShortCode(CultureInfo::Languages language)
{
	return getLanguageArray()[language];
}

std::wstring CultureInfo::GetISO3166_ShortCode(CultureInfo::Countries country)
{
	return getCountryArray()[country];
}

CultureInfo::CountryArray const& CultureInfo::getCountryArray()
{
	static CountryArray COUNTRY_CODES
	{
		{
			std::wstring(TEXT("??"))  // Unknown 
		}
	};

	return COUNTRY_CODES;
}

CultureInfo::LanguageArray const& CultureInfo::getLanguageArray()
{
	static LanguageArray LANGUAGE_CODES
	{
		{
			TEXT("af"), // Afrikaans
			TEXT("ar"), // Arabic
			TEXT("bg"), // Bulgarian
			TEXT("ca"), // Catalan
			TEXT("zh"), // Chinese
			TEXT("hr"), // Croatian
			TEXT("cs"), // Czech
			TEXT("en"), // English
			TEXT("pt"), // Portuguese
			TEXT("es"), // Spanish
			TEXT("??")  // Unknown
		}
	};

	return LANGUAGE_CODES;
}
