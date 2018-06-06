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
#define PARTICLE_NUMBER_X 50
#define PARTICLE_NUMBER_Y 50
#define SPRING_NUMBER (PARTICLE_NUMBER_X) * (PARTICLE_NUMBER_Y - 1) + (PARTICLE_NUMBER_X - 1) * (PARTICLE_NUMBER_Y) + (PARTICLE_NUMBER_X - 1) * (PARTICLE_NUMBER_Y - 1) * 2
#define SPRING_LENGTH 0.25
#define SPRING_CONSTANT 0.05
#define EXCEPTION_NUMBER 6

//Use w to switch from wireframe to cloth

// *************************************************************************************
// * GLOBAL variables. Not ideal but necessary to get around limitatins of GLUT API... *
// *************************************************************************************
int pause = TRUE;
int wire = TRUE;
typedef struct Vec2{
    int x;
    int y;
} vec2;
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
    vec2 end1;
    vec2 end2;
    float k;
    float initialLength;
} Spring;

typedef struct Verlet{
    Vector newPos[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
    Vector oldPos[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
    Vector forces[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
    Vector gravity;
    float dt;
} Verlet;

Verlet verlet;
Particle particle[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
Spring springs[SPRING_NUMBER];
vec2 exceptions[EXCEPTION_NUMBER];

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
  //delta time
  verlet.dt= 0.006;
  //setup ball

  for(int i = 0; i < PARTICLE_NUMBER_Y; i++){
     for(int j = 0; j < PARTICLE_NUMBER_X; j++){
        particle[i][j].pos.x = BALL_POSITION_X - 6 + i * SPRING_LENGTH;
        verlet.oldPos[i][j].x = BALL_POSITION_X - 6 + i * SPRING_LENGTH;
        verlet.newPos[i][j].x = BALL_POSITION_X - 6 + i * SPRING_LENGTH;
        particle[i][j].pos.y = BALL_POSITION_Y + 8 - j * SPRING_LENGTH;
        verlet.oldPos[i][j].y = BALL_POSITION_Y + 8 - j * SPRING_LENGTH;
        verlet.newPos[i][j].y = BALL_POSITION_Y + 8 - j * SPRING_LENGTH;
        particle[i][j].pos.z = BALL_POSITION_Z;
        verlet.oldPos[i][j].z = BALL_POSITION_Z;
        verlet.newPos[i][j].z = BALL_POSITION_Z;
        particle[i][j].forces.x = 0;
        particle[i][j].forces.y = 0;
        particle[i][j].forces.z = 0;
        verlet.forces[i][j].x = 0;
        verlet.forces[i][j].y = 0;
        verlet.forces[i][j].z = 0;
        particle[i][j].velocity.x = 0;
        particle[i][j].velocity.y = 0;
        particle[i][j].velocity.z = 0;
        particle[i][j].mass = 1;
        particle[i][j].radius = 0.15;
    }
  }
  //vertical/horizontal springs
  int spring_index = 0;
  for(int i = 0; i < PARTICLE_NUMBER_Y - 1; i++){
     for(int j = 0; j < PARTICLE_NUMBER_X; j++){
        //Horizontal Springs
        Vector particle_to_particle;
        particle_to_particle.x = particle[i + 1][j].pos.x - particle[i][j].pos.x;
        particle_to_particle.y = particle[i + 1][j].pos.y - particle[i][j].pos.y;
        particle_to_particle.z = particle[i + 1][j].pos.z - particle[i][j].pos.z;
        springs[spring_index].end1.y = i;
        springs[spring_index].end1.x = j;
        springs[spring_index].end2.y = i+1;
        springs[spring_index].end2.x = j;
        springs[spring_index].k = SPRING_CONSTANT;
        springs[spring_index].initialLength = sqrt(pow(particle_to_particle.x, 2) + pow(particle_to_particle.y, 2) + pow(particle_to_particle.z, 2));
        spring_index++;
        //Vertical Springs
        Vector particle_to_particle_2;
        particle_to_particle_2.x = particle[j][i + 1].pos.x - particle[j][i].pos.x;
        particle_to_particle_2.y = particle[j][i + 1].pos.y - particle[j][i].pos.y;
        particle_to_particle_2.z = particle[j][i + 1].pos.z - particle[j][i].pos.z;
        springs[spring_index].end1.x = i;
        springs[spring_index].end1.y = j;
        springs[spring_index].end2.x = i+1;
        springs[spring_index].end2.y = j;
        springs[spring_index].k = SPRING_CONSTANT;
        springs[spring_index].initialLength = sqrt(pow(particle_to_particle_2.x, 2) + pow(particle_to_particle_2.y, 2) + pow(particle_to_particle_2.z, 2));
        spring_index++;
    }
  }
  //shear springs
  for(int i = 0; i < PARTICLE_NUMBER_Y - 1; i++){
     for(int j = 0; j < PARTICLE_NUMBER_X - 1; j++){
        //Horizontal Springs
        Vector particle_to_particle;
        particle_to_particle.x = particle[i][j].pos.x - particle[i + 1][j + 1].pos.x;
        particle_to_particle.y = particle[i][j].pos.y - particle[i + 1][j + 1].pos.y;
        particle_to_particle.z = particle[i][j].pos.z - particle[i + 1][j + 1].pos.z;
        springs[spring_index].end1.y = i+1;
        springs[spring_index].end1.x = j+1;
        springs[spring_index].end2.y = i;
        springs[spring_index].end2.x = j;
        springs[spring_index].k = SPRING_CONSTANT;
        springs[spring_index].initialLength = sqrt(pow(particle_to_particle.x, 2) + pow(particle_to_particle.y, 2) + pow(particle_to_particle.z, 2));
        spring_index++;
        //Vertical Springs
        Vector particle_to_particle_2;
        particle_to_particle_2.x = particle[i + 1][j].pos.x - particle[i][j + 1].pos.x;
        particle_to_particle_2.y = particle[i + 1][j].pos.y - particle[i][j + 1].pos.y;
        particle_to_particle_2.z = particle[i + 1][j].pos.z - particle[i][j + 1].pos.z;
        springs[spring_index].end1.x = i;
        springs[spring_index].end1.y = j+1;
        springs[spring_index].end2.x = i+1;
        springs[spring_index].end2.y = j;
        springs[spring_index].k = SPRING_CONSTANT;
        springs[spring_index].initialLength = sqrt(pow(particle_to_particle_2.x, 2) + pow(particle_to_particle_2.y, 2) + pow(particle_to_particle_2.z, 2));
        spring_index++;
    }
  }
  verlet.gravity.x = 0;
  verlet.gravity.y = -100;
  verlet.gravity.z = 0;

  //anchor points
  for(int i = 0; i < EXCEPTION_NUMBER; i++){
      vec2 temp;
      switch(i){  
          case 0:    
              temp.x = 0;
              temp.y = 0;
              break;
          case 1:    
              temp.x = 0;
              temp.y = 1;
              break;
          case 5:    
              temp.x = 1;
              temp.y = 0;
              break;
          case 2:    
              temp.x = 0;
              temp.y = PARTICLE_NUMBER_Y - 1;
              break;
          case 4:    
              temp.x = 1;
              temp.y = PARTICLE_NUMBER_Y - 1;
              break;
          case 3:    
              temp.x = 0;
              temp.y = PARTICLE_NUMBER_Y - 2;
              break;
          default:
              break;
      }
      exceptions[i] = temp;
   }
}

/*
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
}*/

int exception(int y, int x){
    for(int i = 0; i < EXCEPTION_NUMBER; i++)
        if(exceptions[i].x == x && exceptions[i].y == y) return 1;
    return 0;
}

void satisfy_condition(void){
    for(int i = 0; i < SPRING_NUMBER; i++){
        Vector position2;
        Vector position1;
        Vector correction;
        Vector difference_position;
        float dist, f;      
        vec2 j;
        //calculate spring forces k * (L - l)
        j = springs[i].end1;
        position1.x = verlet.newPos[j.y][j.x].x;
        position1.y = verlet.newPos[j.y][j.x].y;
        position1.z = verlet.newPos[j.y][j.x].z;
        j = springs[i].end2;
        position2.x = verlet.newPos[j.y][j.x].x;
        position2.y = verlet.newPos[j.y][j.x].y;
        position2.z = verlet.newPos[j.y][j.x].z;
        difference_position.x = position2.x - position1.x; 
        difference_position.y = position2.y - position1.y;
        difference_position.z = position2.z - position1.z;
        dist = sqrt(pow(difference_position.x, 2) + pow(difference_position.y, 2) + pow(difference_position.z, 2));
        correction.x = difference_position.x * (1.0f - springs[i].initialLength / dist);
        correction.y = difference_position.y * (1.0f - springs[i].initialLength / dist);
        correction.z = difference_position.z * (1.0f - springs[i].initialLength / dist);
            
        j = springs[i].end1;
        if(!exception(j.y, j.x)){    
        verlet.newPos[j.y][j.x].x += 0.5 * correction.x;
        verlet.newPos[j.y][j.x].y += 0.5 * correction.y;
        verlet.newPos[j.y][j.x].z += 0.5 * correction.z;
        }

        j = springs[i].end2;
        if(!exception(j.y, j.x)){                
        verlet.newPos[j.y][j.x].x -= 0.5 * correction.x;
        verlet.newPos[j.y][j.x].y -= 0.5 * correction.y;
        verlet.newPos[j.y][j.x].z -= 0.5 * correction.z;
        }
    }

}

void calcLoads(void){
    for(int i = 0; !pause && i < PARTICLE_NUMBER_Y; i++){
        for(int j = 0; j < PARTICLE_NUMBER_X; j++){
            //zero forces
            verlet.forces[i][j].x = 0;
            verlet.forces[i][j].y = 0;
            verlet.forces[i][j].z = 0;
            //gravity
            verlet.forces[i][j].x += verlet.gravity.x;
            verlet.forces[i][j].y += verlet.gravity.y;
            verlet.forces[i][j].z += verlet.gravity.z;
        }
    }    

    //calculate spring forces
    for(int i = 0; !pause && i < SPRING_NUMBER; i++){
            Vector position2;
            Vector position1;
            Vector difference_position; 
            float displacement, f;  
            Vector force;     
            vec2 j;
            //calculate spring forces k * (L - l)
            j = springs[i].end1;
            position1.x = verlet.newPos[j.y][j.x].x;
            position1.y = verlet.newPos[j.y][j.x].y;
            position1.z = verlet.newPos[j.y][j.x].z;
            j = springs[i].end2;
            position2.x = verlet.newPos[j.y][j.x].x;
            position2.y = verlet.newPos[j.y][j.x].y;
            position2.z = verlet.newPos[j.y][j.x].z;
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
            verlet.forces[j.y][j.x].x += force.x/2.0f;
            verlet.forces[j.y][j.x].y += force.y/2.0f;
            verlet.forces[j.y][j.x].z += force.z/2.0f;
            j = springs[i].end2;
            verlet.forces[j.y][j.x].x -= force.x/2.0f;
            verlet.forces[j.y][j.x].y -= force.y/2.0f;
            verlet.forces[j.y][j.x].z -= force.z/2.0f;
    }
    for(int i = 0; i < 100 && !pause; i++)
        satisfy_condition();

    //integrate equations of motion
    for(int i = 0; !pause && i < PARTICLE_NUMBER_Y; i++){
        for(int j = 0; j < PARTICLE_NUMBER_X; j++){
            //skip for anchor points
            if(exception(i, j)) continue;
            Vector current = verlet.newPos[i][j];
            Vector temp;
            temp.x = current.x;
            temp.y = current.y;
            temp.z = current.z;
            Vector old = verlet.oldPos[i][j];
            Vector a = verlet.forces[i][j];
            verlet.newPos[i][j].x += (current.x - old.x) + a.x * verlet.dt * verlet.dt;
            verlet.newPos[i][j].y += (current.y - old.y) + a.y * verlet.dt * verlet.dt;
            verlet.newPos[i][j].z += (current.z - old.z) + a.z * verlet.dt * verlet.dt;
            verlet.oldPos[i][j] = temp;
            //zero forces
            verlet.forces[i][j].x = 0;
            verlet.forces[i][j].y = 0;
            verlet.forces[i][j].z = 0;
        }
    }        
}

void draw_rope(void){
    calcLoads();
    if(wire){
        for(int i = 0; i < PARTICLE_NUMBER_Y; i++){
            for(int j = 0; j < PARTICLE_NUMBER_X; j++){
                //compute_detection();
                glPushMatrix();
                //Circle    
                glColor3f(1.0f, 1.0f, 1.0f);
                glTranslatef(verlet.newPos[i][j].x, verlet.newPos[i][j].y, verlet.newPos[i][j].z);
                glutSolidSphere (particle[i][j].radius - 0.1, 50, 50);
                glPopMatrix();
            }
        }
        //Springs
        for(int i = 0; i < SPRING_NUMBER; i++){
           //Lines connecting the two    
           glPushMatrix();
           glColor3f(1.0f, 1.0f, 1.0f);
           glBegin(GL_LINES);
           glVertex3f(verlet.newPos[springs[i].end1.y][springs[i].end1.x].x, verlet.newPos[springs[i].end1.y][springs[i].end1.x].y, verlet.newPos[springs[i].end1.y][springs[i].end1.x].z);
           glVertex3f(verlet.newPos[springs[i].end2.y][springs[i].end2.x].x, verlet.newPos[springs[i].end2.y][springs[i].end2.x].y, verlet.newPos[springs[i].end2.y][springs[i].end2.x].z);            
           glEnd();
           glPopMatrix();
        }
    } else {
        //draw cloth
        for(int i = 0; i < PARTICLE_NUMBER_Y - 1; i++){
            for(int j = 0; j < PARTICLE_NUMBER_X - 1; j++){
                glPushMatrix();
                //Circle    
                if(j % 2 == 0) glColor3f(1.0f, 1.0f, 1.0f);
                else glColor3f(0.5f, 0.5f, 0.5f);
                glTranslatef(verlet.newPos[i][j].x, verlet.newPos[i][j].y, verlet.newPos[i][j].z);
                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(0, 0, 0);
                glVertex3f(SPRING_LENGTH, 0, 0);
                glVertex3f(SPRING_LENGTH, SPRING_LENGTH, 0);
                glVertex3f(0, SPRING_LENGTH, 0);
                glEnd();
                glPopMatrix();
            }
        }    

    }
}



void display (void)
{
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
  //glTranslatef (BALL_POSITION_X, BALL_POSITION_Y, BALL_POSITION_Z);
  //glColor3f (1.0f, 1.0f, 0.0f);
  //glutSolidSphere (BALL_RADIUS - 0.1, 50, 50); 
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
    case 'w':
      wire = 1 - wire;
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
  glutCreateWindow ("Cloth simulator");
  init ();
  glutDisplayFunc (display);  
  glutReshapeFunc (reshape);
  glutKeyboardFunc (keyboard);
  glutSpecialFunc (arrow_keys);
  glutMainLoop ();
}

