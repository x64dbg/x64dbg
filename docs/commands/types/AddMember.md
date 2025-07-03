# AddMember

Add a new member to the end of a struct/union.

## arguments

`arg1` The type name of the struct/union (parent).

`arg2` The type of the new member.

`arg3` The name of the new member.

`[arg4]` The array size. A value greater than zero will make this member an array.

`[arg5]` Offset from the start of the structure, only use this for implicitly padded structures. Overlapping with other members is **not** allowed.

## result

This command does not set any result variables.
