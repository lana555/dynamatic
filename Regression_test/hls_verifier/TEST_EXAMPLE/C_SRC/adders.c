
#include "adders.h"
// input single variable, input pointer, input array, output pointer, output array, inout pointer, inout array
int adders(in_A A, in_B *B, in_C C[4], out_D *D, out_E E[4], inout_F *F, inout_G G[4]) {


//#pragma HLS INTERFACE ap_ctrl_none port=return
//#pragma HLS INTERFACE ap_ctrl_none port=return

// Prevent IO protocols on all input ports
//#pragma HLS INTERFACE ap_none port=in3
//#pragma HLS INTERFACE ap_none port=in2
//#pragma HLS INTERFACE ap_none port=in1


    



    // output pointer test
    *D = A +C[0];

    // output array test

    int i;
    for (i=0; i<4; i++){
    E[i] = C[i] + A;
}
    // inout pointer test
    *F = *F + C[0];

    // inout array test


    for (i=0; i<4; i++){
    G[i] = G[i] + A;
}

    // return test
	int sum;
	
	sum = A + C[0] + *B;
	
	return sum;

}





