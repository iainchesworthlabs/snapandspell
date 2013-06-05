#include "Stdafx.h"
#include "StringConversions.h"

namespace WordJewelsEngine
{

/*
void StringConversions::StringToStlWString(System::String const^ s, std::wstring& os)
{
	System::String^ string = const_cast<System::String^>(s);
	const wchar_t* chars = reinterpret_cast<const wchar_t*>((System::Runtime::InteropServices::Marshal::StringToHGlobalUni(string)).ToPointer());
	os = chars;
	System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr((void*)chars));
}

System::String^ StringConversions::StlWStringToString(std::wstring const& os) 
{
	System::String^ str = gcnew System::String(os.c_str());
	return str;
}

System::String^ StringConversions::WPtrToString(wchar_t const* pData, int length) 
{
	if (0 == length) 
	{
		length = wcslen(pData);
		if (0 == length) 
		{
			System::String^ ret = "";
			return ret;
		}
	}

	System::IntPtr bfr = System::IntPtr(const_cast<wchar_t*>(pData));
	System::String^ ret = System::Runtime::InteropServices::Marshal::PtrToStringUni(bfr, length);
	return ret;
}
*/

};
// namespace WordJewelsEngine
