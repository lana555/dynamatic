
int  phi_test ( int Max[16] )
{

    int    maxIndex;
    int  maxValue;

    maxValue = Max[0];
    maxIndex = 0;
    for (int i = 0; i < 16; i++) {
        if (Max[i] > maxValue)
        {
            maxValue = Max[i];
            maxIndex = i;
        }
    }


    return maxIndex;
}


int main ()
{
    int Input[16];

  phi_test ( Input );

  return 0;

}

