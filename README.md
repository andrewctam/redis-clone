# Redis Clone

This project is a distributed in-memory cache cloning many of the features of Redis.

# Installation
- **This project was developed on Ubuntu-22.04.** On Windows, you need to install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) before following the next steps. Alternatively, you could also open this project on [GitHub codespaces](https://github.com/codespaces).
- Clone this repository: `https://github.com/andrewctam/redis-clone.git`
- Install [cmake](https://cmake.org/install/).
- Install [cppzmq](https://github.com/zeromq/cppzmq).
- In `redis-clone/build`, run `cmake ..` to create a makefile.
- Finally, in `redis-clone/build` run `make` to run the makefile

```
git clone https://github.com/andrewctam/redis-clone.git

cd redis-clone/build

cmake ..

make
```

# Testing
- Run `cmake -DTest=ON ..` in `redis-clone/build` to build the tests.
- Change to the directory `redis-clone/build/tests` and execute `./tests` to run the tests

# Usage
- If you built this project from source, go to `redis-clone/build/programs`. Otherwise if you downloaded a release, unzip the file, and change to the directory.
- There should be 4 executables:
    - `./node` to creating a node.
    - `./client` for starting a client.
    - `./server` for starting worker nodes, a leader node, and a client.
- For a quick start, execute `./server` which will create 1 leader node, 3 worker nodes, and a client using fork and execv system calls.
    - Client messages are sent to stdout, while server messages are sent to stderr. If you want to separate the client messages from server messages, redirect stderr such as with `./server 2> file.txt`. 
- For more information about an executable and the args you can pass, run it with the `--help` flag.

# Commands
- For a list of commands, see [COMMANDS.md](./COMMANDS.md)
- Commands are case-insensitive, but keys/values are case-sensitive
    - `GET Andrew` is the same as `get Andrew` but is not the same as `GET ANDREW`
- Optional parameters are indicated with square brackets and ellipses,
    - For example, in `exists key [keys ...]`:
        - `exists name` is valid
        - `exists name1 name2 name3` is valid
        - `exists` is invalid as at least one key must be provided
