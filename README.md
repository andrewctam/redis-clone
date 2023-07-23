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
    - run `cmake -DTest=ON ..` to also build the tests
## Running the server
- In `./build`, execute `./src/server` to start the server.

## Running the tests cases
- Run `cmake  ..` then `make` to build the tests 
- In `./build`, execute `./tests/tests` to run all of the test cases


# Usage
- Once the server has started, you can send commands to stdin (probably the terminal used to launch the server), or connect using a client.
    - Using telnet: run `telnet localhost 9999` to connect to the server. Then, you can start sending commands to the server similar to.
    

## Commands
- For a list of commands, see [COMMANDS.md](./COMMANDS.md)
- Commands are case-insensitive, but keys/values are case sensitive
    - `GET name andrew` is the same as `get name andrew` but is not the same as `GET name Andrew`
- Optional parameters are indicated with square brackets and ellipses,
    - For example, in `exists key [keys ...]`:
        - `exists name` is valid
        - `exists name1 name2 name3` is valid
        - `exists` is invalid as at least one key must be provided
