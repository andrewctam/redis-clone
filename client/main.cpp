#include <string>
#include <iostream>

#include <zmq.hpp>

int main()
{
    zmq::context_t context{1};

    zmq::socket_t socket{context, zmq::socket_type::req};
    socket.connect("tcp://localhost:5555");

    std::cout << "Client started!" << std::endl;

    while (1) {
        std::cout << "> ";
        std::flush(std::cout);

        std::string input;
        std::getline(std::cin, input);
    
        socket.send(zmq::buffer(input), zmq::send_flags::none);
        
        zmq::message_t reply{};
        socket.recv(reply, zmq::recv_flags::none);

        std::cout << reply.to_string() << std::endl;
    }

    return 0;
}
