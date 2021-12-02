/*******************************************************************************
Vendor: Xilinx 
Associated Filename: adders_test.c
Purpose: Vivado HLS tutorial example 
Device: All 
Revision History: March 1, 2013 - initial release
                                                
*******************************************************************************
Copyright 2008 - 2013 Xilinx, Inc. All rights reserved.

This file contains confidential and proprietary information of Xilinx, Inc. and 
is protected under U.S. and international copyright and other intellectual 
property laws.

DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials 
distributed herewith. Except as otherwise provided in a valid license issued to 
you by Xilinx, and to the maximum extent permitted by applicable law: 
(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX 
HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether 
in contract or tort, including negligence, or under any other theory of 
liability) for any loss or damage of any kind or nature related to, arising under 
or in connection with these materials, including for any direct, or any indirect, 
special, incidental, or consequential loss or damage (including loss of data, 
profits, goodwill, or any type of loss or damage suffered as a result of any 
action brought by a third party) even if such damage or loss was reasonably 
foreseeable or Xilinx had been advised of the possibility of the same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any 
application requiring fail-safe performance, such as life-support or safety 
devices or systems, Class III medical devices, nuclear facilities, applications 
related to the deployment of airbags, or any other applications that could lead 
to death, personal injury, or severe property or environmental damage 
(individually and collectively, "Critical Applications"). Customer asresultes the 
sole risk and liability of any use of Xilinx products in Critical Applications, 
subject only to applicable laws and regulations governing limitations on product 
liability. 

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT 
ALL TIMES.

*******************************************************************************/
#include <stdio.h>

#include "adders.h"
 
int main()
{
	int inA, inB, outD, outE[4], inoutF  ;
	int sum = 0;
	// For adders
	int refOut[5] = {31, 41, 51, 61, 71};
	int pass;
	int i;

	inA = 10;
	inB = 20;
	int inC[4] = {1,2,3,4};
    inoutF = 30;
    int inoutG[4] = {5,6,7,8};
	
	// Call the adder for 5 transactions
	for (i=0; i<5; i++)
	{
		sum = adders(inA, &inB, inC, &outD, outE, &inoutF, inoutG);
		
	  // Test the output against expected results
		if (sum == refOut[i])
			pass = 1;
		else 
			pass = 0;
			
		inA=inA+10;
	}

	if (pass)
	{
		fprintf(stdout, "----------Pass!------------\n");
		return 0;
	}
	else
	{
		fprintf(stderr, "----------Fail!------------\n");
		return 0;
	}
}
