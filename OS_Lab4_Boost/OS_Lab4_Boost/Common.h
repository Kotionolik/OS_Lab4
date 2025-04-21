#pragma once

#pragma pack(push, 1)
struct Message {
    char text[20];
    Message() = default;
    Message(const char* str) {
        strncpy_s(text, str, 19);
        text[19] = '\0';
    }
};

struct SharedData {
    size_t head;
    size_t tail;
    size_t max_messages;
};
#pragma pack(pop)