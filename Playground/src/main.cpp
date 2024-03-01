#include "sparkle.hpp"

#include <fstream>

int main()
{
    std::fstream output;

    for (int chunkX = -1; chunkX <= 1; chunkX++)
    {
        for (int chunkY = -1; chunkY <= 1; chunkY++)
        {
            output.open("chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkY) + ".chk", std::ios_base::out | std::ios_base::binary);

            for (int i = 0; i < 16; i++)
            {
                for (int j = 0; j < 16; j++)
                {
                    for (int h = 0; h < 5; h++)
                    {
                        short value = (h == 0) ? 0 : -1;
                        output.write(reinterpret_cast<const char*>(&value), sizeof(short));
                    }
                }
            }

            output.close();
        }    
    }

    return (0);
}