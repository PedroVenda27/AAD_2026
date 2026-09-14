const std = @import("std");

pub fn main() !void {
  const n_samples: i32 = 2049;
  const c_re_min: f64 = -2.05;
  const c_re_max: f64 =  0.55;
  const c_im_min: f64 = -1.30;
  const c_im_max: f64 =  1.30;
  var count: i32 = 0;

  var timer = try std.time.Timer.start();

  var re_idx: i32 = 0;
  while (re_idx < n_samples) : (re_idx += 1) {
    const c_re = c_re_min + @as(f64, @floatFromInt(re_idx)) * ((c_re_max - c_re_min) / @as(f64, @floatFromInt(n_samples)));
    var im_idx: i32 = 0;
    while (im_idx < n_samples) : (im_idx += 1) {
      const c_im = c_im_min + @as(f64, @floatFromInt(im_idx)) * ((c_im_max - c_im_min) / @as(f64, @floatFromInt(n_samples)));
      var n_iter: i32 = 0;
      var z_re: f64 = 0.0;
      var z_im: f64 = 0.0;
      while (n_iter < 256 and z_re * z_re + z_im * z_im <= 4.0) {
          const tmp = z_re * z_re - z_im * z_im;
          z_im = 2.0 * z_re * z_im + c_im;
          z_re = tmp + c_re;
          n_iter += 1;
      }
      if (n_iter < 256) {
          count += 1;
      }
    }
  }

  const elapsed_ns = timer.read();
  const elapsed_sec = @as(f64, @floatFromInt(elapsed_ns)) / 1_000_000_000.0;

  std.debug.print("{} -- {d:7.3}\n", .{ count, elapsed_sec });
}
