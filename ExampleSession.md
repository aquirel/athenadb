
```
Hello.
ping
PONG.
sets
No sets in db.
index
Object index dump. Total: 0
set a
OK.
eval a
{ }
set b { 1, { 2 }, [ 3 ] }
OK.
eval b
{ 1, { 2 }, [ 3 ] }
index
Object index dump. Total: 7
         0: set = { }
         1: val = 1
         2: val = 2
         3: set = { 2 }
         4: val = 3
         5: tuple = [ 3 ]
         6: set = { 1, { 2 }, [ 3 ] }
contains b 1
1
contains b 2
0
contains b {2}
1
eval b @ b
{ [ 1, 1 ], [ 1, { 2 } ], [ 1, [ 3 ] ], [ { 2 }, 1 ], [ { 2 }, { 2 } ], [ { 2 }, [ 3 ] ], [ [ 3 ], 1 ], [ [ 3 ], { 2 } ], [ [ 3 ], [ 3 ] ] }
set c a + b
OK.
eval c
{ 1, { 2 }, [ 3 ] }
eval b
{ 1, { 2 }, [ 3 ] }

Hello.
set b { 1, { 2 }, [ 3 ] }
OK.
eval b
{ 1, { 2 }, [ 3 ] }
set c { b + { 88 }, [ 33, 4, 5] }
OK.
eval c
{ { 1, { 2 }, [ 3 ], 88 }, [ 33, 4, 5 ] }
eval ^c
{ { }, { { 1, { 2 }, [ 3 ], 88 }, [ 33, 4, 5 ] }, { [ 33, 4, 5 ] }, { { 1, { 2 }, [ 3 ], 88 } } }
quit
Bye.
```