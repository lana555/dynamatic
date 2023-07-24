
int  phi_test ( int Max[16] )
{

    int    maxIndex;
    int  maxValue;

    maxValue = Max[0];
    maxIndex = 2;
    int x = Max[15];
    int i=0;
    while (i<16) {
        Max[i] = x;
        x=i;
        i++;
        if (Max[i] > x)
        {
            maxValue = Max[i];
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

