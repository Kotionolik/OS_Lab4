#include <iostream>
#include <string>
#include <Windows.h>
#include "Common.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: Sender.exe <fileName>" << std::endl;
        return 1;
    }

    std::string fileName = argv[1];

    HANDLE hFile = CreateFileA(
        fileName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file. Error: " << GetLastError() << std::endl;
        return 1;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        std::cerr << "GetFileSize failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return 1;
    }

    HANDLE hMapFile = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, fileSize, NULL);
    if (!hMapFile) {
        std::cerr << "CreateFileMapping failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return 1;
    }

    LPVOID pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, fileSize);
    if (!pData) {
        std::cerr << "MapViewOfFile failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        return 1;
    }

    Header* pHeader = (Header*)pData;
    DWORD numRecords = (fileSize - sizeof(Header)) / sizeof(Message);

    HANDLE fileMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, FILE_MUTEX_NAME.c_str());
    HANDLE freeSlotsSem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, FREE_SLOTS_SEM_NAME.c_str());
    HANDLE usedSlotsSem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, USED_SLOTS_SEM_NAME.c_str());
    HANDLE readyEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, SENDERS_READY_EV_NAME.c_str());

    if (!fileMutex || !freeSlotsSem || !usedSlotsSem || !readyEvent) {
        std::cerr << "Failed to open sync objects. Error: " << GetLastError() << std::endl;
        return 1;
    }

    SetEvent(readyEvent);

    while (true) {
        
        std::string cmd;
        std::cout << "Enter command (send/exit): ";
        std::cin >> cmd;

        if (cmd == "send") {

            WaitForSingleObject(freeSlotsSem, INFINITE);
            WaitForSingleObject(fileMutex, INFINITE);

            std::string input;
            std::cout << "Enter message (up to 20 chars): ";
            std::cin.ignore();
            std::getline(std::cin, input);

            Message msg(input.c_str());

            Message* messages = (Message*)((BYTE*)pData + sizeof(Header));
            messages[pHeader->tail] = msg;
            pHeader->tail = (pHeader->tail + 1) % numRecords;

            ReleaseSemaphore(usedSlotsSem, 1, NULL);
            ReleaseMutex(fileMutex);

            std::cout << "Message sent." << std::endl;
        }
        else if (cmd == "exit") {
            ReleaseMutex(fileMutex);
            break;
        }
        else {
            std::cout << "Unknown command." << std::endl;
        }
    }

    UnmapViewOfFile(pData);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
    CloseHandle(fileMutex);
    CloseHandle(freeSlotsSem);
    CloseHandle(usedSlotsSem);
    CloseHandle(readyEvent);

    return 0;
}