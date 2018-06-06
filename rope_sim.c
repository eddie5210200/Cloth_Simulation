#include <math.h>
#include <stdio.h>
#include <time.h>
#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GLUT/glut.h>
#elif __linux__
  #include <GL/gl.h>
  #include <GL/glut.h> 
#endif




#define BALL_POSITION_X 6
#define BALL_POSITION_Y -7
#define BALL_POSITION_Z 0
#define BALL_RADIUS 0.75
#define TRUE 1
#define FALSE 0
#define PARTICLE_NUMBER 10
#define STRING_LENGTH 1.5



// *************************************************************************************
// * GLOBAL variables. Not ideal but necessary to get around limitatins of GLUT API... *
// *************************************************************************************
int pause = TRUE;

typedef struct Vector{
    float x;
    float y;
    float z;
}Vector;

typedef struct Particle{
    Vector pos;
    Vector velocity;
    Vector forces;
    float mass;
    float radius;
} Particle;

typedef struct Spring{
    int end1;
    int end2;
    float k;
    float initialLength;
} Spring;

typedef struct Verlet{
    Vector newPos[PARTICLE_NUMBER];
    Vector oldPos[PARTICLE_NUMBER];
    Vector forces[PARTICLE_NUMBER];
    Vector gravity;
    float dt;
} Verlet;

Verlet verlet;
Particle particle[PARTICLE_NUMBER];
Spring springs[PARTICLE_NUMBER - 1];


void init (void)
{
  glShadeModel (GL_SMOOTH);
  glClearColor (0.2f, 0.2f, 0.4f, 0.5f);				
  glClearDepth (1.0f);
  glEnable (GL_DEPTH_TEST);
  glDepthFunc (GL_LEQUAL);
  glEnable (GL_COLOR_MATERIAL);
  glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glEnable (GL_LIGHTING);
  glEnable (GL_LIGHT0);
  GLfloat lightPos[4] = {-1.0, 1.0, 0.5, 0.0};
  glLightfv (GL_LIGHT0, GL_POSITION, (GLfloat *) &lightPos);
  glEnable (GL_LIGHT1);
  GLfloat lightAmbient1[4] = {0.0, 0.0,  0.0, 0.0};
  GLfloat lightPos1[4]     = {1.0, 0.0, -0.2, 0.0};
  GLfloat lightDiffuse1[4] = {0.5, 0.5,  0.3, 0.0};
  glLightfv (GL_LIGHT1,GL_POSITION, (GLfloat *) &lightPos1);
  glLightfv (GL_LIGHT1,GL_AMBIENT, (GLfloat *) &lightAmbient1);
  glLightfv (GL_LIGHT1,GL_DIFFUSE, (GLfloat *) &lightDiffuse1);
  glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  for(int i = 0; i < PARTICLE_NUMBER; i++){
    particle[i].pos.x = BALL_POSITION_X - 6 + i * STRING_LENGTH;
    verlet.oldPos[i].x = BALL_POSITION_X - 6 + i * STRING_LENGTH;
    verlet.newPos[i].x = BALL_POSITION_X - 6 + i * STRING_LENGTH;
    particle[i].pos.y = BALL_POSITION_Y + 0.75;
    verlet.oldPos[i].y = BALL_POSITION_Y + 0.75;
    verlet.newPos[i].y = BALL_POSITION_Y + 0.75;
    particle[i].pos.z = BALL_POSITION_Z;
    verlet.oldPos[i].z = BALL_POSITION_Z;
    verlet.newPos[i].z = BALL_POSITION_Z;
    particle[i].forces.x = 0;
    particle[i].forces.y = 0;
    particle[i].forces.z = 0;
    verlet.forces[i].x = 0;
    verlet.forces[i].y = 0;
    verlet.forces[i].z = 0;
    particle[i].velocity.x = 0;
    particle[i].velocity.y = 0;
    particle[i].velocity.z = 0;
    particle[i].mass = 1;
    particle[i].radius = 0.15;
  }
  for(int i = 0; i < PARTICLE_NUMBER - 1; i++){
        Vector particle_to_particle;
        particle_to_particle.x = particle[i + 1].pos.x - particle[i].pos.x;
        particle_to_particle.y = particle[i + 1].pos.y - particle[i].pos.y;
        particle_to_particle.z = particle[i + 1].pos.z - particle[i].pos.z;
        springs[i].end1 = i;
        springs[i].end2 = i+1;
        springs[i].k = 0.5;
        springs[i].initialLength = sqrt(pow(particle_to_particle.x, 2) + pow(particle_to_particle.y, 2) + pow(particle_to_particle.z, 2));
  }
    verlet.gravity.x = 0;
    verlet.gravity.y = -400;
    verlet.gravity.z = 0;
}



