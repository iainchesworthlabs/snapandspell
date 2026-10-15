#pragma once
#ifndef _CULTUREINFO_H
#define _CULTUREINFO_H

#include <array>
#include <string>

class CultureInfo
{
public:
	enum Countries
	{
		UnknownCountry,
		CountryCount
	};

	enum Languages
	{
		Afrikaans,
		Arabic,
		Bulgarian,
		Catalan,
		Chinese,
		Croatian,
		Czech,
		English,
		Portuguese,
		Spanish,
		UnknownLanguage,
		LanguageCount
	};

public:
	CultureInfo();
	~CultureInfo();

public:
	Countries CurrentCountry();
	Languages CurrentLanguage();
	std::wstring GetISO639_ShortCode(Languages language);
	std::wstring GetISO3166_ShortCode(Countries country);

private:
	typedef const std::array<const std::wstring, CountryCount> CountryArray;
	typedef const std::array<const std::wstring, LanguageCount> LanguageArray;
	static CountryArray const& getCountryArray();
	static LanguageArray const& getLanguageArray();

private:
	Countries DecodeISO639_CountryCode(WCHAR pszISOCountry[]);
	Languages DecodeISO639_LanguageCode(WCHAR pszISOLanguage[]);
};

#endif // _CULTUREINFO_H
