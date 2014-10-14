Control.Print.printDepth := 100;

datatype 'a tree = LEAF of 'a | NODE of 'a tree * 'a tree;

fun inctree(LEAF(x)) = LEAF (x+1)
| inctree(NODE(y,z)) = NODE (inctree y, inctree z);

val fir = NODE(NODE(LEAF 0,LEAF 0), LEAF 0);

inctree (fir);