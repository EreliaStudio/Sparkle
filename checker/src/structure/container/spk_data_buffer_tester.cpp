#include "structure/container/spk_data_buffer_tester.hpp"

TEST_F(DataBufferTest, DefaultConstructor)
{
    ASSERT_EQ(buffer.size(), 0) << "Buffer size should be 0 after default construction";
    ASSERT_EQ(buffer.bookmark(), 0) << "Bookmark should be 0 after default construction";
}

TEST_F(DataBufferTest, ConstructorWithSize)
{
    spk::DataBuffer bufferWithSize(100);
    ASSERT_EQ(bufferWithSize.size(), 100) << "Buffer size should be 100 after construction with size";
    ASSERT_EQ(bufferWithSize.bookmark(), 0) << "Bookmark should be 0 after construction with size";
}

TEST_F(DataBufferTest, Resize)
{
    buffer.resize(50);
    ASSERT_EQ(buffer.size(), 50) << "Buffer size should be 50 after resizing";
}

TEST_F(DataBufferTest, Skip)
{
    buffer.resize(100);
    buffer.skip(10);
    ASSERT_EQ(buffer.bookmark(), 10) << "Bookmark should be 10 after skipping 10 bytes";

    ASSERT_THROW(buffer.skip(100), std::runtime_error) << "Skipping more than available bytes should throw an error";
}

TEST_F(DataBufferTest, Clear)
{
    buffer.resize(50);
    buffer.clear();
    ASSERT_EQ(buffer.size(), 0) << "Buffer size should be 0 after clearing";
    ASSERT_EQ(buffer.bookmark(), 0) << "Bookmark should be 0 after clearing";
}

TEST_F(DataBufferTest, Reset)
{
    buffer.resize(50);
    buffer.skip(10);
    buffer.reset();
    ASSERT_EQ(buffer.bookmark(), 0) << "Bookmark should be 0 after resetting";
}

TEST_F(DataBufferTest, AppendData)
{
    std::vector<uint8_t> data = { 1, 2, 3, 4, 5 };
    buffer.append(data.data(), data.size());
    ASSERT_EQ(buffer.size(), data.size()) << "Buffer size should match the appended data size";

    for (size_t i = 0; i < data.size(); ++i)
    {
        ASSERT_EQ(buffer.data()[i], data[i]) << "Buffer data should match appended data";
    }
}

TEST_F(DataBufferTest, EditAndRetrieve)
{
    buffer.resize(10);
    int32_t value = 42;
    buffer.edit(0, value);

    int32_t retrievedValue = buffer.get<int32_t>();
    ASSERT_EQ(retrievedValue, value) << "Retrieved value should match the edited value";
}

TEST_F(DataBufferTest, InsertAndRetrieveStandardLayout)
{
    int32_t intValue = 12345;
    buffer << intValue;

    int32_t retrievedIntValue;
    buffer >> retrievedIntValue;
    ASSERT_EQ(retrievedIntValue, intValue) << "Retrieved int value should match the inserted value";
}

TEST_F(DataBufferTest, InsertAndRetrieveContainer)
{
    std::vector<int32_t> intVector = { 1, 2, 3, 4, 5 };
    buffer << intVector;

    std::vector<int32_t> retrievedIntVector;
    buffer >> retrievedIntVector;
    ASSERT_EQ(retrievedIntVector, intVector) << "Retrieved vector should match the inserted vector";
}

TEST_F(DataBufferTest, EditRawData)
{
    buffer.resize(10);
    const char* rawData = "hello";
    buffer.edit(0, rawData, strlen(rawData) + 1);

    ASSERT_STREQ(reinterpret_cast<const char*>(buffer.data()), rawData) << "Buffer data should match the edited raw data";
}