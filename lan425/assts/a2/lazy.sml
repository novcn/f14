Control.Print.printDepth := 100;

datatype Seq = Nil | Cons of int * (unit -> Seq)

fun head (Cons (x, _)) = x;

fun tail (Cons (_, xs)) = xs ();

(*fun ones = Cons(a, fn ( => )head();*)

fun ones n = Cons(n, fn () => ones(n));
val alotof = ones 1;

fun intList n = Cons(n, fn () => intList(n+1));

fun takeN(0, _) = []
| takeN(_, Nil) = []
| takeN(i, Cons(n,xt)) = n :: takeN(i - 1, xt());

val lst =  intList 10;

takeN(4, lst);
