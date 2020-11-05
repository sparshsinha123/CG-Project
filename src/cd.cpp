#include <GL/glut.h>
#include <bits/stdc++.h>
#include <torch/torch.h>
#include <ATen/ATen.h>

using namespace std;

#define WINDOW_HEIGHT 1200
#define WINDOW_WIDTH 1200
#define PI acos(-1)
#define EPSILON 0.1
static GLint MAX_NUMBER_OF_OBJECTS = 600;
std::chrono::steady_clock::time_point t; // keeps track of the current time

/*  data structures to store velocities , masses , locations of the particles */

/* creating a tensor for velocities */
const GLdouble mass = 200;             /*mass of the particles -- assumed to be the same*/
torch::Tensor velocity;                /* 2 x 1 x n tensor of velocity of objects */
torch::Tensor centres;                 /* 2 x 1 x n tensor of centres of objects */
torch::Tensor radii;                   /* radius of the particles */
torch::Tensor acceleration;            /* acceleration of the particles */
torch::Tensor mask;                    /* global null tensor */
torch::Tensor previousCollisionMatrix; /* boolean matrix for checking collision*/
torch::Tensor previousleftWallCollisionMatrix;
torch::Tensor previousrightWallCollisionMatrix;
torch::Tensor previoustopWallCollisionMatrix;
torch::Tensor previousbottomWallCollisionMatrix;
torch::Tensor haveCollided;
torch::Tensor LIMIT;
torch::Tensor forceMask;
torch::Tensor distances;
torch::Tensor unitNormals;
torch::Tensor commonNormals;

/* since these quantities don't change they are vectors*/

bool delayedStart = false;
static GLint numObjects = 0;
GLint cr = 1, cg = 0, cb = 0;
GLdouble radius = 5;

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
  if (centrex - rad <= EPSILON)
    velocity[0][0][i] = abs(velocity[0][0][i]);
  if (centrex + rad >= WINDOW_WIDTH - EPSILON)
    velocity[0][0][i] = -abs(velocity[0][0][i]);
  if (centrey - rad <= EPSILON)
    velocity[1][0][i] = abs(velocity[1][0][i]);
  if (centrey + rad >= WINDOW_HEIGHT - EPSILON)
    velocity[1][0][i] = -abs(velocity[1][0][i]);
}

void calculateAccelerations()
{
  /* calculate the acceleration of the particles using spring force model */
  const GLdouble unDeformedSpringLen = 0;      /*the undeformed length of the spring in this model*/
  const GLdouble springForceRange = 200;       /*the range of the acting spring force*/
  const GLdouble springConstant = 1.34 * 1e-1; /* the constant K in the spring force expression */
  /* get the distances between the particles */
  //torch::Tensor distances = centres - torch::transpose(centres, 2, 1);
  //distances = torch::sqrt((distances * distances).sum(0));
  /* i - j th entries is the unit normal from the centre of the ith particle to the centre 
  of the other particles */
  //torch::Tensor unitNormals = (-torch::transpose(centres, 2 , 1) + centres) / distances;
  //unitNormals.nan_to_num_(EPSILON * 1e-1);
  /*formula for the spring force
    --> magnitude of the force : k * (distance - undeformed len)
  */
  torch::Tensor magnitudeOfForce = springConstant * (distances - unDeformedSpringLen);
  /* apply the force only if the particles are at distances less than the range of the
  spring force */
  magnitudeOfForce = magnitudeOfForce * (distances <= springForceRange);
  torch::Tensor force = magnitudeOfForce * unitNormals;
  force = force * forceMask;

  /* calculate the sum of all the forces acting on the particle due to the other particles */
  acceleration = torch::transpose(force.sum(2, true), 2, 1) / mass;
  //acceleration.nan_to_num_(0);
}

