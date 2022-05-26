const int WIDTH = 1200;
const int HEIGHT = 800;
unsigned char image[WIDTH * HEIGHT * 4];

unsigned char colour(int iteration, int offset, int scale) {
  iteration = ((iteration * scale) + offset) % 1024;
  if (iteration < 256) {
    return iteration;
  } else if (iteration < 512) {
    return 255 - (iteration - 255);
  } else {
    return 0;
  }
}

int iterateEquation(double x0, double y0, int maxiterations) {
  double a = 0.0, b = 0.0, rx = 0.0, ry = 0.0;
  int iterations = 0;
  while (iterations < maxiterations && (rx * rx + ry * ry <= 4.0)) {
    rx = a * a - b * b + x0;
    ry = 2.0 * a * b + y0;
    a = rx;
    b = ry;
    iterations++;
  }
  return iterations;
}

double scale(double domainStart, double domainLength, int screenLength,
             int step) {
  return domainStart +
         domainLength * ((double)(step - screenLength) / (double)screenLength);
}

void mandelbrot(int maxIterations, double cx, double cy, double diameter) {
  double verticalDiameter = diameter * HEIGHT / WIDTH;
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      // map to mandelbrot coordinates
      double rx = scale(cx, diameter, WIDTH, x);
      double ry = scale(cy, verticalDiameter, HEIGHT, y);
      int iterations = iterateEquation(rx, ry, maxIterations);
      int idx = ((x + y * WIDTH) * 4);
      // set the red and alpha components
      image[idx] = iterations == maxIterations ? 0 : colour(iterations, 0, 4);
      image[idx + 1] =
          iterations == maxIterations ? 0 : colour(iterations, 128, 4);
      image[idx + 2] =
          iterations == maxIterations ? 0 : colour(iterations, 356, 4);
      image[idx + 3] = 255;
    }
  }
}

void mandelbrot_thread(int maxIterations, int num_threads, int rank, double cx,
                       double cy, double diameter) {
  double verticalDiameter = diameter * HEIGHT / WIDTH;
  for (int x = 0; x < WIDTH; x++) {
    int y_stride =
        (HEIGHT + num_threads - 1) / num_threads; // ceil(HEIGHT, num_threads)
    int y_offset = y_stride * rank;
    int y_max = (y_offset + y_stride > HEIGHT) ? HEIGHT : y_offset + y_stride;
    for (int y = y_offset; y < y_max; y++) {
      // map to mandelbrot coordinates
      double rx = scale(cx, diameter, WIDTH, x);
      double ry = scale(cy, verticalDiameter, HEIGHT, y);
      int iterations = iterateEquation(rx, ry, maxIterations);
      int idx = ((x + y * WIDTH) * 4);
      // set the red and alpha components
      image[idx] = iterations == maxIterations ? 0 : colour(iterations, 0, 4);
      image[idx + 1] =
          iterations == maxIterations ? 0 : colour(iterations, 128, 4);
      image[idx + 2] =
          iterations == maxIterations ? 0 : colour(iterations, 356, 4);
      image[idx + 3] = 255;
    }
  }
}

unsigned char *getImage() { return &image[0]; }