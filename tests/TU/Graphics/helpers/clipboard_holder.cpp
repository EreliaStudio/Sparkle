#include <Windows.h>

// A separate process and real HWND establish native clipboard exclusion.
// It never empties or writes the clipboard; the parent owns content restoration.
int main(int argc, char **argv)
{
	if (argc != 3)
		return 1;
	HANDLE acquired = ::OpenEventA(EVENT_MODIFY_STATE, FALSE, argv[1]);
	HANDLE release = ::OpenEventA(SYNCHRONIZE, FALSE, argv[2]);
	if (acquired == nullptr || release == nullptr)
	{
		if (acquired != nullptr) ::CloseHandle(acquired);
		if (release != nullptr) ::CloseHandle(release);
		return 1;
	}
	HWND window = ::CreateWindowExW(0, L"STATIC", L"Sparkle clipboard holder", 0,
		0, 0, 0, 0, HWND_MESSAGE, nullptr, ::GetModuleHandleW(nullptr), nullptr);
	bool opened = false;
	for (int attempt = 0; window != nullptr && attempt < 100; ++attempt)
	{
		if (::OpenClipboard(window))
		{
			opened = true;
			break;
		}
		::Sleep(5);
	}
	int result = 2;
	if (opened)
	{
		::SetEvent(acquired);
		result = ::WaitForSingleObject(release, 10000) == WAIT_OBJECT_0 ? 0 : 3;
		::CloseClipboard();
	}
	if (window != nullptr) ::DestroyWindow(window);
	::CloseHandle(release);
	::CloseHandle(acquired);
	return result;
}
