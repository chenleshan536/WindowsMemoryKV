# remaining issues:

## 1 resize problem
1. mark the removed slots
1. reuse the removed slots

## 2 auto sync
1. The new created MMF is deleted before server synched it
1. Any new put data blocks are in MMF but not in key-position hashmap so cant be queried

## 3 use hash code not loop to query
1. fully done

## 4 Server process
1. needs separate class and separate API

## logs
1. log client operation - done
1. add client ID - done

## code management
1. C# separate library project

## Misc
1. save current snapshot
1. configurable data sizes

## testing
1. ablility to keep testing 
1. long endurance, concurrent testing