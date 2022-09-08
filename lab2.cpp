//modified by: Danny Simpson
//date: Aug 31, 2022
//
//author: Gordon Griesel
//date: Spring 2022
//purpose: get openGL working on your personal computer
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>

const int MAX_PARTICLES = 10;
//some structures
class Global {
public:
    int xres, yres;
    int RB = 0;
    Global();
} g;


class Box {
public:
    float w;
    float dir;
    float vel[2];
    float pos[2];
    Box() {
        w = 20.0f;
        dir = 25.0f;
        pos[0] = g.xres/3.0f;
        pos[1] = g.yres/1.5f;
        vel[0] = vel[1] = 0.0; 
    }
    Box(float wid, float d, float p0, float p1) {
        w = wid;
        dir = d;
        pos[0] = p0;
        pos[1] = p1;
        vel[0] = vel[1] = 0.0f;
    }


} box, particle(2.0, 0.0, g.xres/2.0, g.yres/4.0*3.0);

Box box_array[5];
Box particles[MAX_PARTICLES];
int n = 0;

class X11_wrapper {
private:
    Display *dpy;
    Window win;
    GLXContext glc;
public:
    ~X11_wrapper();
    X11_wrapper();
    void set_title();
    bool getXPending();
    XEvent getXNextEvent();
    void swapBuffers();
    void reshape_window(int width, int height);
    void check_resize(XEvent *e);
    void check_mouse(XEvent *e);
    int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);



//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
    init_opengl();
    //Main loop
    int done = 0;
    while (!done) {
        //Process external events.
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            x11.check_mouse(&e);
            done = x11.check_keys(&e);
        }
        physics();
        render();
        x11.swapBuffers();
        usleep(200);
    }
    return 0;
}

Global::Global()
{
    xres = 600;
    yres = 400;
}

X11_wrapper::~X11_wrapper()
{
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    int w = g.xres, h = g.yres;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        cout << "\n\tcannot connect to X server\n" << endl;
        exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if (vi == NULL) {
        cout << "\n\tno appropriate visual found\n" << endl;
        exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
        InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    set_title();
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "3350 Lab1");
}

bool X11_wrapper::getXPending()
{
    //See if there are pending events.
    return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
    //Get a pending event.
    XEvent e;
    XNextEvent(dpy, &e);
    return e;
}

void X11_wrapper::swapBuffers()
{
    glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
    //window has been resized.
    g.xres = width;
    g.yres = height;
    //
    glViewport(0, 0, (GLint)width, (GLint)height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
    //The ConfigureNotify is sent by the
    //server if the window is resized.
    if (e->type != ConfigureNotify)
        return;
    XConfigureEvent xce = e->xconfigure;
    if (xce.width != g.xres || xce.height != g.yres) {
        //Window size did change.
        reshape_window(xce.width, xce.height);
    }
}
//-----------------------------------------------------------------------------

void make_particle(int x, int y)
{
    if (n>=MAX_PARTICLES)
        return;
    particles[n].w = 2.0;
    particles[n].pos[0] = x;
    particles[n].pos[1] = y;
    particles[n].vel[0]=particles[n].vel[1] =0.0;
    ++n;
    render();
}
void make_boxes()
{
    for (int k = 0; k < 5; k++) {
        if (k == 0) {
            box_array[k].w = 3*box.w;
            box_array[k].pos[0] = box.pos[0] - 2.5*box.w;
            box_array[k].pos[1] = box.pos[1] + 2*box.w;
        }
        if (k == 1){
            box_array[k].w = 3*box.w;
            box_array[k].pos[0] = box.pos[0];
            box_array[k].pos[1] = box.pos[1];
        }
        else {
            box_array[k].w = 3*box.w;
            box_array[k].pos[0] = box.pos[0] + (2.5*(k-1))*box.w;
            box_array[k].pos[1] = box.pos[1] - (2*(k-1))*box.w;
        }
    }
}



void X11_wrapper::check_mouse(XEvent *e)
{
    static int savex = 0;
    static int savey = 0;

    //Weed out non-mouse events
    if (e->type != ButtonRelease &&
        e->type != ButtonPress &&
        e->type != MotionNotify) {
        //This is not a mouse event that we care about.
        return;
    }
    //
    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            //Left button was pressed.
            int y = g.yres - e->xbutton.y;
            int x = e->xbutton.x;
            make_particle(x, y);
            
            return;
        }
        if (e->xbutton.button==3) {
            //Right button was pressed.
            g.RB++;
            if (g.RB == 1) {
                float number;
                for (int count = 0; count > (n - 1); count++) {
                    number = rand() % 20;
                    if (count%2 == 0)
                        make_particle((box.pos[0] - 2.5*box.w + number), (box.pos[1] + 4*box.w));
                    else
                        make_particle((box.pos[0] - 2.5*box.w - number), (box.pos[1] + 4*box.w));
                }	
            }
            if (g.RB == 2) {
                g.RB = 0;
            }
            return;
        }
    }
    if (e->type == MotionNotify) {
        //The mouse moved!
        if (savex != e->xbutton.x || savey != e->xbutton.y) {
            savex = e->xbutton.x;
            savey = e->xbutton.y;
            //Code placed here will execute whenever the mouse moves.
        }
    }
}

