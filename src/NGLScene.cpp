#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Transformation.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Quaternion.h>


//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1;

NGLScene::NGLScene(QWindow *_parent) : OpenGLWindow(_parent)
{
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  m_rotate=false;
  // mouse rotation values set to 0
  m_spinXFace=0;
  m_spinYFace=0;
  setTitle("Qt5 Simple NGL Demo");

  m_sphereUpdateTimer=startTimer(0);
  currentTime.start();
}


NGLScene::~NGLScene()
{
  ngl::NGLInit *Init = ngl::NGLInit::instance();
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
//  Init->NGLQuit();
  m_vao->removeVOA();
}

void NGLScene::resizeEvent(QResizeEvent *_event )
{
  if(isExposed())
  {
  int w=_event->size().width();
  int h=_event->size().height();
  // set the viewport for openGL
  glViewport(0,0,w,h);
  // now set the camera size values as the screen size has changed
  m_cam->setShape(45,(float)w/h,0.05,350);
  renderLater();
  }
}


void NGLScene::initialize()
{
  // we need to initialise the NGL lib which will load all of the OpenGL functions, this must
  // be done once we have a valid GL context but before we call any GL commands. If we dont do
  // this everything will crash
  ngl::NGLInit::instance();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,1,20);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,1,0);

  m_cam= new ngl::Camera(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam->setShape(45,(float)720.0/576.0,0.001,150);

  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  // load a frag and vert shaders

  // we are creating a shader called Phong
  shader->createShaderProgram("Phong");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("PhongVertex",ngl::VERTEX);
  shader->attachShader("PhongFragment",ngl::FRAGMENT);
  // attach the source
  shader->loadShaderSource("PhongVertex","shaders/PhongVertex.glsl");
  shader->loadShaderSource("PhongFragment","shaders/PhongFragment.glsl");
  // compile the shaders
  shader->compileShader("PhongVertex");
  shader->compileShader("PhongFragment");
  // add them to the program
  shader->attachShaderToProgram("Phong","PhongVertex");
  shader->attachShaderToProgram("Phong","PhongFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("Phong");
  // and make it active ready to load values
  (*shader)["Phong"]->use();
  // the shader will use the currently active material and light0 so set them
  ngl::Material m(ngl::GOLD);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");
  shader->setShaderParam3f("viewerPos",m_cam->getEye().m_x,m_cam->getEye().m_y,m_cam->getEye().m_z);
  // now create our light this is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  ngl::Mat4 iv=m_cam->getViewMatrix();
  iv.transpose();
  iv=iv.inverse();
  ngl::Light l(ngl::Vec3(0,1,0),ngl::Colour(1,1,1,1),ngl::Colour(1,1,1,1),ngl::POINTLIGHT);
  l.setTransform(iv);
  // load these values to the shader as well
  l.loadToShader("light");

  buildVAO();
  glViewport(0,0,width(),height());
}


void NGLScene::buildVAO()
{
  ngl::Vec3 verts[]=
  {

      ngl::Vec3(0,2,0.0),
      ngl::Vec3(1,0,0),
      ngl::Vec3(-1,0,0)

//    ngl::Vec3(0,1,1),
//    ngl::Vec3(0,0,-1),
//    ngl::Vec3(-0.5,0,1),
//    ngl::Vec3(0,1,1),
//    ngl::Vec3(0,0,-1),
//    ngl::Vec3(0.5,0,1),
//    ngl::Vec3(0,1,1),
//    ngl::Vec3(0,0,1.5),
//    ngl::Vec3(-0.5,0,1),
//    ngl::Vec3(0,1,1),
//    ngl::Vec3(0,0,1.5),
//    ngl::Vec3(0.5,0,1)

  };

  std::vector <ngl::Vec3> normals;
  ngl::Vec3 n=ngl::calcNormal(verts[0],verts[1],verts[2]);
  normals.push_back(n);
  normals.push_back(n);
  normals.push_back(n);
//  n=ngl::calcNormal(verts[3],verts[4],verts[5]);
//  normals.push_back(n);
//  normals.push_back(n);
//  normals.push_back(n);

//  n=ngl::calcNormal(verts[6],verts[7],verts[8]);
//  normals.push_back(n);
//  normals.push_back(n);
//  normals.push_back(n);

//  n=ngl::calcNormal(verts[11],verts[10],verts[9]);
//  normals.push_back(n);
//  normals.push_back(n);
//  normals.push_back(n);

  std::cout<<"sizeof(verts) "<<sizeof(verts)<<" sizeof(ngl::Vec3) "<<sizeof(ngl::Vec3)<<"\n";
  // create a vao as a series of GL_TRIANGLES
  m_vao= ngl::VertexArrayObject::createVOA(GL_TRIANGLES);
  m_vao->bind();

  // in this case we are going to set our data as the vertices above

	m_vao->setData(sizeof(verts),verts[0].m_x);
	// now we set the attribute pointer to be 0 (as this matches vertIn in our shader)

	m_vao->setVertexAttributePointer(0,3,GL_FLOAT,0,0);

	m_vao->setData(sizeof(verts),normals[0].m_x);
	// now we set the attribute pointer to be 2 (as this matches normal in our shader)

    m_vao->setVertexAttributePointer(2,3,GL_FLOAT,0,0);

    m_vao->setNumIndices(sizeof(verts)/sizeof(ngl::Vec3));

 // now unbind
  m_vao->unbind();


}
static int t=0;
static float v1Xcoord;
const float  startLerp=0.0f;
const float  endLerp=5.0f;
static float increment=0.01f;
static int directionFlag=1;


ngl::Mat4 translateMat=1;
ngl::Mat4 scaleMat=1;
ngl::Mat4 rotateMat=1;
void NGLScene::render()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // Rotation based on the mouse position for our global transform
  ngl::Transformation trans;
  // Rotation based on the mouse position for our global
  // transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_spinXFace);
  rotY.rotateY(m_spinYFace);
  // multiply the rotations
  m_mouseGlobalTX=rotY*rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;


  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["Phong"]->use();
  ngl::Material m(ngl::PEWTER);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;


  ngl::Vec3 v1(2*cos(v1Xcoord*2),4*sin(v1Xcoord*4) ,2*sin(v1Xcoord*2));
  ngl::Vec3 v2(0,0.1,0);

  ngl::Vec3 v2NonNormalized=v2;

  m_transform.reset();
  //draw first triangle
  {

      m_transform.setPosition(v1);
      M=m_transform.getMatrix()*m_mouseGlobalTX;
      MV=  M*m_cam->getViewMatrix();
      MVP= M*m_cam->getVPMatrix();
      normalMatrix=MV;
      normalMatrix.inverse();
      shader->setShaderParamFromMat4("MV",MV);
      shader->setShaderParamFromMat4("MVP",MVP);
      shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
      shader->setShaderParamFromMat4("M",M);

      ngl::VAOPrimitives::instance()->draw("cube");

//      m_vao->bind();
//      m_vao->draw();
//      m_vao->unbind();
  }

  v1.normalize();
  v2.normalize();
  float angle = acos(v1.dot(v2));
  ngl::Vec3 rotationAxis = v1.cross(v2);
  rotationAxis.normalize();

//  ngl::Quaternion q ;
//  q.fromAxisAngle(rotationAxis,angle);


  //calculate rotation vector from 2nd to 1st triangle
  //then

  m.set(ngl::BRONZE);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");

  //... rotate and draw the 2nd triangle as well
  m_transform.reset();
  {


/**************
//not working
//     eulerAngles.m_x = atan2( rotationAxis.m_y, rotationAxis.m_z );
//     if (rotationAxis.m_z >= 0) {
//        eulerAngles.m_y = -atan2( rotationAxis.m_x * cos(eulerAngles.m_x), rotationAxis.m_z );
//     }else{
//        eulerAngles.m_y = atan2( rotationAxis.m_x * cos(eulerAngles.m_x), -rotationAxis.m_z );
//     }

//     eulerAngles.m_z = atan2( cos(eulerAngles.m_x), sin(eulerAngles.m_x) * sin(eulerAngles.m_y) );


     //
//     eulerAngles.m_x= 0;
//     eulerAngles.m_y = atan2((v1-v2).m_x, (v1-v2).m_z);
//     float padj = sqrt(pow((v1-v2).m_x, 2) + pow((v1-v2).m_z, 2));
//     eulerAngles.m_y = atan2(padj, (v1-v2).m_y) ;

  **************/
     //convert axis anle to euler angles
//     toEuler(rotationAxis.m_x, rotationAxis.m_y, rotationAxis.m_z, angle);
//     m_transform.setRotation(eulerAngles.m_x*(180/M_PI),eulerAngles.m_y*(180/M_PI),eulerAngles.m_z*(180/M_PI));
       ngl::Mat4 trs=m_transform.getMatrix();


     rotateMat=matrixFromAxisAngle(rotationAxis,angle);
     translateMat.translate(v2NonNormalized.m_x,v2NonNormalized.m_y,v2NonNormalized.m_z);



//     std::cout<<"mat Matrix():\n"<<"  "<<rotateMat.m_00<<"  "<< rotateMat.m_01<<"  "<<rotateMat.m_02 <<"  "<<rotateMat.m_03<<"  "<<
//                                    rotateMat.m_10<<"  "<< rotateMat.m_11<<"  "<<rotateMat.m_12 <<"  "<<rotateMat.m_13<<"  "<< rotateMat.m_20<<"  "<< rotateMat.m_21<<"  "<<rotateMat.m_22 <<"  "<<rotateMat.m_23<<"  "<< rotateMat.m_30<<"  "<<
//                                    rotateMat.m_31<<"  "<<rotateMat.m_32 <<"  "<<rotateMat.m_33<<"  "<<std::endl;

//     std::cout<<angle<<std::endl;

      M=rotateMat*translateMat/**scaleMat*/;

      M=/*m_transform.getMatrix()*/ M/* trs*/*m_mouseGlobalTX;
      MV=  M*m_cam->getViewMatrix();
      MVP= M*m_cam->getVPMatrix();
      normalMatrix=MV;
      normalMatrix.inverse();
      shader->setShaderParamFromMat4("MV",MV);
      shader->setShaderParamFromMat4("MVP",MVP);
      shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
      shader->setShaderParamFromMat4("M",M);


      m_vao->bind();
      m_vao->draw();
      m_vao->unbind();
  }


}



