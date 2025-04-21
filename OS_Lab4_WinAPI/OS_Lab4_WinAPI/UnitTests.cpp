#include <gtest/gtest.h>
#include <windows.h>
#include <string>
#include "Common.h"

class QueueTest : public ::testing::Test {
protected:
    const DWORD NUM_RECORDS = 5;
    const DWORD FILE_SIZE = sizeof(Header) + NUM_RECORDS * sizeof(Message);

    HANDLE hMapFile = NULL;
    LPVOID pData = nullptr;
    Header* header = nullptr;
    Message* messages = nullptr;

    void SetUp() override {
        hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            FILE_SIZE,
            NULL
        );
        ASSERT_NE(hMapFile, NULL);

        pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, FILE_SIZE);
        ASSERT_NE(pData, nullptr);

        header = reinterpret_cast<Header*>(pData);
        messages = reinterpret_cast<Message*>(static_cast<BYTE*>(pData) + sizeof(Header));

        header->head = 0;
        header->tail = 0;
    }

    void TearDown() override {
        if (pData) UnmapViewOfFile(pData);
        if (hMapFile) CloseHandle(hMapFile);
    }

    void WriteMessage(const char* text) {
        Message msg(text);
        const DWORD index = header->tail;
        messages[index] = msg;
        header->tail = (index + 1) % NUM_RECORDS;
    }

    Message ReadMessage() {
        const DWORD index = header->head;
        Message msg = messages[index];
        header->head = (index + 1) % NUM_RECORDS;
        return msg;
    }
};

TEST_F(QueueTest, InitialStateIsEmpty) {
    EXPECT_EQ(header->head, 0);
    EXPECT_EQ(header->tail, 0);
    EXPECT_TRUE(header->head == header->tail);
}

TEST_F(QueueTest, SingleWriteAndRead) {
    WriteMessage("Test");
    EXPECT_EQ(header->tail, 1);

    Message msg = ReadMessage();
    EXPECT_EQ(header->head, 1);
    EXPECT_STREQ(msg.text, "Test");
}

TEST_F(QueueTest, BufferWraparound) {
    for (int i = 0; i < NUM_RECORDS; ++i) {
        WriteMessage(("Msg" + std::to_string(i)).c_str());
    }
    EXPECT_EQ(header->tail, 0);

    for (int i = 0; i < NUM_RECORDS; ++i) {
        Message msg = ReadMessage();
        EXPECT_EQ(header->head, (i + 1) % NUM_RECORDS);
    }
    EXPECT_EQ(header->head, 0);
}

TEST_F(QueueTest, MessageTruncation) {
    const char* longText = "This is a very long message over 20 characters";
    WriteMessage(longText);

    Message msg = ReadMessage();
    EXPECT_EQ(strlen(msg.text), MAX_MESSAGE_LENGTH - 1);
    EXPECT_STREQ(msg.text, "This is a very long ");
}

TEST_F(QueueTest, FullBufferDetection) {
    for (int i = 0; i < NUM_RECORDS; ++i) {
        WriteMessage("Msg");
    }

    const DWORD oldTail = header->tail;
    WriteMessage("Overflow");
    EXPECT_EQ(header->tail, oldTail);
}

TEST_F(QueueTest, EmptyBufferDetection) {
    Message msg = ReadMessage();
    EXPECT_EQ(header->head, 0);
    EXPECT_EQ(msg.text[0], '\0');
}

TEST_F(QueueTest, ConcurrentAccessSimulation) {
    WriteMessage("Msg1");
    WriteMessage("Msg2");

    Message msg1 = ReadMessage();
    Message msg2 = ReadMessage();

    EXPECT_STREQ(msg1.text, "Msg1");
    EXPECT_STREQ(msg2.text, "Msg2");
    EXPECT_EQ(header->head, 2);
    EXPECT_EQ(header->tail, 2);
}

TEST_F(QueueTest, MixedOperations) {
    WriteMessage("A");
    WriteMessage("B");
    ReadMessage();
    WriteMessage("C");
    ReadMessage();
    WriteMessage("D");

    EXPECT_EQ(header->head, 2);
    EXPECT_EQ(header->tail, 4);

    Message msg = ReadMessage();
    EXPECT_STREQ(msg.text, "C");
    EXPECT_EQ(header->head, 3);
}