

int newton_raphson(int rts, int x1, int xh) {
    int i = 0;
    int dx;

    while (i < 300) {

        int df = 4 * rts;
        int f  = 2 * rts * rts - 100;
        i++;
        if (f < 0)
            x1 = rts;
        else
            xh = rts;

        if (((rts - x1) * df - f) * ((rts - xh) * df - f) <= 0) {

            dx  = (xh - x1) / 2;
            rts = x1 + dx;
        } else {
            dx = rts / 4;
            rts -= dx;
        }
    }
    return rts;
}
