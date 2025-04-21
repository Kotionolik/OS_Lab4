#include <gtest/gtest.h>
#include "Common.h"
#include <boost/interprocess/managed_mapped_file.hpp>

namespace bip = boost::interprocess;

class QueueTest : public ::testing::Test {
protected:
    const char* filename = "test_queue.bin";
};

TEST_F(QueueTest, BasicFunctionality) {

    const size_t max_messages = 3;
    bip::managed_mapped_file file(
        bip::create_only,
        filename,
        sizeof(SharedData) + max_messages * sizeof(Message)
    );

    SharedData* data = file.construct<SharedData>("SharedData")();
    data->head = 0;
    data->tail = 0;
    data->max_messages = max_messages;

    Message* messages = reinterpret_cast<Message*>(static_cast<char*>(file.get_address()) + sizeof(SharedData));

    ASSERT_EQ(data->head, 0);
    ASSERT_EQ(data->tail, 0);
    ASSERT_EQ(data->max_messages, max_messages);

    new (&messages[0]) Message("Test1");
    data->tail = 1;
    ASSERT_STREQ(messages[data->head].text, "Test1");
}

TEST_F(QueueTest, CircularBufferBehavior) {
    const size_t max_messages = 2;
    bip::managed_mapped_file file(
        bip::create_only,
        filename,
        sizeof(SharedData) + max_messages * sizeof(Message)
    );

    SharedData* data = file.construct<SharedData>("SharedData")();
    data->max_messages = max_messages;

    Message* messages = reinterpret_cast<Message*>(static_cast<char*>(file.get_address()) + sizeof(SharedData));

    new (&messages[0]) Message("Msg1");
    data->tail = (data->tail + 1) % max_messages;

    new (&messages[1]) Message("Msg2");
    data->tail = (data->tail + 1) % max_messages;

    ASSERT_EQ(data->tail, 0);
    ASSERT_STREQ(messages[0].text, "Msg1");
    ASSERT_STREQ(messages[1].text, "Msg2");
}