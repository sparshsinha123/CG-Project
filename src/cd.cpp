#include <GL/glut.h>
#include <bits/stdc++.h>
#include <torch/torch.h>
using namespace std;

#define WINDOW_HEIGHT 800
#define WINDOW_WIDTH 800
#define PI acos(-1)

const GLint MAX_NUMBER_OF_OBJECTS = 1000;
std::chrono::steady_clock::time_point t; // keeps track of the current time

/*  data structures to store velocities , masses , locations of the particles */

/* creating a tensor for velocities */
torch::Tensor velocity;                /* 2 x 1 x n tensor of velocity of objects */
torch::Tensor centres;                 /* 2 x 1 x n tensor of centres of objects */
torch::Tensor radii;                   /* radius of the tensors */
torch::Tensor mask;                    /* global null tensor */
torch::Tensor previousCollisionMatrix; /* boolean matrix for checking collision*/
torch::Tensor previousleftWallCollisionMatrix;
torch::Tensor previousrightWallCollisionMatrix;
torch::Tensor previoustopWallCollisionMatrix;
torch::Tensor previousbottomWallCollisionMatrix;

/* since these quantities don't change they are vectors*/

bool delayedStart = true;
static GLint numObjects = 0;
GLint cr = 1, cg = 0, cb = 0;

void checkCollisionWithWalls(GLint i)
{
  /* checks whether the ith object has collided with one of the walls and 
  updates the velocities of the particles then and there */
  /* first get the centre of the circle and its radii*/
  GLdouble centrex = centres[0][0][i].item<GLdouble>();
  GLdouble centrey = centres[1][0][i].item<GLdouble>();
  GLdouble rad = radii[0][i].item<GLdouble>();
  // apply conditions for collision with all the walls and update the signs of the velocities
  // the collision is assumed to be elastic
  if (centrex - rad <= 0)
    velocity[0][0][i] = abs(velocity[0][0][i]);
  if (centrex + rad >= WINDOW_WIDTH)
    velocity[0][0][i] = -abs(velocity[0][0][i]);
  if (centrey - rad <= 0)
    velocity[1][0][i] = abs(velocity[1][0][i]);
  if (centrey + rad >= WINDOW_HEIGHT)
    velocity[1][0][i] = -abs(velocity[1][0][i]);
}
void reCalculatePositions()
{
  /* check the collision of the objects with the walls one by one*/
  // for (int i = 0; i < numObjects; i++){
  //  checkCollisionWithWalls(i);
  // }

  torch::Tensor leftWallCollisionMatrix = ((centres[0] - radii) <= 0);
  torch::Tensor haveCollided = ((leftWallCollisionMatrix & ~previousleftWallCollisionMatrix));
  velocity[0] = velocity[0] * ~haveCollided + abs(velocity[0]) * haveCollided;
  previousleftWallCollisionMatrix = leftWallCollisionMatrix;

  torch::Tensor rightWallCollisionMatrix = ((centres[0] + radii) >= WINDOW_WIDTH);
  haveCollided = ((rightWallCollisionMatrix & ~previousrightWallCollisionMatrix));
  velocity[0] = velocity[0] * ~haveCollided - abs(velocity[0]) * haveCollided;
  previousrightWallCollisionMatrix = rightWallCollisionMatrix;

  torch::Tensor topWallCollisionMatrix = ((centres[1] - radii) <= 0);
  haveCollided = ((topWallCollisionMatrix & ~previoustopWallCollisionMatrix));
  velocity[1] = velocity[1] * ~haveCollided + abs(velocity[1]) * haveCollided;
  previoustopWallCollisionMatrix = topWallCollisionMatrix;

  torch::Tensor bottomWallCollisionMatrix = ((centres[1] + radii) >= WINDOW_HEIGHT);
  haveCollided = ((bottomWallCollisionMatrix & ~previousbottomWallCollisionMatrix));
  velocity[1] = velocity[1] * ~haveCollided - abs(velocity[1]) * haveCollided;
  previousbottomWallCollisionMatrix = bottomWallCollisionMatrix;

  /* use tensors to check the collision between all the objects */
  torch::Tensor distances = centres - torch::transpose(centres, 2, 1);
  distances = torch::sqrt((distances * distances).sum(0));

  torch::Tensor pairWiseMinDistance = torch::transpose(radii, 0, 1) + radii;
  torch::Tensor currentCollisionMatrix = ((distances - pairWiseMinDistance) <= 0);
  haveCollided = ((currentCollisionMatrix & ~previousCollisionMatrix));
  /* apply the conservation laws upon collision */
  /* we know that the ditance between the particles is pairWiseMin for the case of collision */
  torch::Tensor commonNormals = (-(torch::transpose(centres, 2, 1)) + centres);
  commonNormals = (commonNormals) / pairWiseMinDistance;

  commonNormals *= mask;

  /* get the relative velocities of the bodies . Note that the following tensor has two planes */
  torch::Tensor relativeVel = (-(torch::transpose(velocity, 2, 1)) + velocity);
  /* make the final update to the velocity. The updates need to be made only when
  the collision has occured */
  const GLdouble e = 0.0;
  torch::Tensor updater = ((1 + e) / 2) * (haveCollided * (((relativeVel * commonNormals).sum(0)) * (commonNormals)));
  // the i,j th entry of the update contains information for updating the velocity of the particle i
  updater = updater.nan_to_num(0);

  torch::Tensor changeInVel = torch::transpose(updater.sum(2, 2), 1, 2);
  velocity += changeInVel;

  cout << "magnitude of velocity of the first particle ";
  cout << (torch::sqrt((velocity * velocity).sum(0)))[0][0] << endl;

  std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
  GLint timeDelta = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - t).count();
  if (delayedStart == true && timeDelta <= 2 * 1e9)
  {
    glutPostRedisplay();
    return;
  }

  delayedStart = false;
  t = currentTime;

  /* updating the position and velocity tensor */
  centres = centres + velocity * (timeDelta * 1e-9);
  previousCollisionMatrix = currentCollisionMatrix;
  glutPostRedisplay();
}

