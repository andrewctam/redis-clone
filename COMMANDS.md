# Commands
#### Table of Contents
- [Misc](#misc)
- [Nodes](#nodes)
- [Basic Cache](#basic-cache)
- [Ints](#ints)
- [Lists](#lists)

## Misc
- `echo [args ...]` 
    - Returns the args you send.
- `ping`
    - Returns PONG.
- `monitor`    
    - Toggles live montioring of commands (sent to stderr). Returns ACTIVE or INACTIVE.
- `benchmark num`
    - Performs num commands and returns the time taken in ms.
## Nodes
- `nodes`
    - List all nodes with their pid and leader status.
- `create`
    - Creates a new node.
- `kill pid`
    - Kills the node with the specified pid.
- `shutdown`
    - Shutsdown the server and stops all nodes.
## Basic Cache
- `get key` 
    - Returns the value of key, or (NIL) if no value is found.
- `set key value` 
    - Sets key to value. 
    - Returns SUCCESS or FAILURE depending on if the operation was successful.
- `rename key newkey`
    - Renames a key to newkey.
    - Returns SUCCESS, or FAILURE if key is not found. 
- `del key [keys ...]` 
    - Deletes the entries at the specified keys. Returns the number of keys that were removed.
- `exists key [keys ...]`
    - Returns the number of inputted keys that exist. 
- `expire key secs`
    - Expires the key after the specified seconds.
    - Returns the unix seconds timestamp the key will expire at, or FAILURE if the key is not found.
- `expireat key unix`
    - Expires the key at the specified unix seconds timestamp.
    - Returns SUCCESS, or FAILURE if the key is not found.
- `persist key`
    - Removes the expiration set on key.
    - Returns SUCCESS, or FAILURE if there was no expiration or if the key was not found 
- `dbsize`
    - Returns the number of keys currently stored in the cache
    - Expired keys are lazy deleted and may be falsely reflected in the count.
- `type key`
    - Returns the type of the key (string, int, or list) or (NIL) if not found.
- `keys`
    - Returns all keys.
- `flushall`
    - Deletes all keys.

## Ints
Setting a key to an integer will store it as an int, such as `set num 123`. The following commands can be used on ints, and will return NOT AN INT if used on other value types.
- `incr key`
    - Increments the value of key by 1. If the key does not exist, it is created and set to 1.
    - Returns the new value of key.
- `incrby key val`
    - Increments the value of key by the specified value. If the key does not exist, it is created and set to val.
    - Returns the new value of key.
- `decr key`
    - Similar to incr but decrements.
- `decrby key val`
    - Similar to incrby but decrements.

## Lists
Use `lpush` or `rpush` to set a key to a Linked List, which can store either strings and ints. Using these commands on other data types will return NOT A LIST.
- `lpush key element [elements...]`
    - Inserts the elements to the head of the list stored at key, creating the list if it does not exist.
    - Elements are processed from left to right, so `LPUSH key 1 2 3` sets key to [3 2 1].
    - Returns the new length of the list
- `rpush key element [elements...]`
    - Same as lpush but elements are inserted from the tail of the list.
    - Elements are processed from left to right, so `RPUSH key 1 2 3` sets key to [1 2 3].
- `lpop key [count]`
    - Remvoves and returns elements from the head of the list 
    - If count is not provided, removes 1 element.
    - If there is nothing stored at key, returns (NIL).
- `rpop key [count]`
    - Same as lpop but from the tail of the list.
- `lrange key [start] [stop]`
    - Returns and removes the elements of a list stored at key between start and stop (bounds are inclusive).
    - If start is not provided, all elements will be returned.
    - If stop is not provided, all elements after start will be returned.
    - For the bounds, -1 can be entered to indicate the last element, but other negative numbers will return an ERROR
- `llen key`
    - Returns the length of the list stored at key, or 0 is nothing stored at key
