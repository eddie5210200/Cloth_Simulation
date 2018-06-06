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
#define BALL_RADIUS 2
#define TRUE 1
#define FALSE 0
#define PARTICLE_NUMBER_X 100
#define PARTICLE_NUMBER_Y 100
#define SPRING_NUMBER (PARTICLE_NUMBER_X) * (PARTICLE_NUMBER_Y - 1) + (PARTICLE_NUMBER_X - 1) * (PARTICLE_NUMBER_Y) + (PARTICLE_NUMBER_X - 1) * (PARTICLE_NUMBER_Y - 1) * 2
#define SPRING_LENGTH 0.12
#define SPRING_CONSTANT 0.2
#define EXCEPTION_NUMBER 6

//Use w to switch from wireframe to cloth
//Run in wireframe to reduce lag
//Space to run

// *************************************************************************************
// * GLOBAL variables. Not ideal but necessary to get around limitatins of GLUT API... *
// *************************************************************************************
int pause = TRUE;
int wire = FALSE;
typedef struct Vec2{
    int x;
    int y;
} vec2;
typedef struct Vector{
    double x;
    double y;
    double z;
}Vector;

typedef struct Particle{
    Vector pos;
    Vector velocity;
    Vector forces;
    double mass;
    double radius;
} Particle;

typedef struct Spring{
    vec2 end1;
    vec2 end2;
    double k;
    double initialLength;
} Spring;

