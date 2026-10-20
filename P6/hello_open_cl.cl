//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// the OpenCL kernel initializes an array with the string "Hello, world!"
// each "thread" initializes only one array position
//

__kernel
//__attribute__((work_group_size_hint(128,1,1)))
void hello_kernel(__global char *buffer,int buffer_size)
{
  size_t idx;

  idx = get_global_id(0);                           // our kernel only has one dimension, so this is enough to identify the thread
  if(idx >= (size_t)0 && idx < (size_t)buffer_size) // defensive programming: make sure the index we will use is a valid index
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
