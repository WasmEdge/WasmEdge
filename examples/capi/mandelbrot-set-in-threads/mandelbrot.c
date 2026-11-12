#define WIDTH 1200
#define HEIGHT 800
#define ImageSize (WIDTH * HEIGHT * 4)
unsigned char Image[ImageSize];

unsigned char colour(int Iteration, int Offset, int Scale) {
  Iteration = ((Iteration * Scale) + Offset) % 1024;
  if (Iteration < 256) {
    return Iteration;
  }
  if (Iteration < 512) {
    return 255 - (Iteration - 255);
  }
  return 0;
}

int iterateEquation(double X0, double Y0, int Maxiterations) {
  double A = 0.0, B = 0.0, Rx = 0.0, Ry = 0.0;
  int Iterations = 0;
  while (Iterations < Maxiterations && (Rx * Rx + Ry * Ry <= 4.0)) {
    Rx = A * A - B * B + X0;
    Ry = 2.0 * A * B + Y0;
    A = Rx;
    B = Ry;
    Iterations++;
  }
  return Iterations;
}

double scale(double DomainStart, double DomainLength, int ScreenLength,
             int Step) {
  return DomainStart +
         DomainLength * ((double)(Step - ScreenLength) / (double)ScreenLength);
}

void mandelbrot(int MaxIterations, double Cx, double Cy, double Diameter) {
  double VerticalDiameter = Diameter * HEIGHT / WIDTH;
  for (int Y = 0; Y < HEIGHT; Y++) {
    for (int X = 0; X < WIDTH; X++) {
      // map to mandelbrot coordinates
      double Rx = scale(Cx, Diameter, WIDTH, X);
      double Ry = scale(Cy, VerticalDiameter, HEIGHT, Y);
      int Iterations = iterateEquation(Rx, Ry, MaxIterations);
      int Idx = ((X + Y * WIDTH) * 4);
      // set the red and alpha components
      Image[Idx] = Iterations == MaxIterations ? 0 : colour(Iterations, 0, 4);
      Image[Idx + 1] =
          Iterations == MaxIterations ? 0 : colour(Iterations, 128, 4);
      Image[Idx + 2] =
          Iterations == MaxIterations ? 0 : colour(Iterations, 356, 4);
      Image[Idx + 3] = 255;
    }
  }
}

void mandelbrotThread(int MaxIterations, int NumThreads, int Rank, double Cx,
                      double Cy, double Diameter) {
  double VerticalDiameter = Diameter * HEIGHT / WIDTH;
  for (int X = 0; X < WIDTH; X++) {
    int YStride =
        (HEIGHT + NumThreads - 1) / NumThreads; // ceil(HEIGHT, num_threads)
    int YOffset = YStride * Rank;
    int YMax = (YOffset + YStride > HEIGHT) ? HEIGHT : YOffset + YStride;
    for (int Y = YOffset; Y < YMax; Y++) {
      // map to mandelbrot coordinates
      double Rx = scale(Cx, Diameter, WIDTH, X);
      double Ry = scale(Cy, VerticalDiameter, HEIGHT, Y);
      int Iterations = iterateEquation(Rx, Ry, MaxIterations);
      int Idx = ((X + Y * WIDTH) * 4);
      // set the red and alpha components
      Image[Idx] = Iterations == MaxIterations ? 0 : colour(Iterations, 0, 4);
      Image[Idx + 1] =
          Iterations == MaxIterations ? 0 : colour(Iterations, 128, 4);
      Image[Idx + 2] =
          Iterations == MaxIterations ? 0 : colour(Iterations, 356, 4);
      Image[Idx + 3] = 255;
    }
  }
}

unsigned char *getImage() { return &Image[0]; }
