//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// the CUDA kernel initializes an array with the string "Hello, world!"
// each "thread" initializes only one array position
//

extern "C" __global__
//__launch_bounds__(128,1)
void hello_kernel(char *buffer,int buffer_size)
{
  int idx;

  idx = (int)threadIdx.x + (int)blockDim.x * (int)blockIdx.x; // global thread number
  if(idx >= 0 && idx < buffer_size)                           // defensive programming: make sure the index we will use is a valid index
    switch(idx)
    {
      case  0: buffer[idx] = 'H';  break;
      case  1: buffer[idx] = 'e';  break;
      case  2: buffer[idx] = 'l';  break;
      case  3: buffer[idx] = 'l';  break;
      case  4: buffer[idx] = 'o';  break;
      case  5: buffer[idx] = ',';  break;
      case  6: buffer[idx] = ' ';  break;
      case  7: buffer[idx] = 'W';  break;
      case  8: buffer[idx] = 'o';  break;
      case  9: buffer[idx] = 'r';  break;
      case 10: buffer[idx] = 'l';  break;
      case 11: buffer[idx] = 'd';  break;
      case 12: buffer[idx] = '!';  break;
      case 13: buffer[idx] = '\0'; break;
      default: buffer[idx] = 'X';  break;
    }
}
