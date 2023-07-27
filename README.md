# Redis Clone

This project is a distributed in-memory cache cloning many of the features of Redis.

# Installation
- **This project was developed on Ubuntu-22.04.** On Windows, you need to install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) before following the next steps. Alternatively, you could also open this project on [GitHub codespaces](https://github.com/codespaces).
- Clone this repository: `https://github.com/andrewctam/redis-clone.git`
- Install [cmake](https://cmake.org/install/).
- Install [cppzmq](https://github.com/zeromq/cppzmq) .
- In `./build`, run `cmake ..` to create a makefile (run `cmake -DTest=ON ..` to also build the tests). The initial cmake may take a while in order to fetch gRPC and GoogleTest
- Finally, in `/.build` run `make` to run the makefile

```
git clone https://github.com/andrewctam/redis-clone.git

cd redis-clone/build

cmake ..

make
```
# Usage
- In `./build`:
    - execute `./src/server` to start the server.
    - execute `./client/client` to start a client.
- Once the server has started, you can send commands from the client.
- To run the test cases, execute `./tests/tests` (must have been built with -DTEST=ON).
## Commands
- For a list of commands, see [COMMANDS.md](./COMMANDS.md)
- Commands are case-insensitive, but keys/values are case-sensitive
    - `GET Andrew` is the same as `get Andrew` but is not the same as `GET ANDREW`
- Optional parameters are indicated with square brackets and ellipses,
    - For example, in `exists key [keys ...]`:
        - `exists name` is valid
        - `exists name1 name2 name3` is valid
        - `exists` is invalid as at least one key must be provided
