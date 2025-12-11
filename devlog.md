
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


9 December 2025 2:49 AM

I forgot to mention in my previous update that I adjusted my typedef struct for the big endian integers; I'm using uint64_t as the typedef for them now instead
and am using bitwise operators (shifting) to reverse the endianness if needed. Besides that, I got my load tree function completed, since I realized I could just
put in my insert tree function as it's the core logic in the first place. I also have a rough insertion working for 1 level, that is, the root and one level below.
Hopefully I'll finish tomorrow, or later today I suppose.


10 December 2025 7:06 PM

I honestly did not expect to finish so strong. As far as I can tell, from all the testing I have done, the B-Tree works exactly as expected. I didn't even have
to look up any algorithm examples; I just thought about how the insertion algorithm was supposed to work for a really long time. I'm very happy with my results.
The hexdump looks just as expected, every key that was inserted can be searched for without issue, and there's decent enough error handling.
The most difficult part of this project was dealing with the endianness. That's where nearly all my segfaults came from. A quick note: there is no error handling
for an improperly formatted csv file, so make sure that the csv file being handed to the program is correctly formatted.


10 December 2025 7:44 PM

It was bugging me too much that I didn't have any type of error handling for a misformatted csv file so I added a method for it. Now the program properly parses
through most .csv labelled files. And I think with that, I'm basically done with this project.


