# Redis Clone

This project is a distributed in-memory cache cloning many of the features of Redis.

#### Table of Contents
- [Installation](#installation)
    - [Running the server](#running-the-server)
    - [Running the test cases](#running-the-test-cases)
- [Usage](#usage)
    - [Commands](#commands)

# Installation
- Clone this repository: `git clone https://github.com/andrewctam/redis-clone.git`
- On Windows, you should install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) before following the next steps.
- Make sure you have cmake and a C++ compiler installed
- In `./build`, run `cmake ..` to create a makefile, then run `make` to run the makefile

## Running the server
- Execute `./build/src/RedisClone` to start the server.

## Running the tests cases
- Execute `./build/tests/tests` to run all of the test cases


# Usage
- Once the server has started, you can send commands in the same terminal used to launch the server, or connect using a client.
    - Using telnet: 
        - Open a new terminal and run `telnet localhost 9999` to connect to the server. Then, you can start sending commands to the server.
    

## Commands