#include "Class.h"
#include "Function.h"
#include "Enum.h"
#include "Struct.h"

int main(){
	Enum e = Enum::One;
	Class c;
	Struct s;
	Function(e, c, s);
	return 0;
}
