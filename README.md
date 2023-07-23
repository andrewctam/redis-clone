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
- `echo [args ...]` 
    - Returns the args you send.
- `monitor`
    - Can only be exectued from stdin, otherwise returns "DENIED".
    - Toggles live montioring of commands (sent to stderr). Returns "ACTIVE" or "INACTIVE".
- `shutdown`
    - Can only be exectued from stdin, otherwise returns "DENIED".
    - Stops the server.
- `get key` 
    - Returns the value of key, or "(NIL)" if no value is found.
- `set key value` 
    - Sets key to value. 
    - Returns "SUCCESS" or "FAILURE" depending on if the operation was successful.
- `del key [keys ...]` 
    - Deletes the entries at the specified keys. Returns the number of keys that were removed.
- `exists key [keys ...]`
    - Returns the number of inputted keys that exist. 
- `expire key secs`
    - Expires the key after the specified seconds
    - Returns the  unix seconds timestamp the key will expire at, or "FAILURE" if the key is not found.
- `expire key unix`
    - Expires the key at the specified unix seconds timestamp
    - Returns "SUCCESS", or "FAILURE" if the key is not found.
    