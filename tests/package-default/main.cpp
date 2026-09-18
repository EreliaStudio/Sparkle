#include <exception.hpp>

int main()
{
	const spk::Exception error("Installed Sparkle consumer");
	return error.message() == "Installed Sparkle consumer" ? 0 : 1;
}