int X11_wrapper::check_keys(XEvent *e)
{
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;
    int key = XLookupKeysym(&e->xkey, 0);
    if (e->type == KeyPress) {
        switch (key) {
            case XK_1:
                //Key 1 was pressed
                break;
            case XK_Escape:
                //Escape key was pressed
                return 1;
        }
    }
    return 0;
}

void init_opengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, g.xres, g.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set 2D mode (no perspective)
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    //Set the screen background color
    glClearColor(0.1, 0.1, 0.1, 1.0);
}

void physics()
{
    particle.vel[1] -= 0.01;
    particle.pos[0] += particle.vel[0];
    particle.pos[1] += particle.vel[1];
    //
    //check for collision
    for (int k = 0; k < 5; k++) {
        if (particle.pos[1] < (box_array[k].pos[1] + box.w/1.5) &&
            particle.pos[1] > (box_array[k].pos[1] - box.w/1.5) &&
            particle.pos[0] > (box_array[k].pos[0] - (2.5*box.w)) &&
            particle.pos[0] < (box_array[k].pos[0] + (2.5*box.w)) &&
            particle.pos[1] < (box_array[k].pos[1] + box.w/1.5)) {
                
                particle.vel[1] = 0.000;
                particle.vel[0] += 0.01;
        }
    }
    for (int i = 0; i<n; i++) {
        particles[i].vel[1] -= 0.01;
       	particles[i].pos[0] += particles[i].vel[0];
       	particles[i].pos[1] += particles[i].vel[1];
       	//
       	//check for collision
        for (int k = 0; k < 5; k++) {
       	    if (particles[i].pos[1] < (box_array[k].pos[1] + box.w/1.5) &&
                particles[i].pos[1] > (box_array[k].pos[1] - box.w/1.5) &&
                particles[i].pos[0] > (box_array[k].pos[0] - (2.5*box.w)) &&
                particles[i].pos[0] < (box_array[k].pos[0] + (2.5*box.w)) &&
                particles[i].pos[1] < (box_array[k].pos[1] + box.w/1.5)) {
           	    
                particles[i].vel[1] = 0.000;
                particles[i].vel[0] += 0.01;
            }
        }
        for (float j = 0; j < 90; j += 0.5) {
            if (particles[i].pos[1] < ((box.pos[1] - 11*box.w) + 3*box.w*sin(j)) &&
                particles[i].pos[1] > ((box.pos[1] - 11*box.w) - 3*box.w*sin(j)) &&
                particles[i].pos[0] < ((box.pos[0] + 12*box.w) + 3*box.w*cos(j)) &&
                particles[i].pos[0] > ((box.pos[0] + 12*box.w) - 3*box.w*cos(j)) &&
                particles[i].pos[1] < ((box.pos[1] - 11*box.w) + 3*box.w*sin(j)) ) {
                particles[i].vel[1] = 0.0;
                particles[i].vel[0] += 0.01;
            }
        }
        for (float g = 90; g < 180; g += 0.5) {
            if (particles[i].pos[1] < ((box.pos[1] - 11*box.w) + 3*box.w*sin(g)) &&
                particles[i].pos[1] > ((box.pos[1] - 11*box.w) - 3*box.w*sin(g)) &&
                particles[i].pos[0] < ((box.pos[0] + 12*box.w) + 3*box.w*cos(g)) &&
                particles[i].pos[0] > ((box.pos[0] + 12*box.w) - 3*box.w*cos(g)) &&
               	particles[i].pos[1] > ((box.pos[1] - 11*box.w) - 3*box.w*sin(g))) {
                particles[i].vel[1] = 0.0;
                particles[i].vel[0] -= 0.01;
            }
        }		
        if (particles[i].pos[1]< 0.0) {
#define OPT_1
#ifndef OPT_1
            //This code optimise below
            particles[i]= particles[n - 1];
            --n;
#else //OPT_1
            particles[i]= particles[--n];
#endif //OPT_1
        }
    }
}

