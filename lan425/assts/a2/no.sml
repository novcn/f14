Control.Print.printDepth := 100;

fun hd (Cons (x,xf)) = x 
   | hd Nil          =  raise Empty;

fun tl (Cons(x,xf)) = xf()
     |  tl Nil      =  raise Empty ;

(*fun cons(x,xq) = Cons(x, fn() => xq) ;

fun from k = Cons(k, fn() =>from(k+1)) ;
*)
(*val hun = from 100;*)