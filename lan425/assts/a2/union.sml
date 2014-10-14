Control.Print.printDepth := 100;

datatype intstring = tag_int of int | tag_str of string;

val x = if true then tag_str("cats")
else tag_int(3);

let val tag_int(m) = x in m + 5 end;