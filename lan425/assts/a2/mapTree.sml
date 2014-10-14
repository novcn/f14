Control.Print.printDepth := 100;

datatype 'a tree = LEAF of 'a | NODE of 'a tree * 'a tree;

fun f(x) = x + 1;

fun maptree f (LEAF(x)) = LEAF (f x)
|	maptree f (NODE(y,z)) = NODE (maptree f y, maptree f z);

val fir = NODE(NODE(LEAF 1,LEAF 2), LEAF 3);

val res = maptree f (fir);

(*      use "mapTree.sml";       *)


(*fn : ('a -> 'b) -> 'a tree -> 'b tree*)