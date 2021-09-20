kv.c is the C program to implement a key-value pair map.

HOW TO USE:
This program allows you to perform the following 5 actions on the key value store:

1. Put
Puts a key value pair to the map. If there already exists a key value pair in the map with the same key, it replaces the value with the new value. It can be called as follows - p,1,Deepti
2. Get
Gets a key value pair from the map. If no entry is in the map with the input key, displays "Key not found". Called as follows - g,1
3. Delete
Deletes a key value pair from the map. If no entry is in the map with the input key, displays "Key not found". Called as follows - d,1
4. Clear
Empties the whole map. Called as follows - c
5. All
Displays all the key value pairs present in the map. Called as follows - a

IMPLEMENTATION:
The implementation is done by using an array of singly linked lists. The array contains 100 such linked lists, whose memory is allocated on a per-requiremen
t basis. The idea is to use a hash of the input key (1 in the above example), and store the key value pair in the array with index as hash(key), where the hash function is a simple mod100 function. This would result in collisions, for eg., the following commands p,1,Deepti, p,101,Rajagopal would result in the hash function being same for both, as 1 % 100 = 1 = 101 % 100. This is when the linked list comes into picture. If there already exists a key value pair at hash(key), then add a new node with the new key value pair, and make it the new head of the linked list. At the end of the first input it would look like this:

Arr Idx        Head
 ---        ----------
 |1|        |1,Deepti|
 ---        ----------

It will now look like this

Arr Idx         Head
 ---        ---------------    ----------
 |1|        |101,Rajagopal|--->|1,Deepti|
 ---        ---------------    ----------
