Dependencies:
1) Polly - For polyhedral analysis
2) ISL - Because Polly bundles an ancient version

To resolve/bugs:
1) Get parameters from Scop - Can there be input dims in the isl_map that do not correspond to loop parameters?
2) SCoPs immediately following another SCoP are merged. Introduces the chance of false positives
    Eg.
    for(1 to n)
        sum += a[i];

    for(1 to n)
        a[i]  = sum;

3) Replicate divisibility constraints