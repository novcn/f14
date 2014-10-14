Control.Print.printDepth := 100;

datatype 'a tree = LEAF of 'a | NODE of 'a tree * 'a tree;

fun f(x : int, y : int) = x + y;

fun reduce (f) LEAF(x) = x 
|	reduce (f) (NODE(y,z)) = f (reduce y, reduce z);

val fir = NODE(NODE(LEAF 1,LEAF 2), LEAF 3);

val res = reduce f (fir);

(*    use "reduce.sml";    *)
