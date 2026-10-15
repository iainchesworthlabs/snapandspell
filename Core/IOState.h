#pragma once
#ifndef _IOSTATE_H
#define _IOSTATE_H

#include <ios>

class ios_flags_saver
{
public:
	typedef ::std::ios_base            state_type;
	typedef ::std::ios_base::fmtflags  aspect_type;

	explicit  ios_flags_saver(state_type& s) : s_save_(s), a_save_(s.flags())
	{
	}

	ios_flags_saver(state_type& s, aspect_type const& a) : s_save_(s), a_save_(s.flags(a))
	{
	}

	~ios_flags_saver()
	{
		this->restore();
	}

	const aspect_type& value() const
	{
		return a_save_;
	}

	void  restore()
	{
		s_save_.flags(a_save_);
	}

private:
	state_type& s_save_;
	aspect_type const  a_save_;

	ios_flags_saver& operator=(const ios_flags_saver&);
};

template < typename Ch, class Tr >
class basic_ios_fill_saver
{
public:
	typedef ::std::basic_ios<Ch, Tr>        state_type;
	typedef typename state_type::char_type  aspect_type;

	explicit  basic_ios_fill_saver(state_type& s) : s_save_(s), a_save_(s.fill())
	{
	}

	basic_ios_fill_saver(state_type& s, aspect_type const& a) : s_save_(s), a_save_(s.fill(a))
	{
	}

	~basic_ios_fill_saver()
	{
		this->restore();
	}

	const aspect_type& value() const
	{
		return a_save_;
	}

	void  restore()
	{
		s_save_.fill(a_save_);
	}

private:
	state_type& s_save_;
	aspect_type const  a_save_;
};

#endif // _IOSTATE_H