ngl::Mat4 NGLScene::matrixFromAxisAngle(ngl::Vec3 axis, float angle) {

    ngl::Mat4 tmp=1;

    double c = cos(angle);
    double s = sin(angle);
    double t = 1.0 - c;
    //  if axis is not already normalised then uncomment this
    // double magnitude = sqrt(axis.x*axis.x + axis.m_y*axis.m_y + axis.m_z*axis.m_z);
    // if (magnitude==0) throw error;
    // axis.x /= magnitude;
    // axis.m_y /= magnitude;
    // axis.m_z /= magnitude;

    tmp.m_00 = c + axis.m_x*axis.m_x*t;
    tmp.m_11 = c + axis.m_y*axis.m_y*t;
    tmp.m_22 = c + axis.m_z*axis.m_z*t;


    double tmp1 = axis.m_x*axis.m_y*t;
    double tmp2 = axis.m_z*s;
    tmp.m_10 = tmp1 + tmp2;
    tmp.m_01 = tmp1 - tmp2;
    tmp1 = axis.m_x*axis.m_z*t;
    tmp2 = axis.m_y*s;
    tmp.m_20 = tmp1 - tmp2;
    tmp.m_02 = tmp1 + tmp2;
    tmp1 = axis.m_y*axis.m_z*t;
    tmp2 = axis.m_x*s;
    tmp.m_21 = tmp1 + tmp2;
    tmp.m_12 = tmp1 - tmp2;

    return tmp;
}


