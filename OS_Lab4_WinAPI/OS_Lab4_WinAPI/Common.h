#pragma once

#include <windows.h>
#include <string>
#include <cstring>

const DWORD MAX_MESSAGE_LENGTH = 20;

struct Message {
    char text[MAX_MESSAGE_LENGTH];
    Message() { memset(text, 0, MAX_MESSAGE_LENGTH); }
    Message(const char* str) {
        strncpy_s(text, str, MAX_MESSAGE_LENGTH - 1);
        text[MAX_MESSAGE_LENGTH - 1] = '\0';
    }
};

struct Header {
    DWORD head;
    DWORD tail;
    Header() : head(0), tail(0) {}
};

const std::string FILE_MUTEX_NAME = "Global\\FileMutex";
const std::string FREE_SLOTS_SEM_NAME = "Global\\FreeSlotsSem";
const std::string USED_SLOTS_SEM_NAME = "Global\\UsedSlotsSem";
const std::string SENDERS_READY_EV_NAME = "Global\\SendersReadyEv";