//=============================================================================
// Sample Application for GLEW, and cwc Application/Window class using freeglut
//=============================================================================

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "glApplication.h"
#include "glutWindow.h"
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include "glsl.h"
#include "imathvec.h"
extern "C"{
#include "perlin.h"
}

using namespace Imath;
using namespace std;
/**************************
/* put the old stuff here *
 **************************/
GLUquadricObj *quadratic;

typedef Vec3<float> Vec3f;
typedef Vec3<int> Vec3i;


/* define the shaders */
cwc::glShaderManager SM;
cwc::glShader *shader;
cwc::glShader *textureShader;
cwc::glShader *stripeShader;
cwc::glShader *boardShader;
cwc::glShader *bumpShader;
enum{
	NORMAL_SHADER = 0,
	TEXTURE_SHADER,
	STRIPE_SHADER,
	TEXTURE_AND_STRIPE_SHADER,
	WOOD_TEXTURE,
	MARBLE_TEXTURE
};

bool isStripeForLeg = true;
/* angle class */
class Angle{
public:
	Angle(){angle = 0.0;}
	Angle(double _angle):angle(_angle){}

	double DEG2RAD(){
		return(angle*M_PI/180.0);
	}
	void setAngle(double _angle){
		angle = _angle;
	}
	double getAngle(){return angle;}

	void moveAngle(double _angle){
		angle += _angle;
	}
	void moveAnglePositive(double _angle){
		angle += _angle;
		if(angle < 0)
			angle += 360;
		else if(angle >= 360)
			angle -= 360;
	}
	Angle& operator=(const Angle& rhs){
		if(this != &rhs){
			angle = rhs.angle;
		}
		return *this;
	}
private:
	double angle;
};

/* eye class */
class Eye{
public:
	Eye(){x=y=z=lookX=lookY=lookZ=upX=upY=upZ=0.0;}
	Eye(float _x, float _y, float _z, float _lookX, float _lookY, float _lookZ):x(_x),
		y(_y), z(_z), lookX(_lookX), lookY(_lookY), lookZ(_lookZ){
			lookVector = (Vec3f(lookX, lookY, lookZ) - Vec3f(x, y, z));
			lookX = x + lookVector.x;
			lookY = y + lookVector.y;
			lookZ = z + lookVector.z;

			if(lookVector.x == 0){
				angle.setAngle(90.0);
			}
			else{
				angle.setAngle( atan(lookVector.z/lookVector.x) * 180.0 / M_PI);
			}
			dx = 0.5 * static_cast<float>(cos(angle.DEG2RAD()));
			dz = 0.5 * static_cast<float>(sin(angle.DEG2RAD()));
			dy = 0.5;
			upX = 0.0;
			upY = 1.0;
			upZ = 0.0;
	}
	

	void rotate(float theta){
		angle.moveAnglePositive(theta);
		lookVector.x = lookVector.length() * cos(angle.DEG2RAD());
		lookVector.z = -lookVector.length() * sin(angle.DEG2RAD());
		lookX = x + lookVector.x;
		lookZ = z + lookVector.z;
		dx = 0.5 * static_cast<float>(cos(angle.DEG2RAD()));
		dz = 0.5 * static_cast<float>(sin(angle.DEG2RAD()));
		dy = 0.5;

	}

	void rotateLeft(){
		rotate(5);
	}

	void rotateRight(){
		rotate(-5);
	}
	void moveForward(){
		x += dx;
		z -= dz;
		lookX = x + lookVector.x;
		lookZ = z + lookVector.z;
	}
	void moveBackward(){
		x -= dx;
		z += dz;
		lookX = x + lookVector.x;
		lookZ = z + lookVector.z;
	}
	
	void moveDown(){
		y -= dy;
		lookY = y + lookVector.y;
	}
	void moveUp(){
		y += dy;
		lookY = y + lookVector.y;
	}
	float x, y, z, lookX, lookY, lookZ, upX, upY, upZ;
	Angle angle;
	Vec3f lookVector;
protected:
	
	
	float dx, dy, dz;
};

/* material class */
class Material{
public:
	Material(){}
	
	Material(GLfloat amb0, GLfloat amb1, GLfloat amb2, GLfloat amb3,
		     GLfloat dif0, GLfloat dif1, GLfloat dif2, GLfloat dif3,
			 GLfloat spec0, GLfloat spec1, GLfloat spec2, GLfloat spec3,
			 GLfloat shin){

		setMaterial(amb0, amb1, amb2, amb3,
		             dif0, dif1, dif2, dif3,
					 spec0, spec1, spec2, spec3,
					 shin);
	}
	void setMaterial(GLfloat amb0, GLfloat amb1, GLfloat amb2, GLfloat amb3,
					 GLfloat dif0, GLfloat dif1, GLfloat dif2, GLfloat dif3,
					 GLfloat spec0, GLfloat spec1, GLfloat spec2, GLfloat spec3,
					 GLfloat shin){

		mat_ambient[0] = amb0; mat_ambient[1] = amb1; mat_ambient[2] = amb2; mat_ambient[3] = amb3;
		mat_diffuse[0] = dif0; mat_diffuse[1] = dif1; mat_diffuse[2] = dif2; mat_diffuse[3] = dif3;
		mat_specular[0] = spec0; mat_specular[1] = spec1; mat_specular[2] = spec2; mat_specular[3] = spec3;
		mat_shininess[0] = shin;
		mat_emission[0] = 1.0; mat_emission[1] = 0.3; mat_emission[2] = 0.3; mat_emission[3] = 0.0;
	}

