#pragma once

#include <iostream>
#include <mutex>
#include <sstream>

namespace spk
{
	class IOStream
	{
	private:
		std::wstring _prefix;
		std::wstringstream _buffer;
		std::wostream *_outputStream = nullptr;
		static inline std::recursive_mutex _mutex;

		void _flushBuffer();

	public:
		IOStream();
		IOStream(std::wostream &p_outputStream);
		void setPrefix(const std::wstring &p_prefix);

		void redirect(std::wostream &p_outputStream);

		void flush();

		template <typename T>
		IOStream &operator<<(const T &p_value)
		{
			_buffer << p_value;
			return *this;
		}

		using Manipulator = std::wostream &(std::wostream &);

		IOStream &operator<<(Manipulator p_manip);

		std::wstring str() const;
	};

	extern IOStream cout;
	extern IOStream cerr;
}