typedef struct Verlet{
    Vector newPos[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
    Vector oldPos[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
    Vector forces[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
    Vector gravity;
    double dt;
} Verlet;

Verlet verlet;
Particle particle[PARTICLE_NUMBER_Y][PARTICLE_NUMBER_X];
Spring springs[SPRING_NUMBER];
vec2 exceptions[EXCEPTION_NUMBER];
Vector ball;

//variables for calculations - for faster computation
Vector position2;
Vector position1;
Vector difference_position, correction; 
double displacement, f, unit;  
Vector force;     
vec2 j;
Vector particle_to_ball;
double total_dist;
Vector current, temp, old, a;

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
        particle[i][j].pos.x = i * SPRING_LENGTH;
        verlet.oldPos[i][j].x = i * SPRING_LENGTH;
        verlet.newPos[i][j].x = i * SPRING_LENGTH;
        particle[i][j].pos.y = 1 - j * SPRING_LENGTH;
        verlet.oldPos[i][j].y = 1 - j * SPRING_LENGTH;
        verlet.newPos[i][j].y = 1 - j * SPRING_LENGTH;
        particle[i][j].pos.z = 0;
        verlet.oldPos[i][j].z = 0;
        verlet.newPos[i][j].z = 0;
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
  Vector particle_to_particle, particle_to_particle_2;
  //vertical/horizontal springs
  int spring_index = 0;
  for(int i = 0; i < PARTICLE_NUMBER_Y - 1; i++){
     for(int j = 0; j < PARTICLE_NUMBER_X; j++){
        //Horizontal Springs
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
  verlet.gravity.y = -10;
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

    //setup ball
    ball.x = BALL_POSITION_X + 5;
    ball.y = BALL_POSITION_Y;
    ball.z = BALL_POSITION_Z + 3;
}

void compute_detection(Vector circle, double radius){
    for(int i = 0; i < SPRING_NUMBER; i++){
        j = springs[i].end1;
        //calculate the difference between circle center and point
        particle_to_ball.x = verlet.newPos[j.y][j.x].x - circle.x;
        particle_to_ball.y = verlet.newPos[j.y][j.x].y - circle.y;
        particle_to_ball.z = verlet.newPos[j.y][j.x].z - circle.z;
        total_dist = sqrt(pow(particle_to_ball.x, 2) + pow(particle_to_ball.y, 2) + pow(particle_to_ball.z, 2));
        //normalize and offset from the circle
        if(total_dist < radius){
            Vector normalize;
            normalize.x = particle_to_ball.x/total_dist;
            normalize.y = particle_to_ball.y/total_dist;
            normalize.z = particle_to_ball.z/total_dist;
            normalize.x *= (radius - total_dist);
            normalize.y *= (radius - total_dist);
            normalize.z *= (radius - total_dist);
            verlet.newPos[j.y][j.x].x += normalize.x;
            verlet.newPos[j.y][j.x].y += normalize.y;
            verlet.newPos[j.y][j.x].z += normalize.z;
        }
    }
}

int exception(int y, int x){
    for(int i = 0; i < EXCEPTION_NUMBER; i++)
        if(exceptions[i].x == x && exceptions[i].y == y) return 1;
    return 0;
}


void satisfy_condition(void){
    for(int i = 0; i < SPRING_NUMBER; i++){
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
        displacement = sqrt(pow(difference_position.x, 2) + pow(difference_position.y, 2) + pow(difference_position.z, 2));
        correction.x = difference_position.x * (1.0f - springs[i].initialLength / displacement);
        correction.y = difference_position.y * (1.0f - springs[i].initialLength / displacement);
        correction.z = difference_position.z * (1.0f - springs[i].initialLength / displacement);
            
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
            //gravity
            verlet.forces[i][j].x += verlet.gravity.x;
            verlet.forces[i][j].y += verlet.gravity.y;
            verlet.forces[i][j].z += verlet.gravity.z;
            //wind forces
            verlet.forces[i][j].x += 0;
            verlet.forces[i][j].y += 0;
            verlet.forces[i][j].z += -20;
        }
    }    

    //calculate spring forces
    for(int i = 0; !pause && i < SPRING_NUMBER; i++){
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
            unit = sqrt(pow(difference_position.x, 2) + pow(difference_position.y, 2) + pow(difference_position.z, 2));
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

    //satisfy conditions
    for(int i = 0; i < 15 && !pause; i++)
        satisfy_condition();

    //integrate equations of motion
    for(int i = 0; !pause && i < PARTICLE_NUMBER_Y; i++){
        for(int j = 0; j < PARTICLE_NUMBER_X; j++){
            //skip for anchor points
            if(exception(i, j)) continue;
            current = verlet.newPos[i][j];
            temp.x = current.x;
            temp.y = current.y;
            temp.z = current.z;
            old = verlet.oldPos[i][j];
            a = verlet.forces[i][j];
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
    compute_detection(ball, BALL_RADIUS);
    if(wire){
        for(int i = 0; i < PARTICLE_NUMBER_Y; i++){
            for(int j = 0; j < PARTICLE_NUMBER_X; j++){
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
            glPushMatrix();
            glBegin(GL_TRIANGLE_STRIP);       
            for(int j = 0; j < PARTICLE_NUMBER_X; j++){ 
                //glTranslatef(verlet.newPos[i][j].x, verlet.newPos[i][j].y, verlet.newPos[i][j].z);
                if(j % 2 == 0 && i % 2 == 0) glColor3f(1.0f, 1.0f, 1.0f);
                else glColor3f(0.5f, 0.5f, 0.5f);
                glVertex3f(verlet.newPos[i][j].x, verlet.newPos[i][j].y, verlet.newPos[i][j].z);  
                glVertex3f(verlet.newPos[i + 1][j].x, verlet.newPos[i + 1][j].y, verlet.newPos[i + 1][j].z);   
            }
            glEnd();
            glPopMatrix();
        }    
    }
}

void moveball(void){
   if(!pause){
       if(ball.z > -20){
            ball.z-= 0.2;
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
  //draw_rope();
  glPushMatrix ();
  glTranslatef (ball.x, ball.y, ball.z);
  glColor3f (1.0f, 1.0f, 0.0f);
  glutSolidSphere (BALL_RADIUS - 0.1, 50, 50); 
// draw the ball, but with a slightly lower radius, otherwise we could get ugly visual artifacts of rope penetrating the ball slightly
  glPopMatrix ();
  draw_rope();
  moveball();
  glutSwapBuffers();
  glutPostRedisplay();
  //printf("%3f\n", ball.z);
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