	void putMaterial(){
		glMaterialfv(GL_FRONT,GL_AMBIENT,mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	}
	void putGlowingMaterial(){
		putMaterial();
		glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);
	}
	GLfloat mat_ambient[4];
	GLfloat mat_diffuse[4];
	GLfloat mat_specular[4];
	GLfloat mat_shininess[1];
	GLfloat mat_emission[4];


};

/* define chrome material */
Material mat_chrome(0.25, 0.25, 0.25, 1.0,

					0.4, 0.4, 0.4, 1.0,

					0.774597, 0.774597, 0.774597, 1.0,

					76.8);

/* define jade material */
Material mat_jade(	0.135, 0.2225, 0.1575, 0.95,

					0.54, 0.89, 0.63, 0.95,

					0.316228, 0.316228, 0.316228, 0.95,

					12.8);
/* define emerald material */
Material mat_emerald(0.0215, 0.1745, 0.0215, 0.55,

					0.07568, 0.61424, 0.07568, 0.55,

					0.633, 0.727811, 0.633, 0.55,

					76.8);
/* define ruby material */
Material mat_ruby(	0.1745, 0.01175, 0.01175, 0.55,

					0.61424, 0.04136, 0.04136, 0.55,

					0.727811, 0.626959, 0.626959, 0.55,

					76.8);

/* define pearl material */
Material mat_pearl( 0.25, 0.20725, 0.20725, 0.922,

					1.0, 0.829, 0.829, 0.922,

					0.296648, 0.296648, 0.296648, 0.922,

					11.264);

/* define polished silver material */
Material mat_polishedSilver(0.23125, 0.23125, 0.23125, 1.0,

					0.2775, 0.2775, 0.2775, 1.0,

					0.773911, 0.773911, 0.773911, 1.0,

					89.6);

/* define polished gold material */
Material mat_polishedGold(0.24725, 0.2245, 0.0645, 1.0,

					0.34615, 0.3143, 0.0903, 1.0,

					0.797357, 0.723991, 0.208006, 1.0,

					83.2);

Material mat_bronze(0.2125, 0.1275, 0.054, 1.0,          

					0.714, 0.4284, 0.18144, 1.0,

					0.393548, 0.271906, 0.166721, 1.0,

					25.6);


Material mat_turquoise(0.1, 0.18725, 0.1745, 0.8,

					0.396, 0.74151, 0.69102, 0.8,

					0.297254, 0.30829, 0.306678, 0.8,

					12.8);

Material mat_black_plastic(0.0, 0.0, 0.0, 1.0,

					0.01, 0.01, 0.01, 1.0,

					0.50, 0.50, 0.50, 1.0,

					32.0);

Material mat_black_rubber(0.02, 0.02, 0.02, 1.0,

					0.01, 0.01, 0.01, 1.0,

					0.4, 0.4, 0.4, 1.0,

					10.0);
/* simple sphere class, also as node in the tree */
class Sphere{
public:
	Sphere(){
		radius = 0.5;
		slice = 40;
		stack = 40;
		child = NULL;
		sibling = NULL;
	}
	Sphere(float _radius, float _slice, float _stack){
		radius = _radius;
		slice = _slice;
		stack = _stack;
		child = NULL;
		sibling = NULL;
	}
	Sphere(float _radius): radius(_radius){
		slice = 40;
		stack = 40;
		child = NULL;
		sibling = NULL;
	}
	
	void updateMatrix(){
		glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	}

	/* draw with texture */
	void draw(const GLuint texture){
		if(this->isBody()){
			if(textureShader)textureShader->begin();
			gluQuadricTexture(quadratic, 1);
			gluSphere(quadratic, radius, slice, stack);
			if(textureShader)textureShader->end();
		}
		else if(this->isLeg()){
			if(isStripeForLeg){
				if(stripeShader) stripeShader->begin();
				stripeShader->setUniform3f("StripeColor", 1.0, 0.0, 0.0);
				stripeShader->setUniform3f("BackColor", 1.0, 1.0, 1.0);
				stripeShader->setUniform1f("Width", 0.5);
				stripeShader->setUniform1f("Fuzz", 0.1);
				stripeShader->setUniform1f("Scale", 10.0);
				gluSphere(quadratic, radius, slice, stack);
				if(stripeShader) stripeShader->end();
			}
			else{
				if(boardShader) boardShader->begin();
				boardShader->setUniform4f("color1", 0.0, 0.0, 0.0, 0.0);
				boardShader->setUniform4f("color2", 0.5, 0.5, 0.5, 1.0);
				boardShader->setUniform4f("AvgColor", 0.35, 0.35, 0.35, 1.0);
				boardShader->setUniform1f("Frequency", 5);
				glPushAttrib(GL_LIGHTING);
				mat_black_plastic.putMaterial();
				gluSphere(quadratic, radius, slice, stack);
				glPopAttrib();
				if(boardShader) boardShader->end();
			}
			
			
			
			
		}
		else
		{	
			if(shader)shader->begin();
			gluSphere(quadratic, radius, slice, stack);
			if(shader)shader->end();
		}
	}

	/* normal draw function */
	void draw(){
		if(shader)shader->begin();
		gluSphere(quadratic, radius, slice, stack);
		if(shader)shader->end();
	}

	void setType(unsigned int t){
		if (t <= BODY)
			type = BODY;
		else if(t >= EYE){
			type = EYE;
		}
		else{
			type = t;
		}
	}