void display_frame()
{
  static int frame_count = 0;
  static long long time_track = 0;
  double fps = 60.0;
  static std::chrono::steady_clock::time_point prevTime;

  std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
  GLint timeDelta = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - prevTime).count();

  // if (delayedStart == true && timeDelta >= 2 * 1e9)
  // {
  //   glutPostRedisplay();
  //   return;
  // }

  if (timeDelta > (1 / fps) * 1e9)
  {
    glutPostRedisplay();
    frame_count++;
    time_track += timeDelta;
    if (time_track > 5 * 1e9)
    {
      cout << 1e9 * frame_count / (time_track) << " FPS\n";
      frame_count = 0;
      time_track = 0;
    }
    //delayedStart = false;
    prevTime = currentTime;
  }
}

void reCalculatePositions()
{
  /* check the collision of the objects with the walls one by one*/
  // for (int i = 0; i < numObjects; i++){
  //  checkCollisionWithWalls(i);
  // }

  torch::Tensor leftWallCollisionMatrix = ((centres[0] - radii) <= 0);
  haveCollided = ((leftWallCollisionMatrix & ~previousleftWallCollisionMatrix));
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
  distances = centres - torch::transpose(centres, 2, 1);
  distances = torch::sqrt((distances * distances).sum(0));

  torch::Tensor pairWiseMinDistance = torch::transpose(radii, 0, 1) + radii;
  torch::Tensor currentCollisionMatrix = ((distances - pairWiseMinDistance) <= EPSILON);
  /* apply the conservation laws upon collision */
  /* we know that the ditance between the particles is pairWiseMin for the case of collision */
  distances = max(distances, pairWiseMinDistance);
  unitNormals = (-(torch::transpose(centres, 2, 1)) + centres);
  unitNormals = (unitNormals) / distances;
  //commonNormals.nan_to_num_(0);

  commonNormals = unitNormals * mask;

  /* get the relative velocities of the bodies . Note that the following tensor has two planes */
  torch::Tensor relativeVel = (-(torch::transpose(velocity, 2, 1)) + velocity);
  /* make the final update to the velocity. The updates need to be made only when
  the collision has occured */

  /* calculating have collided matrix carefully */
  /*velocity of separation is the component of the relative velocity of the particle along
  the common normal */
  torch::Tensor velocityOfSepartion = ((relativeVel * commonNormals).sum(0));
  torch::Tensor headingTowardsEachOther = (velocityOfSepartion < 0);
  /* collision is assumed to occur if the distance between particles is less than the 
  desired minimum and the particles are heading towards each other s*/
  haveCollided = (currentCollisionMatrix & headingTowardsEachOther);

  const GLdouble e = 0.3;
  torch::Tensor updater = ((1 + e) / 2) * (haveCollided * (((relativeVel * commonNormals).sum(0)) * (commonNormals)));
  // the i,j th entry of the update contains information for updating the velocity of the particle i
  //updater = updater.nan_to_num(0);

  torch::Tensor changeInVel = torch::transpose(updater.sum(2, 2), 1, 2);
  //changeInVel.nan_to_num_(0);

  /* to avoid strong impulses due to boundaries , dummy particles or overflows in numbers 
  or division by zeroes */
  changeInVel = torch::min(changeInVel, LIMIT);
  changeInVel = torch::max(changeInVel, -LIMIT);
  velocity += changeInVel;

  std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
  GLint timeDelta = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - t).count();

  /* updating the position and velocity tensor */
  calculateAccelerations();
  velocity = velocity + acceleration * (timeDelta * 1e-9);
  //velocity.nan_to_num_(0);

  centres = centres + velocity * (timeDelta * 1e-9);
  previousCollisionMatrix = currentCollisionMatrix;

  display_frame();
  t = currentTime;
}

