# cmp

This command compares two expressions. Notice that when you want to check for values being bigger or smaller, the comparison arg1>arg2 is made. If this evaluates to true, the $_BS_FLAG is set to 1, meaning the value is bigger. So you test if arg1 is bigger/smaller than arg2.

## arguments

`arg1` First expression to compare.

`arg2` Second expression to compare.

## result

This command sets the internal variables $_EZ_FLAG and $_BS_FLAG. They are checked when a branch is performed.
