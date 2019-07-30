int arr1[5] = {0, 1, 2, 3, 4};
int arr2[6] = {16, 17, 18, 19, 20, 21};

int func() {
  int sum = 0;
  for (int i = 0; i < 5; i++)
    sum += arr1[i] * i;
  for (int i = 0; i < 6; i++)
    sum += arr2[i] * i;
  return sum;
}