void compute_detection(){
    Vector particle_to_ball;
    float total_dist;
        for(int i = 0; i < PARTICLE_NUMBER; i++){
        //calculate the difference between circle center and point
        particle_to_ball.x = verlet.newPos[i].x - BALL_POSITION_X;
        particle_to_ball.y = verlet.newPos[i].y - BALL_POSITION_Y;
        particle_to_ball.z = verlet.newPos[i].z - BALL_POSITION_Z;
        total_dist = sqrt(pow(particle_to_ball.x, 2) + pow(particle_to_ball.y, 2) + pow(particle_to_ball.z, 2));
        //normalize and offset from the circle
        if(total_dist < particle[i].radius + BALL_RADIUS){
            Vector normalize;
            normalize.x = particle_to_ball.x/total_dist;
            normalize.y = particle_to_ball.y/total_dist;
            normalize.z = particle_to_ball.z/total_dist;
            normalize.x *= (BALL_RADIUS - total_dist);
            normalize.y *= (BALL_RADIUS - total_dist);
            normalize.z *= (BALL_RADIUS - total_dist);
            verlet.newPos[i].x += normalize.x;
            verlet.newPos[i].y += normalize.y;
            verlet.newPos[i].z += normalize.z;
        }
    }
}

void satisfy_condition(void){
    for(int i = 0; i < PARTICLE_NUMBER - 1; i++){
        Vector position2;
        Vector position1;
        Vector correction;
        Vector difference_position;
        float dist, f;      
        int j;
        //calculate spring forces k * (L - l)
        j = springs[i].end1;
        position1.x = verlet.newPos[j].x;
        position1.y = verlet.newPos[j].y;
        position1.z = verlet.newPos[j].z;
        j = springs[i].end2;
        position2.x = verlet.newPos[j].x;
        position2.y = verlet.newPos[j].y;
        position2.z = verlet.newPos[j].z;
        difference_position.x = position2.x - position1.x; 
        difference_position.y = position2.y - position1.y;
        difference_position.z = position2.z - position1.z;
        dist = sqrt(pow(difference_position.x, 2) + pow(difference_position.y, 2) + pow(difference_position.z, 2));
        correction.x = difference_position.x * (1.0f - springs[i].initialLength / dist);
        correction.y = difference_position.y * (1.0f - springs[i].initialLength / dist);
        correction.z = difference_position.z * (1.0f - springs[i].initialLength / dist);
            
        j = springs[i].end1;
        if(j != 0 && j + 1 != PARTICLE_NUMBER){    
        verlet.newPos[j].x += 0.5 * correction.x;
        verlet.newPos[j].y += 0.5 * correction.y;
        verlet.newPos[j].z += 0.5 * correction.z;
        }

        j = springs[i].end2;
        if(j != 0 && j + 1 != PARTICLE_NUMBER){                
        verlet.newPos[j].x -= 0.5 * correction.x;
        verlet.newPos[j].y -= 0.5 * correction.y;
        verlet.newPos[j].z -= 0.5 * correction.z;
        }
    }

}