void render()
{
    make_boxes();
    //
    glClear(GL_COLOR_BUFFER_BIT);
    //Draw box.
    for(int l = 0; l < 5; l++) {
        glPushMatrix();
        glColor3ub(150, 200, 120);
        glTranslatef(box_array[l].pos[0], box_array[l].pos[1], 0.0f);
        glBegin(GL_QUADS);
            glVertex2f(-(box_array[l].w), -box.w/2);
            glVertex2f(-(box_array[l].w),  box.w/2);
            glVertex2f( (box_array[l].w),  box.w/2);
            glVertex2f( (box_array[l].w), -box.w/2);
        glEnd();
        glPopMatrix();
    }
    float x, y;
    glPushMatrix();
    glColor3ub(150, 200, 120);
    glTranslatef((box.pos[0] + 12*box.w), (box.pos[1] - 11*box.w), 0.0f);
    glBegin(GL_TRIANGLES);
    for (float j = 0; j < 360;) {
        x = 3*box.w*cos(j);
        y = 3*box.w*sin(j);
        glVertex2f( x, y );
        glVertex2f( 0, 0);
        j += 0.5;
    }
    glEnd();
    glPopMatrix();

    //draw particle
    glPushMatrix();
    glColor3ub(150, 160, 255);
    glTranslatef(particle.pos[0], particle.pos[1], 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(-particle.w, -particle.w);
        glVertex2f(-particle.w,  particle.w);
        glVertex2f( particle.w,  particle.w);
        glVertex2f( particle.w, -particle.w);
    glEnd();
    glPopMatrix();

    //draw particles
    for (int i = 0; i<n; i++) {
	    glPushMatrix();
       	glColor3ub(150, 160, 255);
        glTranslatef(particles[i].pos[0], particles[i].pos[1], 0.0f);
        glBegin(GL_QUADS);
            glVertex2f(-particles[i].w, -particles[i].w);
            glVertex2f(-particles[i].w,  particles[i].w);
            glVertex2f( particles[i].w,  particles[i].w);
            glVertex2f( particles[i].w, -particles[i].w);
       	glEnd();
        glPopMatrix();
    }
//    if (g.RB == 1) {
//        int number;
//        for (int count = 0; count > (n - 1); count++) {
//            number = rand() % 60;
//            if (count%2 == 0)
//                make_particle((box.pos[0] - 2.5*box.w + number), (box.pos[1] + 4*box.w));
//            else
//                make_particle((box.pos[0] - 2.5*box.w - number), (box.pos[1] + 4*box.w));
//        }	
//    }
//    if (g.RB == 2) {
//        g.RB = 0;
//    }
//	pos[0] += dir;
//	if (pos[0] >= (g.xres-w)) {
//		pos[0] = (g.xres-w);
//		dir = -dir;
//	}
//	if (pos[0] <= w) {
//		pos[0] = w;
//		dir = -dir;
//	}
}
