#include <GL/glut.h>
#include <bits/stdc++.h>
#include <torch/torch.h>

using namespace std;

#define WINDOW_HEIGHT 800
#define WINDOW_WIDTH 800
#define PI acos(-1)

std::chrono::steady_clock::time_point t; // keeps track of the current time

/*  velocities , masses , locations of the particles */
vector<double> vx;
vector<double> vy;
vector<double> vz;

vector<double> cx;
vector<double> cy;
vector<double> cz;

vector<double> radius;
vector<double> masses;

bool delayedStart = true;
static int numObjects;
int cr = 1, cg = 0 , cb = 0;

/* recalculate the velocities of ith and jth particle once they have collided*/
void reCalculateVelocities(int i , int j){
  // apply the principle of conservation of momentum and energy to calculate the final velcocities
  double vxi_final , vyi_final , vxj_final , vyj_final;
  
  double dist = radius[i] + radius[j];
  double normal_x = (cx[j] - cx[i]) / dist; 
  double normal_y = (cy[j] - cy[i]) / dist;
  // assert (normal_x * normal_x + normal_y * normal_y <= 1);

  double vrel_x = ((vx[j] - vx[i]) * normal_x + (vy[j] - vy[i]) * normal_y) * normal_x;
  double vrel_y = ((vx[j] - vx[i]) * normal_x + (vy[j] - vy[i]) * normal_y) * normal_y;

  vxi_final = vx[i] + (2.0 * masses[j] / (masses[i] + masses[j])) * vrel_x;
  vyi_final = vy[i] + (2.0 * masses[j] / (masses[i] + masses[j])) * vrel_y;
  vxj_final = vx[j] - (2.0 * masses[i] / (masses[i] + masses[j])) * vrel_x;
  vyj_final = vy[j] - (2.0 * masses[i] / (masses[i] + masses[j])) * vrel_y;

  vx[i] = vxi_final , vx[j] = vxj_final;
  vy[i] = vyi_final , vy[j] = vyj_final;
}

/* return the simple spring force between two particles at any time*/
double springForce(int i , int j){
   // returns simple attractive forces between two particles
   double unDeformedSpringLen = 50;
   double thresholdDistance = 20;
   double distance = sqrt((cx[i] - cx[j]) * (cx[i] - cx[j]) + (cy[i] - cy[j])* (cy[i] - cy[j]));
   if (distance >= thresholdDistance) return 0;
   else{
     if (distance == 0) return 0;
     double par = (vx[i] * vx[i] + vy[i] * vy[i]);
     double f = 1.34 * 1e-1  * (distance - unDeformedSpringLen) + 1.46*1e-3*par;
     return f;
   }
}

/* check collision between the ith and the jth particle*/
bool checkCollision(int i , int j){
  double dist = sqrt ( (cx[j] - cx[i]) * (cx[j] - cx[i]) + (cy[j] - cy[i]) * (cy[j] - cy[i]));
  double minClosestDistance = radius[i] + radius[j];
  if (cx[i] - radius[i] <= 0) vx[i] = abs(vx[i]); // particle has gone to the left
  if (cx[i] + radius[i] >= WINDOW_WIDTH) vx[i] = -abs(vx[i]); // particle has gone to the right
 
  if (cx[j] - radius[j] <= 0) vx[j] = abs(vx[j]); // particle has gone to the left
  if (cx[j] + radius[j] >= WINDOW_WIDTH) vx[j] = -abs(vx[j]); // particle has gone to the right

  if (cy[i] - radius[i] <= 0) vy[i] = abs(vy[i]);
  if (cy[i] + radius[i] >= WINDOW_HEIGHT) vy[i] = -abs(vy[i]);  
  if (cy[j] - radius[j] <= 0) vy[j] = abs(vy[j]);
  if (cy[j] + radius[j] >= WINDOW_HEIGHT) vy[j] = -abs(vy[j]); 
 
  double normal_x = (cx[j] - cx[i]) / dist;
  double normal_y = (cy[j] - cy[i]) / dist;

  double signOfVelocityOfSeparation = (vx[i] * normal_x + vy[i] * normal_y) - (vx[j] * normal_x + vy[j] * normal_y);
  if (dist <= minClosestDistance && signOfVelocityOfSeparation > 0) {
    // try to detach the objects in case they have entered themselves
    reCalculateVelocities(i , j);
   }
  return dist <= minClosestDistance;   
}