void drawCircle(GLdouble radius, GLdouble centreX, GLdouble centreY, GLfloat cred, GLfloat cblue, GLfloat cgreen)
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
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, WINDOW_HEIGHT, WINDOW_WIDTH, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.149, 0.29, 0.792, 1.0);

  glEnable(GL_DEPTH_TEST);
  for (int i = 0; i < numObjects / 2; i++)
  {
    drawCircle(radii[0][i].item<GLdouble>(), centres[0][0][i].item<GLdouble>(), centres[1][0][i].item<GLdouble>(), 0.886, 0.98, 0.906);
  }
  for (int i = numObjects / 2; i < numObjects; i++)
  {
    drawCircle(radii[0][i].item<GLdouble>(), centres[0][0][i].item<GLdouble>(), centres[1][0][i].item<GLdouble>(), 1, 0.0, 0.867);
  }
  glFlush();
  glutSwapBuffers();
}

void drawObject(GLdouble centreX, GLdouble centreY, GLdouble velocityX, GLdouble velocityY, GLdouble rs, int start, int end)
{
  int drawn_objects = start;
  /* All the different circles of the screen are drawn initially */
  for (double r = rs; drawn_objects <= end; r += 2 * rs)
  {
    double delta = 4 * asin(rs / (2.00 * r)) * (180 / PI);
    for (double i = 0; i + delta <= 360; i += delta)
    {
      GLdouble theta = 1.00 * i * PI / 180;
      GLdouble xc = centreX + r * cos(theta), yc = centreY + r * sin(theta);
      if (drawn_objects > end)
      {
        break;
      }
      centres[0][0][numObjects] = xc;
      centres[1][0][numObjects] = yc;
      radii[0][numObjects] = rs;
      velocity[0][0][numObjects] = velocityX;
      velocity[1][0][numObjects] = velocityY;
      numObjects++;
      drawn_objects++;
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
  auto dev = torch::kCPU;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  /* initialize the tensors */
  velocity = torch::zeros({2, 1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().device(dev, 0));
  centres = torch::zeros({2, 1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().device(dev, 0));
  radii = torch::ones({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().device(dev, 0));
  acceleration = torch::zeros({2, 1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().device(dev, 0));
  LIMIT = torch::zeros({2, 1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().device(dev, 0));
  mask = torch::ones({MAX_NUMBER_OF_OBJECTS, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool).device(dev, 0));
  previousCollisionMatrix = torch::zeros({MAX_NUMBER_OF_OBJECTS, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool).device(dev, 0));
  previousleftWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool).device(dev, 0));
  previousrightWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool).device(dev, 0));
  previoustopWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool).device(dev, 0));
  previousbottomWallCollisionMatrix = torch::zeros({1, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().dtype(torch::kBool).device(dev, 0));
  forceMask = torch::zeros({MAX_NUMBER_OF_OBJECTS, MAX_NUMBER_OF_OBJECTS}, torch::TensorOptions().device(dev, 0));
  for (int i = 0; i < MAX_NUMBER_OF_OBJECTS; i++)
  {
    for (int j = 0; j < MAX_NUMBER_OF_OBJECTS; j++)
    {
      if ((i < MAX_NUMBER_OF_OBJECTS / 2 && j < MAX_NUMBER_OF_OBJECTS / 2) || (i >= MAX_NUMBER_OF_OBJECTS / 2 && j >= MAX_NUMBER_OF_OBJECTS / 2))
        forceMask[i][j] = 1;
      else
        forceMask[i][j] = -1;
    }
  }

  /* draw the objects on to the screen and initialize matrices */
  drawObject(1000, 500, -100, 0, radius, 1, MAX_NUMBER_OF_OBJECTS / 2);
  drawObject(100, 500, 100, 0, radius, MAX_NUMBER_OF_OBJECTS / 2 + 1, MAX_NUMBER_OF_OBJECTS);

  cout << "number of particles " << endl;
  cout << numObjects << endl;

  for (int i = 0; i < MAX_NUMBER_OF_OBJECTS; i++)
  {
    LIMIT[0][0][i] = LIMIT[1][0][i] = 55.5;
    for (int j = 0; j < MAX_NUMBER_OF_OBJECTS; j++)
    {
      if (i > j)
        mask[i][j] = -1;
    }
  }
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