	bool isBody(){
		if (type == BODY)
			return true;
		else
			return false;
	}
	bool isHead(){
		if(type == HEAD)
			return true;
		else 
			return false;
	}
	bool isLeg(){
		if(type == LEG)
			return true;
		else
			return false;
	}
	bool isTail(){
		if(type == TAIL)
			return true;
		else
			return false;
	}
	bool isEar(){
		if(type == EAR)
			return true;
		else 
			return false;
	}
	bool isNose(){
		if(type == NOSE)
			return true;
		else 
			return false;
	}
	bool isEye(){
		if(type == EYE)
			return true;
		else
			return false;
	}
	GLfloat matrix[16];
	float radius;
	Sphere *child;
	Sphere *sibling;
	unsigned int type;

	static const unsigned int BODY = 0;
	static const unsigned int HEAD = 1;
	static const unsigned int LEG = 2;
	static const unsigned int TAIL = 3;
	static const unsigned int EAR = 4;
	static const unsigned int NOSE = 5;
	static const unsigned int EYE = 6;
private:
	float slice;
	float stack;
};
/* tree of spheres */
class Dog{
public:
	Dog(){
		speed = 0;
		angle.setAngle(0);
		legRotateIndex.setAngle(0);
		legRotateSpeed = 0.0;
		isMoving = false;
		center.setValue(0.0, 0.0, 0.0);
	}

	/* set up tree for the dog */
	void createDog(){
		root = new Sphere();
		root->setType(Sphere::BODY);
	
		glPushMatrix();
		glScalef(2, 1, 1);
		root->updateMatrix();
		glPopMatrix();

		head = new Sphere();
		head->setType(Sphere::HEAD);
		root -> child = head;
		
		for(int i=0; i<2; i++){
			ear[i] = new Sphere();
			ear[i]->setType(Sphere::EAR);
		}
		head->child = ear[0];
		ear[0]->sibling = ear[1];

		for(int i=0; i<2; i++){
			eye[i] = new Sphere();
			eye[i]->setType(Sphere::EYE);
		}
		ear[1]->sibling = eye[0];
		eye[0]->sibling = eye[1];

		nose = new Sphere();
		nose->setType(Sphere::NOSE);

		eye[1]->sibling = nose;

		for(int i=0; i<4; i++){
			leg[i] = new Sphere();
			leg[i]->setType(Sphere::LEG);
		}
		for(int i=0; i<3; i++){
			leg[i]->sibling = leg[i+1];
		}
		head->sibling= leg[0];		

		for(int i=0; i<4; i++){
			sleg[i] = new Sphere();
			sleg[i]->setType(Sphere::LEG);
			leg[i]->child = sleg[i];
		}
		for(int i=0; i<3; i++){
			sleg[i]->sibling = sleg[i+1];
		}
	
		for(int i=0; i<4; i++){
			tail[i] = new Sphere();
			tail[i]->setType(Sphere::TAIL);
		}
		leg[3]->sibling = tail[0];
		for(int i=0; i<3; i++){
			tail[i]->child = tail[i+1];
		}

	}

