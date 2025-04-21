#include <iostream>
#include <string>
#include <Windows.h>
#include "Common.h"

int main() {
    std::string fileName;
    DWORD numRecords;
    DWORD numSenders;

    std::cout << "Enter file name (without extension): ";
    std::cin >> fileName;
    fileName += ".bin";
    std::cout << "Enter number of records: ";
    std::cin >> numRecords;
    while (numRecords < 1)
    {
        std::cout << "Error: number of records is less than one. Enter again: ";
        std::cin >> numRecords;
    }
    std::cout << "Enter number of Senders: ";
    std::cin >> numSenders;
    while (numSenders < 1)
    {
        std::cout << "Error: number of senders is less than one. Enter again: ";
        std::cin >> numSenders;
    }

    HANDLE hFile = CreateFileA(
        fileName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create file. Error: " << GetLastError() << std::endl;
        return 1;
    }

    DWORD fileSize = sizeof(Header) + numRecords * sizeof(Message);
    SetFilePointer(hFile, fileSize, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);

    HANDLE hMapFile = CreateFileMappingA(
        hFile,
        NULL,
        PAGE_READWRITE,
        0,
        fileSize,
        NULL
    );

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
    *pHeader = Header();

    HANDLE fileMutex = CreateMutexA(NULL, FALSE, FILE_MUTEX_NAME.c_str());
    HANDLE freeSlotsSem = CreateSemaphoreA(NULL, numRecords, numRecords, FREE_SLOTS_SEM_NAME.c_str());
    HANDLE usedSlotsSem = CreateSemaphoreA(NULL, 0, numRecords, USED_SLOTS_SEM_NAME.c_str());

    if (!fileMutex || !freeSlotsSem || !usedSlotsSem) {
        std::cerr << "Failed to create sync objects. Error: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        return 1;
    }

    HANDLE* senders = new HANDLE[numSenders];
    HANDLE* events = new HANDLE[numSenders];

    for (DWORD i = 0; i < numSenders; ++i) {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        std::string cmdLine = "Sender.exe " + fileName;

        HANDLE event = CreateEventA(NULL, FALSE, FALSE, SENDERS_READY_EV_NAME.c_str());
        events[i] = event;

        if (!CreateProcessA(
            NULL,
            const_cast<LPSTR>(cmdLine.c_str()),
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi
        )) {
            std::cerr << "Failed to start Sender. Error: " << GetLastError() << std::endl;
        }
        else {
            senders[i] = pi.hProcess;
            CloseHandle(pi.hThread);
        }
    }

    WaitForMultipleObjects(numSenders, events, TRUE, INFINITE);

    std::cout << "All Senders are ready." << std::endl;

    while (true) {
        std::string cmd;
        std::cout << "Enter command (read/exit): ";
        std::cin >> cmd;

        if (cmd == "read") {
            WaitForSingleObject(usedSlotsSem, INFINITE);
            WaitForSingleObject(fileMutex, INFINITE);

            Header header = *pHeader;
            Message* messages = (Message*)((BYTE*)pData + sizeof(Header));
            Message msg = messages[header.head];
            header.head = (header.head + 1) % numRecords;
            *pHeader = header;

            ReleaseMutex(fileMutex);
            ReleaseSemaphore(freeSlotsSem, 1, NULL);

            std::cout << "Received: " << msg.text << std::endl;
        }
        else if (cmd == "exit") {
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
    for (int i = 0; i < numSenders; ++i) {
        CloseHandle(events[i]);
        CloseHandle(senders[i]);
    }
    delete[] events;
    delete[] senders;

    return 0;
}