//Heading = rotation about y axis
//Attitude = rotation about z axis
//Bank = rotation about x axis
void NGLScene::toEuler(double x,double y,double z,double angle) {
    float heading;
    float attitude;
    float bank;
    double s=sin(angle);
    double c=cos(angle);
    double t=1-c;
    //  if axis is not already normalised then uncomment this
    // double magnitude = sqrt(x*x + y*y + z*z);
    // if (magnitude==0) throw error;
    // x /= magnitude;
    // y /= magnitude;
    // z /= magnitude;
    if ((x*y*t + z*s) > 0.998) { // north pole singularity detected
        heading = 2*atan2(x*sin(angle/2),cos(angle/2));
        attitude = M_PI/2;
        bank = 0;
        return;
    }
    if ((x*y*t + z*s) < -0.998) { // south pole singularity detected
        heading = -2*atan2(x*sin(angle/2),cos(angle/2));
        attitude = -M_PI/2;
        bank = 0;
        return;
    }

    heading = atan2(y * s- x * z * t , 1 - (y*y+ z*z ) * t);
    attitude = asin(x * y * t + z * s) ;
    bank = atan2(x * s - y * z * t , 1 - (x*x + z*z) * t);

    eulerAngles.set(bank,heading,attitude);
}



//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += (float) 0.5f * diffy;
    m_spinYFace += (float) 0.5f * diffx;
    m_origX = _event->x();
    m_origY = _event->y();
    renderLater();

  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = (int)(_event->x() - m_origXPos);
    int diffY = (int)(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    renderLater();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
	renderLater();
}
//----------------------------------------------------------------------------------------------------------------------



void NGLScene::timerEvent( QTimerEvent *_event )
{
    if(_event->timerId() == m_sphereUpdateTimer)
    {
        if(currentTime.elapsed() > 50)//update every 100 millisecs
        {


            if(v1Xcoord>endLerp)
            {
                directionFlag=-directionFlag;
                v1Xcoord-=directionFlag*0.1;//make sure corner cases behave correctly

            }
            if(v1Xcoord<startLerp)
            {
                directionFlag=-directionFlag;
                v1Xcoord+=directionFlag*0.1;//make sure corner cases behave correctly
            }

            increment = directionFlag * increment;
            v1Xcoord+=startLerp + increment*(endLerp-startLerp);




//            v1Xcoord+=0.1;



//            m_fps=m_frames;
//            m_frames=0;
            currentTime.restart();
        }


        //update();
        renderNow();
    }
}

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  default : break;
  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    renderLater();
}