	/* update matrix in each node */
	void updateDog(){
		glPushMatrix();
		glLoadIdentity();
		
		/* body matrix(root) */
		glPushMatrix();
		glTranslatef(center.x, center.y, center.z);
		glRotatef(angle.getAngle(), 0, 1, 0);
		glTranslatef(-speed, 0, 0);
		
		glScalef(2, 1, 1);
		root->updateMatrix();
		glPopMatrix();

		/* head matrix */
		glPushMatrix();
		glTranslatef(abs(cos(45*M_PI/180.0))*(-1+ root->radius/4.0), (1-root->radius/4.0)*abs(sin(45*M_PI/180.0)), 0);
		glScalef(0.5, 1, 1);
		head->updateMatrix();
		glPopMatrix();
		
		/* ear matrix */
		glPushMatrix();
		glRotatef(30, 1, 0, 0);
		glTranslatef(0, 0, -head->radius);
		glScalef(0.2, 0.2, 0.5);
		ear[0]->updateMatrix();
		glPopMatrix();
		
		glPushMatrix();
		glRotatef(180-30, 1, 0, 0);
		glTranslatef(0, 0, -head->radius);
		glScalef(0.2, 0.2, 0.5);
		ear[1]->updateMatrix();
		glPopMatrix();

		/* eye matrix */
		glPushMatrix();
		glRotatef(30, 0, 0, -1);
		glRotatef(30, 0, 1, 0);
		glTranslatef(-head->radius, 0, 0);
		glScalef(0.2, 0.2, 0.2);
		eye[0]->updateMatrix();
		glPopMatrix();

		glPushMatrix();
		glRotatef(30, 0, 0, -1);
		glRotatef(-30, 0, 1, 0);
		glTranslatef(-head->radius, 0, 0);
		glScalef(0.2, 0.2, 0.2);
		eye[1]->updateMatrix();
		glPopMatrix();

		/* nose matrix */
		glPushMatrix();
		glRotatef(-20, 0, 0, -1);
		glTranslatef(-head->radius, 0, 0);
		glScalef(0.3, 0.2, 0.2);
		nose->updateMatrix();
		glPopMatrix();

		/* leg matrix */
		glPushMatrix();
		glScalef(0.5, 1, 1);
		glRotatef(-30, 1, 0, 0);
		glRotatef(-30 - legRotateSpeed*sin(legRotateIndex.DEG2RAD()), 0, 0, 1);
		glTranslatef(0, -root->radius*2*0.7, 0);
		glRotatef(90 ,1, 0, 0);
		glScalef(0.2, 0.2, 0.7);
		leg[0]->updateMatrix();
		glPopMatrix();

		glPushMatrix();
		glScalef(0.5, 1, 1);
		glRotatef(30, 1, 0, 0);
		glRotatef(-30 + legRotateSpeed*sin(legRotateIndex.DEG2RAD()), 0, 0, 1);
		glTranslatef(0, -root->radius*2*0.7, 0);
		glRotatef(90 ,1, 0, 0);
		glScalef(0.2, 0.2, 0.7);
		leg[1]->updateMatrix();
		glPopMatrix();

		glPushMatrix();
		glScalef(0.5, 1, 1);
		glRotatef(30, 1, 0, 0);
		glRotatef(30 + legRotateSpeed*sin(legRotateIndex.DEG2RAD()), 0, 0, 1);
		glTranslatef(0, -root->radius*2*0.7, 0);
		glRotatef(90 ,1, 0, 0);
		glScalef(0.2, 0.2, 0.7);
		leg[2]->updateMatrix();
		glPopMatrix();

		glPushMatrix();
		glScalef(0.5, 1, 1);
		glRotatef(-30, 1, 0, 0);
		glRotatef(30 - legRotateSpeed*sin(legRotateIndex.DEG2RAD()), 0, 0, 1);
		glTranslatef(0, -root->radius*2*0.7, 0);
		glRotatef(90 ,1, 0, 0);
		glScalef(0.2, 0.2, 0.7);
		leg[3]->updateMatrix();
		glPopMatrix();


		for(int i=0; i<4; i++){
			glPushMatrix();
			
			glTranslatef(0, 0, root->radius*2*0.8);
			sleg[i]->updateMatrix();
			glPopMatrix();
		}

		/* tail matrix */
		glPushMatrix();
		glScalef(0.5, 1, 1);
		glTranslatef(root->radius*1.1*2, -root->radius*0.3, 0);
		glRotatef(-45, 0, 0, 1);
		glScalef(0.4, 0.2, 0.2);
		tail[0]->updateMatrix();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(root->radius, 0, 0);
		glScalef(1/0.4, 1/0.2, 1/0.2);
		glRotatef(-20, 0, 0, 1);
		glScalef(0.4, 0.2, 0.2);
		glTranslatef(root->radius, 0, 0);
		tail[1]->updateMatrix();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(root->radius, 0, 0);
		glScalef(1/0.4, 1/0.2, 1/0.2);
		glRotatef(-20, 0, 0, 1);
		glScalef(0.4, 0.2, 0.2);
		glTranslatef(root->radius, 0, 0);
		tail[2]->updateMatrix();
		glPopMatrix();

		glPopMatrix();
	}

	/* traverse through the tree of spheres and draw the dog */
	void traverse(Sphere *r, const GLuint texture, int flag){
		
		if(r == NULL) return;
		
		glPushMatrix();
		glMultMatrixf(r->matrix);
		
		glPushAttrib(GL_LIGHTING_BIT);
		/* set material for each component */
		if(r->isBody())
			mat_emerald.putMaterial();
		else if(r->isLeg())
			mat_chrome.putMaterial();
		else if(r->isHead())
			mat_pearl.putMaterial();
		else if(r->isTail())
			mat_polishedSilver.putMaterial();
		else if(r->isEar() || r->isEye() || r->isNose())
			mat_ruby.putGlowingMaterial();
		/* draw component */
		if(flag == 0)
			r->draw(texture);
		if(flag == -1)
			r->draw();
		else
			r->draw();
		glPopAttrib();
		if(r->child != NULL) 
			traverse(r->child, texture, flag);
		glPopMatrix();
		if(r->sibling != NULL) 
			traverse(r->sibling, texture, flag);
	}

	void drawDog(GLuint texture){
		traverse(root, texture, 0);
	}
	
	void drawDog(){
		traverse(root, 0, -1);
	}
	void rotate(float ang){
		angle.moveAnglePositive(ang);
	}
	Vec3f getPointFromMatrix(Vec3f p, GLfloat matrix[]){
		Vec3f t;
		t.x = p.x * matrix[0] + p.y * matrix[4] + p.z * matrix[8] + matrix[12];
		t.y = p.x * matrix[1] + p.y * matrix[5] + p.z * matrix[9] + matrix[13];
		t.z = p.x * matrix[2] + p.y * matrix[6] + p.z * matrix[10] + matrix[14];
		return t;
	}
	void moveDog(){
		center = getPointFromMatrix(Vec3f(0.0, 0.0, 0.0), root->matrix);
	}
	void speedUp(){
		speed += 0.01;
		legRotateSpeed += 2;
		if (legRotateSpeed > 20)
			legRotateSpeed = 20;
	}
	void slowDown(){
		speed -= 0.01;
		if(speed < 0)
			speed = 0;
		legRotateSpeed -= 2;
		if (legRotateSpeed < 0)
			legRotateSpeed = 0;
	}
	void reset(){
		//speed = 0;
		center.setValue(0.0, 0.0, 0.0);
		glPushMatrix();
		glLoadIdentity();
		root->updateMatrix();
		glPopMatrix();
		//legRotateSpeed = 0;
	}

	void changeLegRotateIndex(float spd){
		legRotateIndex.moveAnglePositive(spd);
	}