void calcLoads(void){
    for(int i = 0; !pause && i < PARTICLE_NUMBER; i++){
        //zero forces
        verlet.forces[i].x = 0;
        verlet.forces[i].y = 0;
        verlet.forces[i].z = 0;
        //gravity
        verlet.forces[i].x += verlet.gravity.x;
        verlet.forces[i].y += verlet.gravity.y;
        verlet.forces[i].z += verlet.gravity.z;
    }    
    //calculate spring forces
    for(int i = 0; !pause && i < PARTICLE_NUMBER - 1; i++){
        Vector position2;
        Vector position1;
        Vector difference_position; 
        float displacement, f;  
        Vector force;     
        int j;
        //calculate spring forces k * (L - l)
        j = springs[i].end1;
        position1.x = verlet.newPos[j].x;
        position1.y = verlet.newPos[j].y;
        position1.z = verlet.newPos[j].z;
        j = springs[i].end2;
        position2.x = verlet.newPos[j].x;
        position2.y = verlet.newPos[j].y;
        position2.z = verlet.newPos[j].z;
        difference_position.x = position2.x - position1.x; 
        difference_position.y = position2.y - position1.y;
        difference_position.z = position2.z - position1.z;
        float unit = sqrt(pow(difference_position.x, 2) + pow(difference_position.y, 2) + pow(difference_position.z, 2));

        displacement = unit - springs[i].initialLength;
        f = displacement * springs[i].k;
        
        //actual forces
        force.x = (-difference_position.x / unit) * f;
        force.y = (-difference_position.y / unit) * f;
        force.z = (-difference_position.z / unit) * f;
        j = springs[i].end1;
        verlet.forces[j].x += force.x/2.0f;
        verlet.forces[j].y += force.y/2.0f;
        verlet.forces[j].z += force.z/2.0f;
        j = springs[i].end2;
        verlet.forces[j].x -= force.x/2.0f;
        verlet.forces[j].y -= force.y/2.0f;
        verlet.forces[j].z -= force.z/2.0f;
    }
    for(int i = 0; i < 100 && !pause; i++)
        satisfy_condition();
    
    //integrate equations of motion
    for(int i = 0; !pause && i < PARTICLE_NUMBER; i++){
        //skip for calculating first and last particle
        if(i == 0 || i == PARTICLE_NUMBER - 1) continue;
        Vector current = verlet.newPos[i];
        Vector temp;
        temp.x = current.x;
        temp.y = current.y;
        temp.z = current.z;
        Vector old = verlet.oldPos[i];
        Vector a = verlet.forces[i];
        verlet.newPos[i].x += (current.x - old.x) + a.x * verlet.dt * verlet.dt;
        verlet.newPos[i].y += (current.y - old.y) + a.y * verlet.dt * verlet.dt;
        verlet.newPos[i].z += (current.z - old.z) + a.z * verlet.dt * verlet.dt;
        verlet.oldPos[i] = temp;
                //zero forces
        verlet.forces[i].x = 0;
        verlet.forces[i].y = 0;
        verlet.forces[i].z = 0;
    }        
}
void draw_rope(void){
    calcLoads();
    for(int i = 0; i < PARTICLE_NUMBER; i++){
    compute_detection();
    glPushMatrix();
       //Circle    
        glColor3f(1.0f, 1.0f, 1.0f);
        glTranslatef(verlet.newPos[i].x, verlet.newPos[i].y, verlet.newPos[i].z);
        glutSolidSphere (particle[i].radius - 0.1, 50, 50);
        glPopMatrix();
        if(i + 1 < PARTICLE_NUMBER){
            //Lines connecting the two    
            glPushMatrix();
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_LINES);
                glVertex3f(verlet.newPos[springs[i].end1].x, verlet.newPos[springs[i].end1].y, verlet.newPos[springs[i].end1].z);
                glVertex3f(verlet.newPos[springs[i].end2].x, verlet.newPos[springs[i].end2].y, verlet.newPos[springs[i].end2].z);            
            glEnd();
        }
    glPopMatrix();
    }
}



void display (void)
{
  //time
  verlet.dt= 0.002;
  int x;
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity ();
  glDisable (GL_LIGHTING);
  //background
  glBegin (GL_POLYGON);
  glColor3f (0.8f, 0.8f, 1.0f);
  glVertex3f (-200.0f, -100.0f, -100.0f);
  glVertex3f (200.0f, -100.0f, -100.0f);
  glColor3f (0.4f, 0.4f, 0.8f);	
  glVertex3f (200.0f, 100.0f, -100.0f);
  glVertex3f (-200.0f, 100.0f, -100.0f);
  glEnd ();
  glEnable (GL_LIGHTING);
  glTranslatef (-6.5, 6, -9.0f); // move camera out and center on the rope
  draw_rope();
  glPushMatrix ();
  glTranslatef (BALL_POSITION_X, BALL_POSITION_Y, BALL_POSITION_Z);
  glColor3f (1.0f, 1.0f, 0.0f);
  glutSolidSphere (BALL_RADIUS - 0.1, 50, 50); 
// draw the ball, but with a slightly lower radius, otherwise we could get ugly visual artifacts of rope penetrating the ball slightly
  glPopMatrix ();
  draw_rope();
  glutSwapBuffers();
  glutPostRedisplay();
}




void reshape (int w, int h)  
{
  glViewport (0, 0, w, h);
  glMatrixMode (GL_PROJECTION); 
  glLoadIdentity ();  
  if (h == 0)  
  { 
    gluPerspective (80, (float) w, 1.0, 5000.0);
  }
  else
  {
    gluPerspective (80, (float) w / (float) h, 1.0, 5000.0);
  }
  glMatrixMode (GL_MODELVIEW);  
  glLoadIdentity (); 
}





void keyboard (unsigned char key, int x, int y) 
{
  switch (key) 
  {
    case 27:    
      exit (0);
    break;  
    case 32:
      pause = 1 - pause;
      break;
    default: 
    break;
  }
}





void arrow_keys (int a_keys, int x, int y) 
{
  switch(a_keys) 
  {
    case GLUT_KEY_UP:
      glutFullScreen();
    break;
    case GLUT_KEY_DOWN: 
      glutReshapeWindow (1280, 720 );
    break;
    default:
    break;
  }
}





int main (int argc, char *argv[]) 
{
  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); 
  glutInitWindowSize (1280, 720 ); 
  glutCreateWindow ("Rope simulator");
  init ();
  glutDisplayFunc (display);  
  glutReshapeFunc (reshape);
  glutKeyboardFunc (keyboard);
  glutSpecialFunc (arrow_keys);
  glutMainLoop ();
}

