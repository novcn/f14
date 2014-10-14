Control.Print.printDepth := 100;

fun f(x) = x+1;

fun	map f nil = nil
|	map f (h::t) = (f h)::(map f t);

val lst = [1,2,3,4];

val res = map f (lst);
