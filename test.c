int main(){
	int i;
	int max=17;
	printf("%d\n",max);

	scanf("%d",&max);
	for(i=1;i<=max;i=i+1){
		if(i%3==0 && i%5==0)
			printf("FizzBuzz\n");
		else if(i%3==0)
			printf("Fizz\n");
		else if(i%5==0)
			printf("Buzz\n");
		else
			printf("%d\n",i);
	}

	return 0;
}

