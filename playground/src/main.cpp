#include "playground.hpp"

int main()
{
	DEBUG_LINE();
    spk::DataBuffer buffer(32);

	DEBUG_LINE();
    spk::DataBufferLayout layout(&buffer);

	DEBUG_LINE();
    layout.addElement(L"intField", 0, sizeof(int));

	DEBUG_LINE();
    layout.addElement(L"floatField", 4, sizeof(float));

	DEBUG_LINE();
    layout.addElement(L"charField", 8, sizeof(char));

	DEBUG_LINE();
    layout.addElement(L"arrayField", 12, 3, sizeof(int), 0);

	DEBUG_LINE();
    layout[L"intField"] = 42;
    layout[L"floatField"] = 3.14159f;
    layout[L"charField"] = 'A';

	DEBUG_LINE();
    layout[L"arrayField"][0] = 0xFFFFFFFF;
    layout[L"arrayField"][1] = 0xFFFFFFFF;
    layout[L"arrayField"][2] = 0xFFFFFFFF;

	DEBUG_LINE();
    int    readInt    = layout[L"intField"].get<int>();
    float  readFloat  = layout[L"floatField"].get<float>();
    char   readChar   = layout[L"charField"].get<char>();
    int  arrVal0    = layout[L"arrayField"][0].get<int>();
    int  arrVal1    = layout[L"arrayField"][1].get<int>();
    int  arrVal2    = layout[L"arrayField"][2].get<int>();

	DEBUG_LINE();
    std::cout << "===== DataBufferLayout Test =====" << std::endl;
    std::cout << "intField   = " << readInt   << std::endl;
    std::cout << "floatField = " << readFloat << std::endl;
    std::cout << "charField  = " << readChar  << std::endl;
    std::cout << "arrayField[0] = " << arrVal0 << std::endl;
    std::cout << "arrayField[1] = " << arrVal1 << std::endl;
    std::cout << "arrayField[2] = " << arrVal2 << std::endl;

	DEBUG_LINE();
    std::cout << "\nRaw buffer (hex): ";
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(buffer.data()[i]) << " ";
    }
    std::cout << std::dec << std::endl;

	DEBUG_LINE();
    return 0;
}