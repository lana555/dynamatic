

void sobel(int indata[256], int outdata[256], int GX[9], int GY[9]) {
    unsigned int X, Y;
    /*---------------------------------------------------
         SOBEL ALGORITHM STARTS HERE
    ---------------------------------------------------*/
    for (Y = 1; Y < 15; Y++) {
        for (X = 1; X < 15; X++) {
            long sumX = 0;
            long sumY = 0;
            int SUM   = 0;
            int t1, t2, c1, c2, c3;

            /* image boundaries */
            t1 = Y == 0;
            t2 = Y == 15;
            c1 = t1 || t2;
            c1 = !c1;

            t1 = X == 0;
            t2 = X == 15;
            c2 = t1 || t2;
            c2 = !c2;

            c3 = c1 && c2;

            if (c3)
            /* Convolution starts here */
            {
                int I, J;

                /*-------X GRADIENT APPROXIMATION------*/
                for (I = -1; I <= 1; I++) {
                    for (J = -1; J <= 1; J++) {
                        sumX = sumX + (int)(indata[X + I + (Y + J) * 16] * GX[3 * I + J + 4]);
                        sumY = sumY + (int)(indata[X + I + (Y + J) * 16] * GY[3 * I + J + 4]);
                    }
                }
                if (sumX > 255)
                    sumX = 255;
                if (sumX < 0)
                    sumX = 0;

                /*-------Y GRADIENT APPROXIMATION-------*/
                if (sumY > 255)
                    sumY = 255;
                if (sumY < 0)
                    sumY = 0;

                SUM = sumX + sumY; /*---GRADIENT MAGNITUDE APPROXIMATION (Myler p.218)----*/
            }
            //         printf("y=%d ; x=%d --> SUM = %d", Y,X,SUM);
            outdata[X + Y * 16] =
                255 - (unsigned char)(SUM); /* make edges black and background white */
            //         printf(" --> outdata[%d] = %d\n", X + Y*incols, outdata[X + Y*incols]);
        }
    }
}
