int global = 7;

int *f(int *x){
	 *x += 1;
	 global += 1;

	 return x;
}

int main(){
	 int x=3;

	 (*f(&x))++;

	// 18
	return global + x;
}
