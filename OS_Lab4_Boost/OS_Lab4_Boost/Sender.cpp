#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <iostream>
#include <string>
#include <cstring>
#include "Common.h"

namespace bip = boost::interprocess;

int main(int argc, char* argv[]) {
    if (argc != 2) { std::cerr << "Usage: Sender <filename>\n"; return 1; }
    std::string filename = argv[1];

    bip::managed_mapped_file file(
        bip::open_only,
        filename.c_str()
    );

    SharedData* data = file.find<SharedData>("SharedData").first;
    Message* messages = reinterpret_cast<Message*>(static_cast<char*>(file.get_address()) + sizeof(SharedData));

    bip::named_mutex mutex(bip::open_only, (filename + "_mutex").c_str());
    bip::named_semaphore free_sem(bip::open_only, (filename + "_free").c_str());
    bip::named_semaphore used_sem(bip::open_only, (filename + "_used").c_str());
    bip::named_semaphore ready_sem(bip::open_only, (filename + "_ready").c_str());
    ready_sem.post();

    std::string cmd;
    while (std::cout << "Enter command (send/exit): " && std::cin >> cmd) {
        if (cmd == "send") {
            std::string text;
            std::cout << "Enter message: ";
            std::cin.ignore();
            std::getline(std::cin, text);

            if (text.size() >= 20) text.resize(19);
            std::memset(messages[data->tail].text, 0, 20);
            std::memcpy(messages[data->tail].text, text.c_str(), text.size());

            free_sem.wait();
            bip::scoped_lock<bip::named_mutex> lock(mutex);
            data->tail = (data->tail + 1) % data->max_messages;
            used_sem.post();
        }
        else if (cmd == "exit") break;
    }
}