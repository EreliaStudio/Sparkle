#include "structure/spk_iostream.hpp"

namespace spk
{
	spk::IOStream cout(std::wcout);
	spk::IOStream cerr(std::wcerr);

	void IOStream::_flushBuffer()
	{
		if (_outputStream == nullptr)
		{
			_outputStream = &std::wcout;
		}
		auto bufferContent = _buffer.str();

		if (bufferContent.empty())
		{
			return;
		}

		{
			std::lock_guard<std::recursive_mutex> lock(_mutex());
			if (!_prefix.empty())
			{
				*_outputStream << "[" << _prefix << "] - ";
			}
			*_outputStream << bufferContent;
		}
		_buffer.str(L"");
		_buffer.clear();
	}

	IOStream::IOStream() :
		_prefix(L""),
		_buffer(),
		_outputStream(nullptr)
	{
	}

	IOStream::IOStream(std::wostream &p_outputStream) :
		_prefix(L""),
		_buffer(),
		_outputStream(&p_outputStream)
	{
	}

	void IOStream::setPrefix(const std::wstring &p_prefix)
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex());
		_prefix = p_prefix;
	}

	void IOStream::redirect(std::wostream &p_outputStream)
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex());
		_outputStream = &p_outputStream;
	}

	void IOStream::flush()
	{
		_flushBuffer();
	}

	IOStream &IOStream::operator<<(Manipulator p_manip)
	{
		_buffer << p_manip;
		if (p_manip == static_cast<Manipulator &>(std::endl))
		{
			_flushBuffer();
		}
		return *this;
	}

	std::wstring IOStream::str() const
	{
		return (_buffer.str());
	}
}