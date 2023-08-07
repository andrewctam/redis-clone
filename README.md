# Redis Clone

This project is a distributed in-memory cache cloning some of features of Redis.

Features include:
- Key-value mapping for strings, ints, and lists in O(1) using an LRU replacement policy. Additional constant and linear time operations, such as getting keys, partial list ranges, and more. See [COMMANDS.md](./COMMANDS.md) for all commands.
- Horizontal scalability, allowing nodes to join and leave dynamically.
- Consistent hashing to distribute the cache and provide fault tolerance. As new nodes join, the cache can be split and shared.
- Fault tolerance with leader elections. If a worker node detects the leader is no longer responding, a new one will be elected.

# Installation
- **This project was developed on Ubuntu-22.04.** On Windows, you need to install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) before following the next steps. Alternatively, you can try to open this project on [GitHub codespaces](https://github.com/codespaces).
- Install [cmake](https://cmake.org/install/).
- Install [cppzmq](https://github.com/zeromq/cppzmq).
- Clone this repository: `https://github.com/andrewctam/redis-clone.git`
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
- In `redis-clone/build/tests` execute `./tests/tests` to run the tests

# Usage
- If you built this project from source, go to `redis-clone/build/programs`. Otherwise if you downloaded a release, unzip the file and enter the unzipped folder.
- There should be 3 executables:
    - `./node` for creating a worker or leader node.
    - `./client` for starting a client.
    - `./server` for starting worker nodes, a leader node, and a client.
- For more information about an executable and the args you can pass, run it with the `--help` flag.
- For a quick start, execute `./server` which will create 1 leader node, 3 worker nodes, and a client using fork and execv system calls.


# Commands
- For a list of commands, see [COMMANDS.md](./COMMANDS.md)
- Commands are case-insensitive, but keys/values are case-sensitive
    - `GET Andrew` is the same as `get Andrew` but is not the same as `GET ANDREW`
- Optional parameters are indicated with square brackets and ellipses,
    - For example, in `exists key [keys ...]`:
        - `exists name` is valid
        - `exists name1 name2 name3` is valid
        - `exists` is invalid as at least one key must be provided
