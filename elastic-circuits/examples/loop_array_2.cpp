

// input single variable, input pointer, input array, output pointer, output array, inout pointer, inout array
void loop_array_4_graph(int n, int a[10], int b[10], int p, int q){
	int x = 7;
	int y = p*q;


	for (int i = 0; i < n; i++) {
		a[i] = x * i * i;
	}


	for (int i = 0; i < n; i++) {
		b[i] = y * i * i;
	}

}

int main () {
	int a[10];
	int b[10];
	loop_array_4_graph(5,a,  b,  5,  5);
	return 0;
}