#pragma once

#include <iostream>
#include <sstream>
#include <mutex>

namespace spk
{
	class IOStream
	{
	private:
		std::wstring _prefix;
		std::wstringstream _buffer;
		std::wostream* _outputStream = nullptr;
		static inline std::recursive_mutex _mutex;

		void _flushBuffer();

	public:
		IOStream();
		IOStream(std::wostream& p_outputStream);
		void setPrefix(const std::wstring& p_prefix);

		void redirect(std::wostream& p_outputStream);

		void flush();

		template <typename T>
		IOStream& operator<<(const T& value)
		{
			_buffer << value;
			return *this;
		}

		using Manipulator = std::wostream& (std::wostream&);

		IOStream& operator<<(Manipulator manip);

		std::wstring str() const;
	};

	extern IOStream cout;
	extern IOStream cerr;
}
