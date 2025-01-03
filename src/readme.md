# remaining issues:

## 1 resize problem
1. take small size at beginning, and resize when needed - done
1. don't copy data when resizing, extend data section - done
1. mark the removed slots
1. reuse the removed slots
1. shrink the size when no need

## 2 auto sync
1. The new created MMF is deleted before server synched it
1. Any new put data blocks are in MMF but not in key-position hashmap so cant be queried


## Put function
1. Update same key again should not create a new block (if the key is put by another client before, then loop it if search fails)
1. The first client expand a MMF, the second client should sync before using it (put/get)

## 3 use hash code not loop to query
1. fully done

## 4 Server process
1. needs separate class and separate API

## logs
1. log client operation - done
1. add client ID - done

## code management
1. C# separate library project - done

## Misc
1. save current snapshot
1. configurable data sizes

## testing
1. ablility to keep testing 
1. long endurance, concurrent testing