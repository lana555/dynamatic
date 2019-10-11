

void array_RAM(int c[20], int A[20][20]) {

    for (int j = 1; j <= 18; j++)
    /* loop for the generation of upper triangular matrix*/

    {
        for (int i = j + 1; i <= 18; i++)

        {

            for (int k = 1; k <= 19; k++)

            {

                A[i][k] = A[i][k] - c[j] * A[j][k];
            }
        }
    }
}