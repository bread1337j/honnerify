### Honnerifier 

A program similar to Spu7nix's obamify, made for a linear algebra final project.

### Compilation: 

You will need the CUDA SDK and GCC installed. Then you can simply run make compile. 

### Usage: 

honnerify [OPTIONS] 

- -t [FILENAME] | target image
- -s [FILENAME] | source image
- -i [NUMBER] | Max number of iterations, default 100
- -r [NUMBER] | Regularization constant, default 0.1
- -c | Use CUDA runtime 
- -g | Create a GIF
- -R | Recursive mode, an output image is created on every iteration and is then used as the new source image. 

### How it works:

The program uses the Sinkhorn-knopp algorithm for optimal transport. 

We start off by creating a cost matrix C. This program uses eucledian distance to determine the cost. Then, we create a gibbs kernel by the following formula:

K_ij = e^(-C_ij / reg) 

Note that these matrices are too large to store in memory for bigger images, so we instead compute the values as we need them.

Then we convert the input and output images to stochastic vectors based on brightness, we will denote these vectors as a and b. 

We will then create two 1s vectors, u and v. 

The sinkhorn theorem states that any positive matrix (such as our K) has two diagonal matrices diag(u) and diag(v) for which diag(u)Kdiag(v) will be doubly stochastic. The sinkhorn algorithm is to then continuously scale the values of u and v to be stochastic in either rows or columns until it converges on both.

For optimal transport, instead of scaling u and v to be doubly stochastic, we scale u and v to produce the supply and demand vectors. 

The formula is roughly this: 

v = b / (Ku)

u = a / (K^Tv)

Note that division is defined to be element-wise and K is symmetric, thus K=K^T

Consult the actual code in sinkhorn.c and multiprocessing.cu for more details. 

After a bunch of iterations, we will create a transport plan P = diag(u)Kdiag(v) 

Then, we can divide each value in P by the corresponding value of the supply vector and use it as a traversible graph. The end result will be the optimal transport of the source image into the output. 