void drawCircle(GLdouble radius, GLdouble centreX, GLdouble centreY, GLint cred, GLint cblue, GLint cgreen)
{
  GLdouble theta;
  glColor3f(cred, cgreen, cblue);
  glBegin(GL_POLYGON);
  for (int i = 0; i < 360; i++)
  {
    theta = 1.00 * i * PI / 180;
    GLdouble x = centreX + radius * cos(theta);
    GLdouble y = centreY + radius * sin(theta);
    glVertex3f(x, y, 0);
  }
  glEnd();
}

void display()
{

  static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();

  GLint timeDelta = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - previousTime).count();

  if (timeDelta <= (20 * 1e6))
  {
    return;
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, WINDOW_HEIGHT, WINDOW_WIDTH, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  for (int i = 0; i < numObjects; i++)
  {
    drawCircle(radii[0][i].item<GLdouble>(), centres[0][0][i].item<GLdouble>(), centres[1][0][i].item<GLdouble>(), (i == 0 ? 1 : 0), (i == 0 ? 0 : 1), (i == 0 ? 0 : 0));
  }
  glFlush();
  glutSwapBuffers();
  previousTime = currentTime;
}

void drawObject(GLdouble centreX, GLdouble centreY, GLdouble velocityX, GLdouble velocityY, GLdouble rs)
{
  /* All the different circles of the screen are drawn initially */
  for (int r = 3; r <= 50; r += 15)
  {
    for (int i = 0; i <= 360; i += 30)
    {
      GLdouble theta = 1.00 * i * PI / 180;
      GLdouble xc = centreX + r * cos(theta), yc = centreY + r * sin(theta);
      if (numObjects >= MAX_NUMBER_OF_OBJECTS)
      {
        cout << "limit of number of objects is set to " << MAX_NUMBER_OF_OBJECTS << endl;
        cout << "terminating program " << endl;
        exit(1);
      }
      centres[0][0][numObjects] = xc;
      centres[1][0][numObjects] = yc;
      radii[0][numObjects] = rs;
      velocity[0][0][numObjects] = velocityX;
      velocity[1][0][numObjects] = velocityY;
      numObjects++;
    }
  }
}

void reshape(GLsizei width, GLsizei height)
{
  if (height == 0)
    height = 1;
  GLfloat aspect = (GLfloat)width / (GLfloat)height;

  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(45.0f, aspect, 0.1f, 100.0f);
}

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  /* initialize the tensors */
  velocity = torch::zeros({2, 1, MAX_NUMBER_OF_OBJECTS});
  centres = torch::zeros({2, 1, MAX_NUMBER_OF_OBJECTS});
  radii = torch::zeros({1, MAX_NUMBER_OF_OBJECTS});
  mask = torch::ones({MAX_NUMBER_OF_OBJECTS, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool));
  previousCollisionMatrix = torch::zeros({MAX_NUMBER_OF_OBJECTS, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool));
  previousleftWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool));
  previousrightWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool));
  ;
  previoustopWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool));
  previousbottomWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool));

  for (int i = 0; i < MAX_NUMBER_OF_OBJECTS; i++)
  {
    for (int j = 0; j < MAX_NUMBER_OF_OBJECTS; j++)
    {
      if (i > j)
        mask[i][j] = -1;
    }
  }
  /* draw the objects on to the screen and initialize matrices */
  drawObject(200, 200, 50, 0, 5);
  drawObject(700, 200, -50, 0, 5);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  glutCreateWindow("SIMULATION");
  glEnable(GL_DEPTH_TEST);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(reCalculatePositions);
  glutMainLoop();
  return 0;
}