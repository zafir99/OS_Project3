
22 November 2025 6:31 PM

I don't plan on doing anything for this project right now. I'm just doing basic file setup for what I think I'll need, and the initial commit.


3 December 2025 1:12 PM

I'm going to code as much of it as I can to completion. To begin with, I'm going to code the basic file management and functions related to endianness first.
I'll make a struct for the B-Tree Node and then I'll just make sure very basic file reading and writing works how I want it to. After that, I'll get down to
how exactly to implement the child pointer insertions with the promotions and that whole mess. I expect that to be where most of the headache will lie.

4 December 2025 5:20 PM

I got done with the basic file handling and command parsing. Dealing with strings as argv is a lot easier than handling them from raw standard input.
I made a simple data structure for holding big endian integers. I could have just used uint64_t, but I think making it more explicit like this with a union is
nicer from an intuitive standpoint. What I have left to do is the actual insertion logic of the B-Tree, reading the B-Tree, outputting to csv, and reading from
csv to insert into the B-Tree. The insertion logic is really the bulk of the work. Reading the B-Tree is very simple: just read the blocks into memory and 
output the corresponding key value pairs with the TreeNode struct. Outputting to csv requires that and then formatting it with a comma and newline. Reading from
csv is part of basic file handling, and inserting the values from it directly follows from the fundamental insertion logic. So, my next course of action is to
tackle how to do insertion and promotion.

8 December 2025 10:51 PM

I have been working on the project every day, extensively so, but I have not been updating the devlog. I finished all of the basic methods apart from loading
the tree and inserting into the tree. Loading the tree is of course using the tree insertion method at its core, and the tree insertion is very complicated to
figure out so far. Granted, I have not looked up any B-Tree algorithms, but this is still much more difficult than I expected. If I used recursive logic it
would certainly be easier to implement, but it would be much heavier on memory usage, and in practice it could get excessive depending on how many nodes need
to be split and promoted. I'll see how much I can get done, but I'm not sure if I will be able to complete it. At a minimum, very basic insertion on a single
node, and every other operation work as expected.
