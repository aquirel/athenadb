## Networking ##
Port by default 3307, listen on all adresses.
Usage: athena `[<bind-address> [<bind-port>]]`

## Commands ##
<pre>
set a [<expression>] create set<br>
del a delete set<br>
exists a check set's existence<br>
contains set member check whether set contains member<br>
rename a b renaming set<br>
randset returns random set from db<br>
rand a returns random set member<br>
eval <expr> calculate expression and print result<br>
add a 1 add element to set<br>
rem b 2 remove element from set<br>
card a get set's cardinality<br>
mov src dest member move a member from one set to another<br>
pop key Remove and return a random member from a set<br>
lock a place writelock on a set<br>
unlock a remove writelock from a set<br>
ping test connection<br>
quit disconnect from server<br>
shutdown shutdown server<br>
flushall Remove all keys<br>
gc run garbage collection (removing unused objects)<br>
trunc shrink set sizes<br>
index print object index<br>
sets print all sets in db<br>
<br>
eq a . b set a equals set b?<br>
sube a b set a is a subset of b or equals to it?<br>
sub a b set a is a subset of b?<br>
<br>
NOTE: expressions can be used almost everywhere.<br>
</pre>

## Some examples ##
<pre>
add a 1<br>
rem a 2<br>
set a b<br>
set b b + c - d ~ e (x + y) + { 1, 2, 3, [4, 5, 6] }<br>
</pre>

## Expression syntax ##
<pre>
identifier<br>
operator<br>
{ ... } set<br>
[ ... ] tuple<br>
( ) - brackets for setting operator's precedence<br>
identifier - name of existing set<br>
</pre>

## Operations ##
<pre>
+ union<br>
- difference<br>
* intersect<br>
~ sym. difference<br>
@ cartesian product<br>
^ boolean (prefix op.)<br>
</pre>