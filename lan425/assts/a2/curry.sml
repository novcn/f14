Control.Print.printDepth := 100;

fun f(x,y) = x + y;

fun curry f x y = f (x, y); 
curry f 3 2; 

fun z x y = x + y;

fun uncurry z (x,y) = z x y;
uncurry z (6,3);

val g = (3,1);


(*fun curry (x : 'a): 'a->'b = ((fn ('b) => 'a * 'b) : 'b->'c);*)