	void resetLegRotateIndex(){
		legRotateIndex.setAngle(0);
	}
	Sphere *root;
	Sphere *head;
	Sphere *leg[4];
	Sphere *sleg[4];
	Sphere *tail[4];
	Sphere *eye[2];
	Sphere *ear[2];
	Sphere *nose;

	float speed;
	Angle angle;

	float legRotateSpeed;
	Angle legRotateIndex;
	bool isMoving;
	Vec3f center;
};


/* draw the checker board */
void drawCheckerBoard(float width){
	if(!isStripeForLeg){
		if(stripeShader) stripeShader->begin();
		stripeShader->setUniform3f("StripeColor", 1.0, 0.0, 0.0);
		stripeShader->setUniform3f("BackColor", 1.0, 1.0, 1.0);
		stripeShader->setUniform1f("Width", 0.5);
		stripeShader->setUniform1f("Fuzz", 0.1);
		stripeShader->setUniform1f("Scale", 10.0);
	}
	else{
		if(boardShader) boardShader->begin();
		boardShader->setUniform4f("color1", 0.0, 0.0, 0.0, 0.0);
		boardShader->setUniform4f("color2", 0.5, 0.5, 0.5, 0.5);
		boardShader->setUniform4f("AvgColor", 0.5, 0.5, 0.5, 1.0);
		boardShader->setUniform1f("Frequency", 5);
	}
	glBegin(GL_QUADS);
		glNormal3f(0.0, 0.0, 1.0);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);

		glTexCoord2f(1.0, 0.0);
		glVertex3f(width, 0.0, 0.0);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(width, width, 0.0);

		glTexCoord2f(0.0, 1.0);
		glVertex3f(0.0, width, 0.0);
			
	glEnd();
	if(stripeShader) stripeShader->end();
	if(boardShader) boardShader->end();
}

/* draw a plane */
void drawPlane(int width){
	glBegin(GL_QUADS);
		glNormal3f(0.0, 0.0, 1.0);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);

		glTexCoord2f(1.0, 0.0);
		glVertex3f(width, 0.0, 0.0);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(width, width, 0.0);

		glTexCoord2f(0.0, 1.0);
		glVertex3f(0.0, width, 0.0);
			
	glEnd();
}
void drawCoord(){
	glBegin(GL_LINES);
		glVertex3f(0.0, 0.0, 0.0); glVertex3f(100.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 100.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 0.0, 100.0);
	glEnd();
}

/* woodmap function */
	Vec3f woodmap(float a){
		Vec3f earlywood(0.06, 0.035, 0.0);
		Vec3f latewood(0.6, 0.35, 0.0);

		return (1-a)*earlywood + a*latewood;
	}
	/* get wood color */
	Vec3i woodColor(float s, float t, float r){
		double n;
		/* only take the fraction part of s^2 + t^2*/
		float frac = (s-0.5)*(s-0.5) + (t-0.5)*(t-0.5);
		frac = frac /(r*r);
		frac = (float)modf((double)frac + 2*PerlinNoise2D(s,t,2,2,5), &n);
		return woodmap(frac) * 255.0;
	}
	/* marble map */
	Vec3f marblemap(float a){
		Vec3f earlymarble(0.03, 0.035, 0.04);
		Vec3f latemarble(0.3, 0.35, 0.4);

		return (1-a)*earlymarble + a*latemarble;
	}
	/* get marble color*/
	Vec3i marbleColor(float s, float t, float r){
		float frac = sin(20*s + 6*PerlinNoise3D(5*s, 5*t, r, 2, 2, 10));
		if(frac < 0) frac = 0;
		else if(frac > 1)frac =1;
		return marblemap(frac) * 255.0;
	}
	/* generate a texture */
	void generateTexture(GLubyte img[512][512][4], GLuint& tex, int flag){
		if(flag == WOOD_TEXTURE){
			for(int s = 0; s < 512; s++){
				for(int t = 0; t<512; t++){
					Vec3i c = woodColor(s/512.0, t/512.0, 0.2);
					img[s][t][0] = (GLubyte)c[0];
					img[s][t][1]= (GLubyte)c[1];
					img[s][t][2] = (GLubyte)c[2];
					img[s][t][3] = (GLubyte)255;
				}
			}
		}
		else if(flag == MARBLE_TEXTURE){
			for(int s = 0; s < 512; s++){
				for(int t = 0; t<512; t++){
					Vec3i c = marbleColor(s/512.0, t/512.0, 11.5);
					img[s][t][0] = (GLubyte)c[0];
					img[s][t][1]= (GLubyte)c[1];
					img[s][t][2] = (GLubyte)c[2];
					img[s][t][3] = (GLubyte)255;
				}
			}
		}
		else
			return;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
		
	}
/****************************
/* Declare Global Variables *
 ****************************/
Eye eye(1, 2, 30, 0, 0, 0);
Dog dog;
//-----------------------------------------------------------------------------

extern "C" unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
extern double PerlinNoise2D(double x,double y,double alpha,double beta,int n);

class myWindow : public cwc::glutWindow
{
protected:
   

protected: 
	int scene;

	//Ross: Some variables I created for texturing
	unsigned char *image;
	int imageWidth;
	int imageHeight;
	int bitDepth;
	GLuint texture;
	GLubyte woodImage[512][512][4];
	GLubyte marbleImage[512][512][4];
	GLuint woodTexture;
	GLuint marbleTexture;
	unsigned int texture_choice;
	bool useBumpForScene3;
public:
	myWindow(){}
	
