#include <GL/glut.h>
#include <bits/stdc++.h>
#include <torch/torch.h>

using namespace std;


int main(){
  torch::Tensor tensor = torch::rand({2, 3});
  std::cout << tensor << std::endl;

  vector<int> v = {2 , 4 , 5};

  cout << "created a new tensor from a vector " << endl;
  torch::Tensor te = torch::zeros({1 , 3});
  for(int i = 0; i < 3; i++){
    te[0][i] += v[i];
  }
  cout << te << endl;

  cout << "addition of two tensor " << endl;
  // addition of two tensors
  torch::Tensor te1 = torch::ones({2 , 4});
  torch::Tensor te2 = torch::ones({2 , 4});
  torch::Tensor tex = te1 + te2;
  cout << tex << endl;

  // reciprocal of a tensor 
  cout << "reciprocal of tensor " << endl;
  cout << 1 / tex << endl;

  // scalar subtraction

  cout << "subtracting the entire tensor from 1 "  << endl;
  cout <<  (1 - tex) << endl;

  // modifying one row of the tensor
  tex[0] = tex[0] - 1;
  cout << "reducing one row of the tensor" << endl;
  cout << tex << endl;


  // fetching one row of the tensor
  cout << "output of the 1st row of the tensor" << endl;
  cout << tex[1] << endl;

  // taking the transpose of the tensor
  cout << "transpose of the tensor is " << endl;
  torch::Tensor tr = torch::transpose(tex , 1, 0);
  cout << tex << endl;
  cout << tr << endl;


  // creating a nxnx2 tensor
  cout << " nx n x 2 tensor" << endl;
  torch::Tensor gd = torch::rand({2 , 4 , 2});
  cout << " first plane " << endl;
  cout << gd[0] << endl;

  cout << " second plane " << endl;
  cout << gd[1] << endl;

  cout << "complete tensor " << endl;
  cout << gd << endl;


  // square root of a tensor 
  cout << "the square root of the tensor above " << endl;
  cout << torch::sqrt(gd) << endl;

  torch::Tensor t1 = torch::ones({2 , 3});
  torch::Tensor t2 = torch::ones({2 , 3});
 // cout << t1.dot(t2) << endl;
  cout << "simple dot product of matrices " << endl;
  cout << t1 * t2 << endl;
  cout << "matrix mult " << endl;
  torch::Tensor g1 = torch::ones({2 , 3});
  torch::Tensor g2 = torch::ones({3 , 2});
  cout << "first matrix " << endl;
  cout << g1 << endl;
  cout << "second matrix " << endl;
  cout << g2 << endl;
  cout << " multiplication of matrices are  " << endl;
  cout << torch::mm(g1 , g2) << endl;
}