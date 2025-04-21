#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/callable_traits/args.hpp>
#include <boost/process/v1/search_path.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "Common.h"

namespace bip = boost::interprocess;
namespace bp = boost::process;
namespace bpv = boost::process::v1;

int main() {
    std::string filename;
    size_t max_messages, num_senders;

    std::cout << "Enter filename: "; std::cin >> filename;
    std::cout << "Enter max messages: "; std::cin >> max_messages;
    std::cout << "Enter number of senders: "; std::cin >> num_senders;

    bip::managed_mapped_file file(
        bip::create_only,
        filename.c_str(),
        sizeof(SharedData) + max_messages * sizeof(Message)
    );

    SharedData* data = file.construct<SharedData>("SharedData")();
    data->head = data->tail = 0;
    data->max_messages = max_messages;

    const std::string mutex_name = filename + "_mutex";
    bip::named_mutex mutex(bip::create_only, mutex_name.c_str());
    bip::named_semaphore free_sem(bip::create_only, (filename + "_free").c_str(), max_messages);
    bip::named_semaphore used_sem(bip::create_only, (filename + "_used").c_str(), 0);
    bip::named_semaphore ready_sem(bip::create_only, (filename + "_ready").c_str(), 0);

    std::vector<bpv::child> senders;
    for (size_t i = 0; i < num_senders; ++i) {
        senders.emplace_back(
            bpv::child(
                bpv::search_path("Sender"),
                filename
            )
        );
    }

    for (size_t i = 0; i < num_senders; ++i) ready_sem.wait();

    Message* messages = reinterpret_cast<Message*>(static_cast<char*>(file.get_address()) + sizeof(SharedData));
    std::string cmd;
    while (std::cout << "Enter command (read/exit): " && std::cin >> cmd) {
        if (cmd == "read") {
            used_sem.wait();
            bip::scoped_lock<bip::named_mutex> lock(mutex);
            std::cout << "Message: " << messages[data->head].text << '\n';
            data->head = (data->head + 1) % data->max_messages;
            free_sem.post();
        }
        else if (cmd == "exit") break;
    }

    for (auto& s : senders) s.terminate();
    bip::named_mutex::remove(mutex_name.c_str());
    bip::named_semaphore::remove((filename + "_free").c_str());
    bip::named_semaphore::remove((filename + "_used").c_str());
    bip::named_semaphore::remove((filename + "_ready").c_str());
}