	virtual void OnRender(void)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Ross:  This is where I draw things.  I have three different scene examples.
		switch(scene)
		{
			case 1:
				Scene1();
				break;
			case 2:
				Scene2();
				break;
			case 3:
				Scene3();
				break;
		}
		
		glutSwapBuffers();
	}

	void Scene1(void)
	{
		 DemoLight();
		glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	
       gluPerspective(45,1,1,100);
	   glMatrixMode(GL_MODELVIEW);
	   glLoadIdentity();
	  
	   gluLookAt(eye.x, eye.y, eye.z, eye.lookX, eye.lookY, eye.lookZ, eye.upX, eye.upY, eye.upZ);
	  
	   //drawCoord();
		if (textureShader) textureShader->begin();
			//Essentially I need to tell the shader what texture to use
			glBindTexture(GL_TEXTURE_2D, this->texture);
	
			/* draw the simple plane */
			glPushAttrib(GL_LIGHTING_BIT);
			mat_polishedSilver.putMaterial();
			glPushMatrix();
			glTranslatef(5.0, 0.0, 0.0);
			drawPlane(2);
			glPopMatrix();
			glPopAttrib();

		/* draw the simple sphere */
		glPushMatrix();
		glTranslatef(4, 0, 0);
		glBindTexture(GL_TEXTURE_2D, this->texture);
		gluQuadricTexture(quadratic, 1);
		gluSphere(quadratic, 0.5, 40, 40);
		glPopMatrix();
		if (textureShader) textureShader->end();

		/* draw dog */
		dog.updateDog();
		dog.drawDog(this->texture);

		/* draw the checkerboard */
		glPushMatrix();
		glTranslatef(0.0, -1.2, 0.0);
		glRotatef(90, 1, 0, 0);
		glTranslatef(-9.0, -9.0, 0.0);
		glPushAttrib(GL_LIGHTING_BIT);
		mat_chrome.putMaterial();
		drawCheckerBoard(18.0);
		glPopAttrib();
		glPopMatrix();
		 
		
			//glutSolidTeapot(1.);
		
	}

	void Scene2(void)
	{
		
		DemoLight();
		glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	
       gluPerspective(45,1,1,100);
	   glMatrixMode(GL_MODELVIEW);
	   glLoadIdentity();
	   gluLookAt(eye.x, eye.y, eye.z, eye.lookX, eye.lookY, eye.lookZ, eye.upX, eye.upY, eye.upZ);

		
		//Essentially I need to tell the shader what texture to use
		glBindTexture(GL_TEXTURE_2D, this->texture);

		
		if(shader) shader->begin();

		/* draw torus */
		glPushMatrix();
		glTranslatef(0, 0, -5);
		glPushAttrib(GL_LIGHTING_BIT);
		mat_chrome.putMaterial();
		glutSolidTorus(0.5, 0.8, 40, 40);
		glPopAttrib();
		glPopMatrix();

		/* draw ground plane */
		glPushMatrix();
		glTranslatef(0, -1.2, 0);
		glRotatef(90, -1, 0, 0);
		glTranslatef(-9, -9, 0);
		glPushAttrib(GL_LIGHTING_BIT);
		mat_turquoise.putMaterial();
		drawPlane(18);
		glPopAttrib();
		glPopMatrix();

		/* draw the transparent dog */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		dog.updateDog();
		dog.drawDog();
		glDisable(GL_BLEND);

		//drawCoord();
		
		if (shader)shader->end();
	}

	void Scene3(void)
	{
		DemoLight();
		glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	
       gluPerspective(45,1,1,100);
	   glMatrixMode(GL_MODELVIEW);
	   glLoadIdentity();
	   gluLookAt(eye.x, eye.y, eye.z, eye.lookX, eye.lookY, eye.lookZ, eye.upX, eye.upY, eye.upZ);
	 
		//Essentially I need to tell the shader what texture to use
	   if(texture_choice == WOOD_TEXTURE)
			glBindTexture(GL_TEXTURE_2D, this->woodTexture);
	   else if(texture_choice == MARBLE_TEXTURE)
			glBindTexture(GL_TEXTURE_2D, this->marbleTexture);
	   else
			glBindTexture(GL_TEXTURE_2D, this->woodTexture);

	   /* choose the material */
		glPushAttrib(GL_LIGHTING_BIT);
		if(texture_choice == WOOD_TEXTURE)
			mat_bronze.putMaterial();
		else 
			mat_polishedSilver.putMaterial();

		/* decide whether to use bump mapping */
		if(!useBumpForScene3){
			if(textureShader) textureShader->begin();
		}
		else{
			if(bumpShader) bumpShader->begin();
		}

		/* draw sphere */
		glPushMatrix();
		gluSphere(quadratic, 1, 100, 100); 
		glPopMatrix();
	
		/* draw simple plane */
		glPushMatrix();
		glTranslatef(-5, 0, 0);
		drawPlane(2);
		glPopMatrix();
		
		/* draw ground plane */
		glPushMatrix();
		glTranslatef(0, -1.2, 0);
		glRotatef(90, -1, 0, 0);
		glTranslatef(-9, -9, 0);
		drawPlane(18);
		glPopMatrix();

		/* draw cone */
		glPushMatrix();
			glTranslatef(5, 0, 0);
			glRotatef(-90, 0, 1, 0);
			gluQuadricTexture(quadratic, 2);
			gluCylinder(quadratic, 1.0, 0, 1.5, 100, 100);
		glPopMatrix();
		
		if (textureShader) textureShader->end();
		if (bumpShader) bumpShader->end();
		glPopAttrib();
	}


	virtual void OnIdle() {}

	// When OnInit is called, a render context (in this case GLUT-Window) 
	// is already available!
	virtual void OnInit()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);

		//Ross: Here is where I declare some variables you need for textures (not scene, that's for changing the scene)
		this->scene = 1;
		this->imageWidth = 512;
		this->imageHeight = 512;
		this->bitDepth = 24;

		//Ross:  This is where I set up the lights
		DemoLight();

		//Ross:  This is where I set up the textures
		DemoTexture();
		generateTexture(woodImage, woodTexture, WOOD_TEXTURE);
		generateTexture(marbleImage, marbleTexture, MARBLE_TEXTURE);
		texture_choice = WOOD_TEXTURE;
		useBumpForScene3 = false;
		//Ross: The GLSL loaders initialization is already handled by the CWC files linked.
		//If you change the name of your shader you would need to modify here

		/* create the normal lighting shader */
		shader = SM.loadfromFile("vertexshader.txt","fragmentshader.txt"); // load (and compile, link) from file
		
		if (shader==0) 
			std::cout << "Error Loading, compiling or linking shader\n";
		
		
		/* create the shader for texture mapping */
		textureShader = SM.loadfromFile("texture_vertexshader.txt", "texture_fragmentshader.txt");
		if (textureShader==0)
			std::cout << "Error Loading, compiling or linking texture shader\n";

		/* create the shader for precedurely generating the strips */
		stripeShader = SM.loadfromFile("stripes_vertexshader.txt", "stripes_fragmentshader.txt");
		if (stripeShader==0)
			std::cout << "Error Loading, compiling or linking stripes shader\n";
		/* shader for checker board */
		boardShader = SM.loadfromFile("board_vertexshader.txt", "board_fragmentshader.txt");
		if (boardShader == 0)
			std::cout << "Error Loading, compiling or linking board shader\n";

		/* shader for bump mapping */
		bumpShader = SM.loadfromFile("bump_vertexshader.txt", "bump_fragmentshader.txt");
		if(bumpShader == 0)
			std::cout << "Error Loading, compiling or linking bump shader\n";
	}

	//Ross: Callbacks for most everything you need
	virtual void OnResize(int w, int h) {}
	virtual void OnClose(void){}
	virtual void OnMouseDown(int button, int x, int y) {}    
	virtual void OnMouseUp(int button, int x, int y) {}
	virtual void OnMouseWheel(int nWheelNumber, int nDirection, int x, int y){}

	//Ross: This should be obvious, but these are the keyboard listeners
	virtual void OnKeyDown(int nKey, char cAscii)
	{       
		if (cAscii == 27) // 0x1b = ESC
		{
			this->Close(); // Close Window!
		} 

		if(cAscii == '1')
			this->scene = 1;
		if(cAscii == '2')
			this->scene = 2;
		if(cAscii == '3')
			this->scene = 3;

		/* dog control */
		if(cAscii == 'l' || cAscii == 'L'){
			dog.rotate(-5);
		}
		if(cAscii == 'k' || cAscii == 'K'){
			dog.rotate(5);
		}

		/* scene 3 texture control */
		if(cAscii == 't' || cAscii == 'T'){
			if(texture_choice == WOOD_TEXTURE)
				texture_choice = MARBLE_TEXTURE;
			else if(texture_choice == MARBLE_TEXTURE)
				texture_choice = WOOD_TEXTURE;
		}

		/* scene 3 bump control */
		if(cAscii == 'b' || cAscii == 'B'){
			if(useBumpForScene3)
				useBumpForScene3 = false;
			else
				useBumpForScene3 = true;
		}
		/* scene 1 shader control */
		if(cAscii == 'g' || cAscii == 'G'){
			if(isStripeForLeg)
				isStripeForLeg = false;
			else
				isStripeForLeg = true;
		}
		/* eye control */
		if(cAscii == 'a' || cAscii == 'A'){
			eye.moveDown();
		}
		if(cAscii == 's' || cAscii == 'S'){
			eye.moveUp();
		}
		if(nKey == GLUT_KEY_UP){
			eye.moveForward();
		}
		if(nKey == GLUT_KEY_DOWN){
			eye.moveBackward();
		}
		if(nKey == GLUT_KEY_LEFT){
			eye.rotateLeft();
		}
		if(nKey == GLUT_KEY_RIGHT){
			eye.rotateRight();
		}
		glutPostRedisplay();
	};

	virtual void OnKeyUp(int nKey, char cAscii)
	{
		if (cAscii == 'f')
		{
			SetFullscreen(true);
		}
		else if (cAscii == 'w')
		{
			SetFullscreen(false);
		}
	};


	

	//Ross: This is where I set up all the textures
	void DemoTexture(void)
	{
		/* Load in a PNG image */
		int loadCorrectly = 0;
		//Use the stbi_image file to load an image
		image = stbi_load("me3.bmp", &this->imageWidth, &this->imageHeight, &this->bitDepth, 0);

		glGenTextures(1, &this->texture); //generate the texture with the loaded data

		//The first thing that must take place in the process of uploading the texture is a call to glBindTexture. 
		//What glBindTexture does is it tells OpenGL which texture "id" we will be working with. 
		//A texture "id" is just a number that you will use to access your textures. Here is a sample call.
		glBindTexture(GL_TEXTURE_2D, texture); //bind texture to its array


		//The glPixelStorei call tells OpenGL how the data that is going to be uploaded is aligned. 
		//This call tells OpenGL that the pixel data which is going to be passed to it is aligned in byte order, 
		//this means that the data has one byte for each component, one for red, green and blue
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
		//The glTexEnvf call sets environment variables for the current texture. 
		//What this does is tell OpenGL how the texture will act when it is rendered into a scene.
		//What this does is sets the active texture to GL_MODULATE. 
		//The GL_MODULATE attribute allows you to apply effects such as lighting and coloring to your texture. 
		//If you do not want lighting and coloring to effect your texture and you would like to display the texture unchanged when 
		//coloring is applied replace GL_MODULATE with GL_DECAL.
		//This is not needed if you use shaders!!
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		//The glTexImage2D call is our goal. 
		//This call will upload the texture to the video memory where it will be ready for us to use in our programs. 
		//I am going to explain this call parameter by parameter since it is so important to what we are doing.
		//target - The target of this call, it will always be GL_TEXTURE_2D.
		//level - The level of detail number, this should be left at 0 for our purposes. 
			//Once you become more adept at OpenGL texture mapping this parameter will be something that you might change.
		//internalformat - Internal components parameter. 
			//This tells OpenGL how many color components to represent internally from the texture that is uploaded. 
			//There are many symbolic constants for this parameter but the one which is most widely used is GL_RGB; this constant is equal to 3.
		//width & height - The width and height of the image data. These must be integers that are equal to 2n+2(border) for some integer n.
			//What this basically means is that the texture width and height must be a power of two (2,4,8,16,32,63,128,256,512, etc).
		//border - Image border, must be 0 or 1. I always use 0 in my code since I do not use image borders.
		//format - Format of the pixel data that will be uploaded. There are many constants which are accepted but GL_RGB is the value that is widely used.
		//type - Type of data that will be uploaded. Again there are several symbolic constants but the one which I use is GL_UNSIGNED_BYTE.
		//pixels - Pointer to the image data. 
			//This is the image data that will be uploaded to the video memory. 
			//Note that after your call to glTexImage2D you can free this memory since the texture is already uploaded into video memory.
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glEnable (GL_TEXTURE_2D);
	}

   //Ross: This is where I set up the lights
   void DemoLight(void)
   {
     glEnable(GL_LIGHTING);
     glEnable(GL_LIGHT0);
     glEnable(GL_NORMALIZE);
     
     // Light model parameters:
     // -------------------------------------------
     
     GLfloat lmKa[] = {0.0, 0.0, 0.0, 0.0 };
     glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmKa);
     
     glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0);
     glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0.0);
     
     // -------------------------------------------
     // Spotlight Attenuation
     
     GLfloat spot_direction[] = {1.0, -1.0, -1.0 };
     GLint spot_exponent = 30;
     GLint spot_cutoff = 180;
     
     glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
     glLighti(GL_LIGHT0, GL_SPOT_EXPONENT, spot_exponent);
     glLighti(GL_LIGHT0, GL_SPOT_CUTOFF, spot_cutoff);
    
     GLfloat Kc = 1.0;
     GLfloat Kl = 0.0;
     GLfloat Kq = 0.0;
     
     glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,Kc);
     glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, Kl);
     glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, Kq);
     
     
     // ------------------------------------------- 
     // Lighting parameters:

     //GLfloat light_pos[] = {0.0f, 5.0f, 5.0f, 1.0f};
	 GLfloat light_pos[] = {5.0f, 5.0f, 5.0f, 1.0f};
     GLfloat light_Ka[]  = /*{1.0f, 0.5f, 0.5f, 1.0f};*/ {1.0f, 1.0f, 1.0f, 1.0f};
     GLfloat light_Kd[]  = /*{1.0f, 0.1f, 0.1f, 1.0f};*/ {1.0f, 1.0f, 1.0f, 1.0f};
     GLfloat light_Ks[]  = {1.0f, 1.0f, 1.0f, 1.0f};

     glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
     glLightfv(GL_LIGHT0, GL_AMBIENT, light_Ka);
     glLightfv(GL_LIGHT0, GL_DIFFUSE, light_Kd);
     glLightfv(GL_LIGHT0, GL_SPECULAR, light_Ks);

     // -------------------------------------------
     // Material parameters:

     GLfloat material_Ka[] = {0.5f, 0.0f, 0.0f, 1.0f};
     GLfloat material_Kd[] = {0.4f, 0.4f, 0.5f, 1.0f};
     GLfloat material_Ks[] = {0.8f, 0.8f, 0.0f, 1.0f};
     GLfloat material_Ke[] = {0.1f, 0.0f, 0.0f, 0.0f};
     GLfloat material_Se = 20.0f;

     glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_Ka);
     glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_Kd);
     glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_Ks);
     glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_Ke);
     glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material_Se);
   }
};

//-----------------------------------------------------------------------------

class myApplication : public cwc::glApplication
{
public:
	//Ross: Here is a simple output for checking that things are working
	virtual void OnInit() {std::cout << "Hello World!\n"; }
};

//-----------------------------------------------------------------------------

int main(void)
{
	quadratic = gluNewQuadric();
	gluQuadricNormals(quadratic, GLU_SMOOTH);
		/* create the tree */
	dog.createDog();
	myApplication*  pApp = new myApplication;
	myWindow* myWin = new myWindow();

	pApp->run();
	delete pApp;
	return 0;
}

//-----------------------------------------------------------------------------

