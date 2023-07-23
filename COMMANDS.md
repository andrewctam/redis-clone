# Commands
#### Table of Contents
- [Misc](#misc)
- [Server](#server)
- [Basic Cache](#basic-cache)

## Misc
- `echo [args ...]` 
    - Returns the args you send.

## Server
These commands can only be exectued from stdin, otherwise they will return "DENIED".

- `monitor`    
    - Toggles live montioring of commands (sent to stderr). Returns "ACTIVE" or "INACTIVE".
- `shutdown`
    - Stops the server.
- `keys`
    - Returns all keys.
- `benchmark num`
    - Performs num / 2 get commands and num / 2 set commands.
    - Returns the time taken in ms.
## Basic Cache
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
- `expireat key unix`
    - Expires the key at the specified unix seconds timestamp
    - Returns "SUCCESS", or "FAILURE" if the key is not found.

## Ints
Setting a key to an integer will store it as an int, such as `set num 123`. The following commands can be used on ints, and will return "NOT AN INT" if used on other value types.
- `incr key`
    - Increments the value of key by 1. If the key does not exist, it is created and set to 1.
    - Returns the new value of key.
- `incrby key val`
    - Increments the value of key by the specificed value. If the key does not exist, it is created and set to val.
    - Returns the new value of key.
- `decr key`
    - Decrements the value of key by 1. If the key does not exist, it is created and set to -1.
    - Returns the new value of key.
- `decrby key val`
    - Increments the value of key by the specificed value. If the key does not exist, it is created and set to -val.
    - Returns the new value of key.
