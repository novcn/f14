

fun doublist nil = nil
| doublist(h::t) = 2*h :: doublist t;


doublist [4, 2, 5, 1];
