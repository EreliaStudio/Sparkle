#include "playground.hpp"

namespace spk
{
    struct CustomData
    {
        int id;
        size_t test;
        float value;

        // Serialize
        friend spk::DataBuffer& operator<<(spk::DataBuffer& db, const CustomData& cd)
        {
            db << cd.id;
            db << cd.value;
            return db;
        }

        // Deserialize
        friend const spk::DataBuffer& operator>>(const spk::DataBuffer& db, CustomData& cd)
        {
            db >> cd.id;
            db >> cd.value;
            return db;
        }
    };
}

int main()
{
    spk::Message msg;

    // Testing with an integer
    int myInt = 123;
    msg << myInt;
    std::cout << "Message size : " << msg.size() << std::endl;

    // Testing with a string
    std::string myString = "Hello, world!";
    msg << myString;
    std::cout << "Message size : " << msg.size() << std::endl;

    // Testing with a vector
    std::vector<int> myVector = {1, 2, 3, 4, 5};
    msg << myVector;
    std::cout << "Message size : " << msg.size() << std::endl;

    // Testing with a custom data structure
    spk::CustomData myData = {7, 45, 3.14f};
    msg << myData;
    std::cout << "Message size : " << msg.size() << std::endl;

    int receivedInt;
    msg >> receivedInt;
    std::cout << "Sent integer: " << myInt << ", received integer: " << receivedInt << std::endl;
    std::string receivedString;
    msg >> receivedString;
    std::cout << "Sent string: " << myString << ", received string: " << receivedString << std::endl;
    std::vector<int> receivedVector;
    msg >> receivedVector;
    std::cout << "Sent vector: ";
    for (int v : myVector) std::cout << v << " ";
    std::cout << ", received vector: ";
    for (int v : receivedVector) std::cout << v << " ";
    std::cout << std::endl;
    spk::CustomData receivedData;
    msg >> receivedData;
    std::cout << "Sent custom data: {id: " << myData.id << ", value: " << myData.value
              << "}, received custom data: {id: " << receivedData.id << ", value: " << receivedData.value << "}" << std::endl;

    // Resetting and checking size
    msg.reset();
    std::cout << "Message size after reset: " << msg.header().length << std::endl;
    msg.clear();
    std::cout << "Message size after clear: " << msg.header().length << std::endl;

    return 0;
}
