#include <GL/glut.h>
#include <bits/stdc++.h>

using namespace std;

#define WINDOW_HEIGHT 400
#define WINDOW_WIDTH 400
#define PI acos(-1)

std::chrono::steady_clock::time_point t; // keeps track of the current time


/*  velocities , masses , locations of the particles */
double vx1 = 200, vy1 = 500, vx2 = -500, vy2 = 500;
double cx1 = 180, cy1 = 100, cx2 = 350, cy2 = 350;
double radius1 = 40, radius2 = 40;
double m1 = 300, m2 = 300;

/*whether to delay the start or not*/ 
bool delayedStart = true;

void reCalculateVelocities(){
  // apply the principle of conservation of momentum and energy to calculate the final velcocities
  double vx1_final , vy1_final , vx2_final , vy2_final;
  
  double dist = radius1 + radius2;
  double normal_x = (cx2 - cx1) / dist; 
  double normal_y = (cy2 - cy1) / dist;
  assert (normal_x * normal_x + normal_y * normal_y <= 1);

  double vrel_x = ((vx2 - vx1) * normal_x + (vy2 - vy1) * normal_y) * normal_x;
  double vrel_y = ((vx2 - vx1) * normal_x + (vy2 - vy1) * normal_y) * normal_y;

  vx1_final = vx1 + (2.0 * m2 / (m1 + m2)) * vrel_x;
  vy1_final = vy1 + (2.0 * m2 / (m1 + m2)) * vrel_y;
  vx2_final = vx2 - (2.0 * m1 / (m1 + m2)) * vrel_x;
  vy2_final = vy2 - (2.0 * m1 / (m1 + m2)) * vrel_y;
  

  vx1 = vx1_final , vx2 = vx2_final;
  vy1 = vy1_final , vy2 = vy2_final;
  cout << vx1 << " , " << vy1 << " , " << vx2 << " , " << vy2 << endl;
}

bool checkCollision(){
  double dist = sqrt ( (cx2 - cx1) * (cx2 - cx1) + (cy2 - cy1) * (cy2 - cy1));
  double minClosestDistance = radius1 + radius2;
  if (cx1 - radius1 <= 0) vx1 = abs(vx1); // particle has gone to the left
  if (cx1 + radius1 >= WINDOW_WIDTH) vx1 = -abs(vx1); // particle has gone to the right
  if (cx2 - radius2 <= 0) vx2 = abs(vx2); // particle has gone to the left
  if (cx2 + radius2 >= WINDOW_WIDTH) vx2 = -abs(vx2); // particle has gone to the right

  if (cy1 - radius1 <= 0) vy1 = abs(vy1);
  if (cy1 + radius1 >= WINDOW_HEIGHT) vy1 = -abs(vy1);  
  if (cy2 - radius2 <= 0) vy2 = abs(vy2);
  if (cy2 + radius2 >= WINDOW_HEIGHT) vy2 = -abs(vy2);
  
  if (dist <= minClosestDistance) {
    cout << "collided and degree = " << minClosestDistance - dist << endl;
    // try to detach the objects in case they have entered themselves
    reCalculateVelocities();
   }
  return dist <= minClosestDistance;   
}

void reCalculatePositions (){
  if (checkCollision()){
    cout << "updated velocities as collision was detected\n";
  }
  std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
  int timeDelta = std::chrono::duration_cast<std::chrono::nanoseconds> (currentTime - t).count();
  t = currentTime;  
  cx1 += ((vx1 * timeDelta) * (1e-9));
  cy1 += ((vy1 * timeDelta) * (1e-9));
  cx2 += ((vx2 * timeDelta) * (1e-9));
  cy2 += ((vy2 * timeDelta) * (1e-9));
  cout << "pos : (" << cx1 << " , " << cy1 << ") " << " ( " << cx2 << " , " << cy2 << ") \n";
  cout << "vel : (" << vx1 << " , " << vy1 << ") " << " ( " << vx2 << " , " << vy2 << ") \n";
  
  glutPostRedisplay();
}

void drawCircle (double radius , double centreX , double centreY){
  double theta;
  glColor3f(1, 0, 0);
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
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, WINDOW_HEIGHT, WINDOW_WIDTH, 0.0f, 0.0f, 1.0f);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable (GL_DEPTH_TEST);
  drawCircle(radius1 , cx1 , cy1);
  drawCircle(radius2 , cx2 , cy2);
  glFlush();
  glutSwapBuffers();
}

int main (int argc , char ** argv){
    glutInit (&argc , argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowPosition (0 , 0);
    glutInitWindowSize (WINDOW_WIDTH , WINDOW_HEIGHT);
    glutCreateWindow ("SIMULATION");
    glEnable (GL_DEPTH_TEST);
    glutDisplayFunc (display);
    glutIdleFunc(reCalculatePositions);
    glutMainLoop();
    return 0;
}