void reCalculatePositions (){
  for (int i = 0; i < numObjects; i++){
    for (int j = i + 1; j < numObjects; j++){
      if(checkCollision(i , j)){
      }
    }
  }
  std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
  int timeDelta = std::chrono::duration_cast<std::chrono::nanoseconds> (currentTime - t).count();
  if (delayedStart == true && timeDelta <= 2 * 1e9){
    glutPostRedisplay();
    return;
  }
  delayedStart = false;
  t = currentTime;


  
  for(int i = 0; i < numObjects ; i++){
    cx[i] += ((vx[i] * timeDelta) * (1e-9));
    cy[i] += ((vy[i] * timeDelta) * (1e-9));
    
    double fx = 0; double fy = 0;
    for (int otherObject = 0; otherObject < numObjects ; otherObject++) if(otherObject != i){
      double force = springForce(i , otherObject);
      double distance = sqrt((cx[i] - cx[otherObject]) * (cx[i] - cx[otherObject]) + 
                        (cy[i] - cy[otherObject])* (cy[i] - cy[otherObject]));
      double directionX = (cx[otherObject] - cx[i]) / distance;
      double directionY = (cy[otherObject] - cy[i]) / distance;
      fx += directionX * force;
      fy += directionY * force;
    }
    double ax = fx / masses[i] ; double ay = fy / masses[i];
    vx[i] += ((ax * timeDelta) * (1e-9));
    vy[i] += ((ay * timeDelta) * (1e-9));
  }  
  glutPostRedisplay();
}

void drawCircle (double radius , double centreX , double centreY){
  double theta;
  glColor3f(cr, cg, cb);
  glBegin(GL_POLYGON);
  for (int i = 0; i < 360; i++){
    theta = 1.00 * i * PI / 180;
    double x = centreX + radius * cos(theta);
    double y = centreY + radius * sin(theta);
    glVertex3f(x , y,  0);
  }
  glEnd();
}

void display (){
  
  static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
  
  int timeDelta = std::chrono::duration_cast<std::chrono::nanoseconds> (currentTime - previousTime).count();
  
  if (timeDelta <=  (20 * 1e6)){
     return;
  }
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, WINDOW_HEIGHT, WINDOW_WIDTH, 0.0f, 0.0f, 1.0f);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable (GL_DEPTH_TEST);
  for (int i = 0; i < numObjects; i++){
    drawCircle (radius[i] , cx[i] , cy[i]);
  }
  glFlush();
  glutSwapBuffers();

  previousTime = currentTime;
}

void drawObject (double centreX, double centreY , double velocityX, double velocityY){
    for (int r = 10; r <= 50; r += 10){
      for (int i = 0; i < 360; i += 5){
          double theta = 1.00 * i * PI / 180;
          double xc = centreX + r * cos(theta);
          double yc = centreY + r * sin(theta);
          double rs = 5;
          cx.emplace_back(xc); cy.emplace_back(yc);
          radius.emplace_back(rs);
          vx.emplace_back (velocityX);
          vy.emplace_back (velocityY);
          masses.emplace_back(12);
          numObjects++;
      }
    }
}

int main (int argc , char ** argv){
    glutInit (&argc , argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
    numObjects = 0;
    drawObject (200 , 200 , 100 , 100 );
    drawObject (700 , 200  , -100 , 100);

    cout << " number of objects are " << numObjects << endl;
    
    glutInitWindowPosition (0 , 0);
    glutInitWindowSize (WINDOW_WIDTH , WINDOW_HEIGHT);
    glutCreateWindow ("SIMULATION");
    glEnable (GL_DEPTH_TEST);
    glutDisplayFunc (display);
    glutIdleFunc(reCalculatePositions);
    glutMainLoop();
    return 0;
}
