float cordic(float theta, float a[2], float cordic_phase[1000], float current_cos,
             float current_sin) {

    float factor = 1.0;

    for (int i = 0; i < 1000; i++) {
        float sigma   = (theta < 0) ? -1.0 : 1.0;
        float tmp_cos = current_cos;

        current_cos = current_cos - current_sin * sigma * factor;
        current_sin = tmp_cos * sigma * factor + current_sin;

        theta  = theta - sigma * cordic_phase[i];
        factor = factor / 2;
    }
    a[0] = current_cos;
    a[1] = current_sin;
    return theta;
}