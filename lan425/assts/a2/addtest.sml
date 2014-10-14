Control.Print.printDepth := 100;

datatype E = I of int | Plus of E * E ;

fun calc (Plus (I n, I m)) = I (n+m)
|   calc (Plus (I n, e2 )) = Plus(I n, calc e2)
|   calc (Plus (e1,  e2 )) = Plus (calc e1, e2);

fun   eval  (I n) = n
|     eval   exp  = eval (calc exp) ;


val testv = Plus(Plus(I 3, I 3), Plus(I 4,I 5));

val resone = calc testv;
val restwo = cal resone;