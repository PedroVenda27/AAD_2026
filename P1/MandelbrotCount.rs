//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// Rust code
//
// Mandelbrot set
// * iterate f(z) = z * z + c starting at z = 0 for a given complex number c
// * stop iterating when either
//   * the number of iterates reaches a pre-specified limit (256)
//   or
//   * the absolute value of the iterate is larger that 2 (divergence!)
// * do this for 1025 values in each axis, such that
//   * -2.05 <= real(c) <= 0.55
//   * -1.3 <= imag(c) <= 1.3
// Instead of generating an image of the Mandelbrot set we will only
// * count the number of values of c for which the iterates do not reach the iteration limit
//

use std::time::Instant;

fn main()
{
  let n_samples = 2049;
  let c_re_min = -2.05;
  let c_re_max =  0.55;
  let c_im_min = -1.30;
  let c_im_max =  1.30;
  let mut count = 0;
  let start = Instant::now();
  for re_idx in 0..n_samples
  {
    let c_re = c_re_min + (re_idx as f64) * ((c_re_max - c_re_min) / (n_samples as f64));
    for im_idx in 0..n_samples
    {
      let c_im = c_im_min + (im_idx as f64) * ((c_im_max - c_im_min) / (n_samples as f64));
      let mut n_iter = 0;
      let mut z_re = 0.0;
      let mut z_im = 0.0;
      while n_iter < 256 && z_re * z_re + z_im * z_im <= 4.0
      {
        let tmp = z_re * z_re - z_im * z_im;
        z_im = 2.0 * z_re * z_im + c_im;
        z_re = tmp + c_re;
        n_iter += 1;
      }
      if n_iter < 256
      {
        count += 1;
      }
    }
  }
  let duration = start.elapsed();
  println!("{} -- {:7.3}",count,duration.as_secs_f64());
}
