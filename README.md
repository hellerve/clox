# clox

My attempt to organize the code that was handed to us by Bob Nystrom in his
book [Crafting Interpreters](http://www.craftinginterpreters.com/chunks-of-bytecode.html),
and follow along in the process. I suggest you go read the book, it’s a lot of
fun.

This version tries to incorporate all of the challenges at the end of the chapters. I also took the liberty to add a `char` type that, as of now, doesn’t
do much. You can concatenate it to other chars or strings, though.

It depends on [libreadline](https://tiswww.case.edu/php/chet/readline/rltop.html).
