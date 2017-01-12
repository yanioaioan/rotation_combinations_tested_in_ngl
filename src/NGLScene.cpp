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

#include <ngl/VAOFactory.h>
#include <ngl/SimpleIndexVAO.h>

//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=1;

NGLScene::NGLScene()
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
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";


  m_vao->removeVAO();
  m_vao2->removeVAO();
}

void NGLScene::resizeGL(int _w, int _h)
{
  // set the viewport for openGL
  glViewport(0,0,_w,_h);
  // now set the camera size values as the screen size has changed
  m_cam->setShape(45,(float)_w/_h,0.05,350);
  update();
}


void NGLScene::initializeGL()
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
  shader->attachShader("PhongVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("PhongFragment",ngl::ShaderType::FRAGMENT);
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




  shader->createShaderProgram("Colour");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("ColourVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("ColourFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("ColourVertex","shaders/ColourVertex.glsl");
  shader->loadShaderSource("ColourFragment","shaders/ColourFragment.glsl");
  // compile the shaders
  shader->compileShader("ColourVertex");
  shader->compileShader("ColourFragment");
  // add them to the program
  shader->attachShaderToProgram("Colour","ColourVertex");
  shader->attachShaderToProgram("Colour","ColourFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("Colour");
  // and make it active ready to load values
  (*shader)["Phong"]->use();

  //  (*shader)["Colour"]->use();



  // the shader will use the currently active material and light0 so set them
  ngl::Material m(ngl::STDMAT::GOLD);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");
  shader->setShaderParam3f("viewerPos",m_cam->getEye().m_x,m_cam->getEye().m_y,m_cam->getEye().m_z);
  // now create our light this is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  ngl::Mat4 iv=m_cam->getViewMatrix();
  iv.transpose();
  iv=iv.inverse();
  ngl::Light l(ngl::Vec3(0,1,0),ngl::Colour(1,1,1,1),ngl::Colour(1,1,1,1),ngl::LightModes::POINTLIGHT);
  l.setTransform(iv);
  // load these values to the shader as well
  l.loadToShader("light");

  buildVAO();
  buildVAO2();

  glViewport(0,0,width(),height());
}

void NGLScene::buildVAO()
{
//  // create a vao as a series of GL_TRIANGLES
//  m_vao= ngl::VertexArrayObject::createVOA(GL_TRIANGLES);
//  m_vao->bind();



//  const static GLubyte indices[]=  {
////                                      0,1,5,0,4,5, // back
////                                      3,2,6,7,6,3, // front
////                                      0,1,2,3,2,0, // top
////                                      4,5,6,7,6,4, // bottom
////                                      0,3,4,4,7,3,
////                                      1,5,2,2,6,5



//                      0,1,2, 2,3,0,   // first half (18 indices)
//                      0,3,4, 4,5,0,
//                      0,5,6, 6,1,0,

//                      1,6,7, 7,2,1,   // second half (18 indices)
//                      7,4,3, 3,2,7,
//                      4,7,6, 6,5,4

//                                   };




//   GLfloat vertices[] = {/*-1,1,-1,
//                         1,1,-1,
//                         1,1,1,
//                         -1,1,1,r
//                         -1,-1,-1,
//                         1,-1,-1,
//                         1,-1,1,
//                         -1,-1,1*/

//                         (-1.0f, -1.0f, 1.0f),//fbl 0
//                         (1.0f, -1.0f, 1.0f),//fbr 1
//                         (1.0f, 1.0f, 1.0f),//fur 2
//                         (-1.0f, 1.0f, 1.0f),//ful 3
//                         (-1.0f, -1.0f, -1.0f),//bbl 4
//                         (1.0f, -1.0f, -1.0f),//bbr 5
//                         (1.0f, 1.0f, -1.0f),//bur 6
//                         (-1.0f, 1.0f, -1.0f)//bul 7



//                        };



//   GLfloat colours[]={
//                        1,0,0,
//                        0,1,0,
//                        0,0,1,
//                        1,1,1,
//                        0,0,1,
//                        0,1,0,
//                        1,0,0,
//                        1,1,1
//                      };


////   GLfloat normals[]={
//////                        1,0,0,
//////                        0,1,0,
//////                        0,0,1,
//////                        1,1,1,
//////                        0,0,1,
//////                        0,1,0,
//////                        1,0,0,
//////                        1,1,1


////                    (-1.0f, -1.0f, 1.0f),
////                    (1.0f, -1.0f, 1.0f),
////                    (1.0f, 1.0f, 1.0f),
////                    (-1.0f, 1.0f, 1.0f),
////                    (-1.0f, -1.0f, -1.0f),
////                    (1.0f, -1.0f, -1.0f),
////                    (1.0f, 1.0f, -1.0f),
////                    (-1.0f, 1.0f, -1.0f)


////                      };



//   // in this case we are going to set our data as the vertices above

//   m_vao->setIndexedData(24*sizeof(GLfloat),vertices[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW);
//   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
//   m_vao->setVertexAttributePointer(0,3,GL_FLOAT,0,0);
//   m_vao->setIndexedData(24*sizeof(GLfloat),colours[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW);
//   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
//   m_vao->setVertexAttributePointer(3,3,GL_FLOAT,0,0);

////   m_vao->setIndexedData(24*sizeof(GLfloat),normals[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW);
////    now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
////   m_vao->setVertexAttributePointer(2,3,GL_FLOAT,0,0);

//   m_vao->setNumIndices(sizeof(indices));

// // now unbind
//  m_vao->unbind();


      const static GLubyte indices[]=  {
                                          0,1,5,0,4,5, // back
                                          3,2,6,7,6,3, // front
                                          0,1,2,3,2,0, // top
                                          4,5,6,7,6,4, // bottom
                                          0,3,4,4,7,3,
                                          1,5,2,2,6,5
                                       };



    ngl::Vec3 verts[]=
     {
//       ngl::Vec3(0,1,1),
//       ngl::Vec3(0,0,-1),
//       ngl::Vec3(-0.5,0,1),
//       ngl::Vec3(0,1,1),
//       ngl::Vec3(0,0,-1),
//       ngl::Vec3(0.5,0,1),
//       ngl::Vec3(0,1,1),
//       ngl::Vec3(0,0,1.5),
//       ngl::Vec3(-0.5,0,1),
//       ngl::Vec3(0,1,1),
//       ngl::Vec3(0,0,1.5),
//       ngl::Vec3(0.5,0,1)

        ////                                      0,1,5,0,4,5, // back
        ////                                      3,2,6,7,6,3, // front
        ////                                      0,1,2,3,2,0, // top
        ////                                      4,5,6,7,6,4, // bottom
        ////                                      0,3,4,4,7,3,
        ////                                      1,5,2,2,6,5

        //Create all 12 triangles separately
        //6 faces > 12 triangles > 12*3 vertices > 12*3*(3each vertex)= 108 vertices total

    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
    ngl::Vec3(1.0f, -1.0f, 1.0f),//fbr 1
    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5
    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5

    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3
    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
    ngl::Vec3(-1.0f, 1.0f, -1.0f),//bul 7
    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3

    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
    ngl::Vec3(1.0f, -1.0f, 1.0f),//fbr 1
    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3
    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0

    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5
    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
    ngl::Vec3(-1.0f, 1.0f, -1.0f),//bul 7
    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4

    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3
    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
    ngl::Vec3(-1.0f, 1.0f, -1.0f),//bul 7
    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3

    ngl::Vec3(1.0f, -1.0f, 1.0f),//fbr 1
    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5
    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5





     };


//     ..3 vertices for each triangle..9 coordinates*12= 108 normals total  too
     std::vector <ngl::Vec3> normals;

     //1st face normals-bottom
     ngl::Vec3 n=ngl::calcNormal(verts[1],verts[2],verts[0]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);
     n=ngl::calcNormal(verts[4],verts[3],verts[5]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     //2nd face normals-top
     n=ngl::calcNormal(verts[7],verts[6],verts[8]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     n=ngl::calcNormal(verts[10],verts[11],verts[9]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     //3rd face normals-front
     n=ngl::calcNormal(verts[13],verts[12],verts[14]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     n=ngl::calcNormal(verts[15],verts[16],verts[17]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     //4th face normals-back
     n=ngl::calcNormal(verts[18],verts[19],verts[20]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     n=ngl::calcNormal(verts[20],verts[21],verts[23]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     //5th face normals-left
     n=ngl::calcNormal(verts[26],verts[25],verts[24]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     n=ngl::calcNormal(verts[27],verts[28],verts[29]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);

     //6th face normals-right
     n=ngl::calcNormal(verts[31],verts[30],verts[32]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);


     n=ngl::calcNormal(verts[35],verts[33],verts[34]);
     normals.push_back(n);
     normals.push_back(n);
     normals.push_back(n);



     std::cout<<"sizeof(verts) "<<sizeof(verts)<<" sizeof(ngl::Vec3) "<<sizeof(ngl::Vec3)<<"\n";
     // create a vao as a series of GL_TRIANGLES
//     m_vao= ngl::VertexArrayObject::createVOA(GL_TRIANGLES);
//     m_vao.reset(ngl::VertexArrayObject::createVOA(GL_TRIANGLES));

     m_vao.reset( ngl::VAOFactory::createVAO("simpleIndexVAO",GL_TRIANGLES));
     m_vao->bind();

     // in this case we are going to set our data as the vertices above



     //   m_vao->setData(ngl::SimpleIndexVAO::VertexData( 24*sizeof(GLfloat),vertices[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
     //   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
     //   m_vao->setVertexAttributePointer(0,3,GL_FLOAT,0,0);
     //   m_vao->setData(ngl::SimpleIndexVAO::VertexData(24*sizeof(GLfloat),colours[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
     //   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
     //   m_vao->setVertexAttributePointer(1,3,GL_FLOAT,0,0);
     //   m_vao->setNumIndices(sizeof(indices));
     // // now unbind
     //  m_vao->unbind();


       m_vao->setData(ngl::SimpleIndexVAO::VertexData(36*sizeof(ngl::Vec3),verts[0].m_x,sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
       // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)

       m_vao->setVertexAttributePointer(0,3,GL_FLOAT,0,0);

       m_vao->setData(ngl::SimpleIndexVAO::VertexData(normals.size()*sizeof(ngl::Vec3),normals[0].m_x,sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
       // now we set the attribute pointer to be 2 (as this matches normal in our shader)

       m_vao->setVertexAttributePointer(2,3,GL_FLOAT,0,0);


//       m_vao->setNumIndices(sizeof(verts)/sizeof(ngl::Vec3));
       m_vao->setNumIndices(sizeof(indices));

    // now unbind
     m_vao->unbind();
}



//void NGLScene::buildVAO()
//{
//  // create a vao as a series of GL_TRIANGLES
//  m_vao.reset( ngl::VAOFactory::createVAO("simpleIndexVAO",GL_TRIANGLES));
//  m_vao->bind();


//  const static GLubyte indices[]=  {
//                                      0,1,5,0,4,5, // back
//                                      3,2,6,7,6,3, // front
//                                      0,1,2,3,2,0, // top
//                                      4,5,6,7,6,4, // bottom
//                                      0,3,4,4,7,3,
//                                      1,5,2,2,6,5
//                                   };

//   GLfloat vertices[] = {-1,1,-1,
//                         1,1,-1,
//                         1,1,1,
//                         -1,1,1,
//                         -1,-1,-1,
//                         1,-1,-1,
//                         1,-1,1,
//                         -1,-1,1
//                        };

//   GLfloat colours[]={
//                        1,0,0,
//                        0,1,0,
//                        0,0,1,
//                        1,1,1,
//                        0,0,1,
//                        0,1,0,
//                        1,0,0,
//                        1,1,1
//                      };
//   // in this case we are going to set our data as the vertices above

//   GLuint buffer;
//   glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer);


//   m_vao->setData(ngl::SimpleIndexVAO::VertexData( 24*sizeof(GLfloat),vertices[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
//   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
//   m_vao->setVertexAttributePointer(0,3,GL_FLOAT,0,0);
//   m_vao->setData(ngl::SimpleIndexVAO::VertexData(24*sizeof(GLfloat),colours[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
//   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
//   m_vao->setVertexAttributePointer(1,3,GL_FLOAT,0,0);
//   m_vao->setNumIndices(sizeof(indices));
// // now unbind
//  m_vao->unbind();

//}


//void NGLScene::buildVAO2()
//{

//    ngl::Vec3 verts[]=
//     {


//        //Create all 12 triangles separately
//        //6 faces > 12 triangles > 12*3 vertices > 12*3*(3each vertex)= 108 vertices total

//    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
//    ngl::Vec3(1.0f, -1.0f, 1.0f),//fbr 1
//    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5
//    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
//    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
//    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5

//    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3
//    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
//    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
//    ngl::Vec3(-1.0f, 1.0f, -1.0f),//bul 7
//    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
//    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3

//    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
//    ngl::Vec3(1.0f, -1.0f, 1.0f),//fbr 1
//    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
//    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3
//    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
//    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0

//    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
//    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5
//    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
//    ngl::Vec3(-1.0f, 1.0f, -1.0f),//bul 7
//    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
//    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4

//    ngl::Vec3(-1.0f, -1.0f, 1.0f),//fbl 0
//    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3
//    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
//    ngl::Vec3(-1.0f, -1.0f, -1.0f),//bbl 4
//    ngl::Vec3(-1.0f, 1.0f, -1.0f),//bul 7
//    ngl::Vec3(-1.0f, 1.0f, 1.0f),//ful 3

//    ngl::Vec3(1.0f, -1.0f, 1.0f),//fbr 1
//    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5
//    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
//    ngl::Vec3(1.0f, 1.0f, 1.0f),//fur 2
//    ngl::Vec3(1.0f, 1.0f, -1.0f),//bur 6
//    ngl::Vec3(1.0f, -1.0f, -1.0f),//bbr 5



//     };


////     ..3 vertices for each triangle..9 coordinates*12= 108 normals total  too
//     std::vector <ngl::Vec3> normals;

//     //1st face normals-bottom
//     ngl::Vec3 n=ngl::calcNormal(verts[1],verts[2],verts[0]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);
//     n=ngl::calcNormal(verts[4],verts[3],verts[5]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     //2nd face normals-top
//     n=ngl::calcNormal(verts[7],verts[6],verts[8]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     n=ngl::calcNormal(verts[10],verts[11],verts[9]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     //3rd face normals-front
//     n=ngl::calcNormal(verts[13],verts[12],verts[14]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     n=ngl::calcNormal(verts[15],verts[16],verts[17]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     //4th face normals-back
//     n=ngl::calcNormal(verts[18],verts[19],verts[20]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     n=ngl::calcNormal(verts[20],verts[21],verts[23]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     //5th face normals-left
//     n=ngl::calcNormal(verts[26],verts[25],verts[24]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     n=ngl::calcNormal(verts[27],verts[28],verts[29]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);

//     //6th face normals-right
//     n=ngl::calcNormal(verts[31],verts[30],verts[32]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);


//     n=ngl::calcNormal(verts[35],verts[33],verts[34]);
//     normals.push_back(n);
//     normals.push_back(n);
//     normals.push_back(n);



//     std::cout<<"sizeof(verts) "<<sizeof(verts)<<" sizeof(ngl::Vec3) "<<sizeof(ngl::Vec3)<<"\n";
//     // create a vao as a series of GL_TRIANGLES
//     m_vao2.reset(ngl::VertexArrayObject::createVOA(GL_TRIANGLES));
//     m_vao2->bind();

//     // in this case we are going to set our data as the vertices above

//       m_vao2->setData(sizeof(verts),verts[0].m_x);
//       // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)

//       m_vao2->setVertexAttributePointer(0,3,GL_FLOAT,0,0);

//       m_vao2->setData(normals.size()*sizeof(ngl::Vec3),normals[0].m_x);
//       // now we set the attribute pointer to be 2 (as this matches normal in our shader)

//       m_vao2->setVertexAttributePointer(2,3,GL_FLOAT,0,0);

//       m_vao2->setNumIndices(sizeof(verts)/sizeof(ngl::Vec3));

//    // now unbind
//     m_vao2->unbind();



//}



void NGLScene::buildVAO2()
{
  // create a vao as a series of GL_TRIANGLES
  m_vao2.reset( ngl::VAOFactory::createVAO("simpleIndexVAO",GL_TRIANGLES));
  m_vao2->bind();


  const static GLubyte indices[]=  {
                                      0,1,5,0,4,5, // back
                                      3,2,6,7,6,3, // front
                                      0,1,2,3,2,0, // top
                                      4,5,6,7,6,4, // bottom
                                      0,3,4,4,7,3,
                                      1,5,2,2,6,5
                                   };

   GLfloat vertices[] = {-1,1,-1,
                         1,1,-1,
                         1,1,1,
                         -1,1,1,
                         -1,-1,-1,
                         1,-1,-1,
                         1,-1,1,
                         -1,-1,1
                        };

   GLfloat colours[]={
                        1,0,0,
                        0,1,0,
                        0,0,1,
                        1,1,1,
                        0,0,1,
                        0,1,0,
                        1,0,0,
                        1,1,1
                      };
   // in this case we are going to set our data as the vertices above

   m_vao2->setData(ngl::SimpleIndexVAO::VertexData( 24*sizeof(GLfloat),vertices[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
   m_vao2->setVertexAttributePointer(0,3,GL_FLOAT,0,0);
   m_vao2->setData(ngl::SimpleIndexVAO::VertexData(24*sizeof(GLfloat),colours[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
   // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
   m_vao2->setVertexAttributePointer(1,3,GL_FLOAT,0,0);
   m_vao2->setNumIndices(sizeof(indices));
 // now unbind
  m_vao2->unbind();

}



static int t=0;
static float v1Xcoord;
const float  startLerp=0.0f;
const float  endLerp=5.0f;
static float increment=0.01f;
static int directionFlag=1;


ngl::Mat4 translateMat=1;
ngl::Mat4 translateMat2=1;

ngl::Mat4 scaleMat=1;
ngl::Mat4 rotateMat=1;

static float testangle;


 void NGLScene::toEuler(double x,double y,double z,double angle, double &heading, double &attitude, double &bank)
 {
    double s=sin(angle);
    double c=cos(angle);
    double t=1-c;
    //  if axis is not already normalised then uncomment this
    // double magnitude = Math.sqrt(x*x + y*y + z*z);
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
}

float vary=1;
ngl::Vec3 v1;
ngl::Vec3 v2;


//return shortest arc quaternion that rotates start to dest
 ngl::Quaternion NGLScene::RotationBetweenVectors(ngl::Vec3 start, ngl::Vec3  dest){

     ngl::Quaternion q;

     (start).normalize();

     (dest).normalize();



     float cosTheta = start.dot(dest);

     ngl::Vec3  rotationAxis;


     /**
      * https://bitbucket.org/sinbad/ogre/src/9db75e3ba05c/OgreMain/include/OgreVector3.h?fileviewer=file-view-default#cl-651
      *
      * Gets the shortest arc quaternion to rotate this vector to the destination
                vector.
            @remarks
                If you call this with a dest vector that is close to the inverse
                of this vector, we will rotate 180 degrees around the 'fallbackAxis'
                (if specified, or a generated axis if not) since in this case
                ANY axis of rotation is valid.
 */
     if (cosTheta >= 1.0f)//same vectors
     {
         return ngl::Quaternion();//identity quaternion
     }

     if (cosTheta < (1e-6f - 1.0f))
     {
         // Generate an axis
         rotationAxis = ngl::Vec3 (0.0f, 0.0f, 1.0f).cross( start);

         if (rotationAxis.length()==0) // pick another if colinear
             rotationAxis = ngl::Vec3 (0.0f, 1.0f, 0.0f).cross( start);

         rotationAxis.normalize();
         q.fromAxisAngle(rotationAxis,180.0f);
     }




 //    if (cosTheta < -1 + 0.001f)
 //    {

 //        // special case when vectors in opposite directions:

 //        // there is no "ideal" rotation axis

 //        // So guess one; any will do as long as it's perpendicular to start

 //        rotationAxis = ngl::Vec3 (0.0f, 0.0f, 1.0f).cross( start);

 //        float t=rotationAxis.lengthSquared();
 //        if (t< 0.01 ) // bad luck, they were parallel, try again!

 //            rotationAxis = ngl::Vec3 (1.0f, 0.0f, 0.0f).cross( start);


 //        (rotationAxis).normalize();

 //        q.fromAxisAngle(rotationAxis,180.0f);

 //        return q;
 //    }



     rotationAxis = start.cross(dest);



     float s = sqrt( (1+cosTheta)*2 );

     float invs = 1 / s;



     return ngl::Quaternion(

         s * 0.5f,

         rotationAxis.m_x * invs,

         rotationAxis.m_y * invs,

         rotationAxis.m_z * invs

     );


  }



void NGLScene::paintGL()
{

    if(testangle==89)
        vary=-1;

    if(testangle==-89)
        vary=1;

//This bit has been  MOVED TO TIMER EVENT for more 'slow-motion' control
//    testangle+=vary;
    std::cout<<testangle<<std::endl;

    ngl::Vec3 v1(-5+15*sin((testangle)*(M_PI/180))-2,-5+15*sin((testangle)*(M_PI/180)),  4*sin((testangle)*(M_PI/180))-2);
    ngl::Vec3 v2(-4,0.01,-5+15*sin((testangle)*(M_PI/180)));//transform the triangle vao to 2,2,0

      ngl::Vec3 v2NonNormalized=v2;
      ngl::Vec3 v1NonNormalized=v1;


  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // Rotation based on the mouse position for our global transform

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
//  (*shader)["Colour"]->use();

  ngl::Material m(ngl::STDMAT::PEWTER);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;


  //*********
  m_transform.reset();
  //draw box
  {
      m_transform.setPosition(v1NonNormalized);
      M=m_transform.getMatrix()*m_mouseGlobalTX;
      MV=  M*m_cam->getViewMatrix();
      MVP= M*m_cam->getVPMatrix();
      normalMatrix=MV;
      normalMatrix.inverse();
      shader->setShaderParamFromMat4("MV",MV);
      shader->setShaderParamFromMat4("MVP",MVP);
      shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
      shader->setShaderParamFromMat4("M",M);


      //ngl::VAOPrimitives::instance()->draw("cube");
      m_vao2->bind();
      m_vao2->draw();
      m_vao2->unbind();

  }


    v1.normalize();
    v2.normalize();
    float angle = /*atan2(v1.m_y,v1.m_x) - atan2(v2.m_y,v2.m_x);//*/acos(v1.dot(v2));

    ngl::Vec3 rotationAxis = v1.cross(v2);
    rotationAxis.normalize();

//    ngl::Quaternion q ;
//    q.fromAxisAngle(rotationAxis,angle);

    ngl::Mat4 s,rotateMat,translateMat;
    s=1;

    //Use either RotationBetweenVectors or matrixFromAxisAngle
    rotateMat=RotationBetweenVectors(v2,v1).toMat4();
    //rotateMat=matrixFromAxisAngle(rotationAxis,angle);//q.toMat4();


    //calculate euler angles from axis-angle
//    double heading,attitude,bank;
//    toEuler(rotationAxis.m_x, rotationAxis.m_y, rotationAxis.m_z, angle, heading, attitude, bank);

//    m_transform.reset();
//    std::cout<<bank*(180/M_PI)<<","<<heading*(180/M_PI)<<","<<attitude*(180/M_PI)<<","<<std::endl;
//    m_transform.setRotation(bank*(180/M_PI),heading*(180/M_PI),attitude*(180/M_PI));
//    r= m_transform.getMatrix();

    m_transform.reset();
    m_transform.setPosition(v2NonNormalized);
    translateMat= m_transform.getMatrix();

    ngl::Mat4 modelmatrix=s*rotateMat*translateMat;

  m_transform.reset();
  //draw triangle
  {
      //    load our material values to the shader into the structure material (see Vertex shader)
      m.set(ngl::STDMAT::BRONZE);
      m.loadToShader("material");

      M=/*m_transform.getMatrix()*/  modelmatrix*m_mouseGlobalTX;
      MV=  M*m_cam->getViewMatrix();
      MVP= M*m_cam->getVPMatrix();
      normalMatrix=MV;
      normalMatrix.inverse();
      shader->setShaderParamFromMat4("MV",MV);
      shader->setShaderParamFromMat4("MVP",MVP);
      shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
      shader->setShaderParamFromMat4("M",M);


//      ngl::VAOPrimitives::instance()->draw("cube");
      m_vao->bind();
      m_vao->draw();
      m_vao->unbind();

   }



//    //draw the tip-cube of the triangle
//  {
//    m.set(ngl::GOLD);
//    // load our material values to the shader into the structure material (see Vertex shader)
//    m.loadToShader("material");

//    translateMat=1;

//  //  rotateMat=1;
//  //  scaleMat=1;

//  //not working
//  //  ngl::Vec3 v=v1.cross(v2);
//  //  float c=v1.dot(v2);
//  //  float h=1-c/v.dot(v);
//  //  rotateMat=ngl::Mat4(c*h*v.m_x*v.m_x,                 h*v.m_x*v.m_y-v.m_z,           h*v.m_x*v.m_z+v.m_y, 1,
//  //                      h*v.m_x*v.m_y+v.m_z,             c+h*v.m_y*v.m_y,               h*v.m_y*v.m_z-v.m_x, 1,
//  //                      h*v.m_x*v.m_z-v.m_y,       h*v.m_y*v.m_z+v.m_x,                c+h*v.m_z*v.m_z,      1,
//  //                      0 ,                            0 ,                                     0,            1);
//  //  rotateMat.transpose();
//  //not working



//  //     ngl::Mat4 trs=m_transform.getMatrix();
//  //     rotateMat=matrixFromAxisAngle(rotationAxis,angle);
//  //     translateMat.translate(-v2NonNormalized.m_x,-(v2NonNormalized.m_y),-v2NonNormalized.m_z);

//    //Based on [R] = [T].inverse * [R0] * [T] //http://www.euclideanspace.com/maths/geometry/affine/aroundPoint/
//    /*
//      translate the arbitrary point to the origin (subtract P which is translate by -Px,-Py,-Pz)
//      rotate about the origin (can use 3×3 matrix R0)
//      then translate back. (add P which is translate by +Px,+Py,+Pz)
//    */
//       translateMat.inverse();//step 1.. translate pointToRotate to origin

//       //(rotation matrix) - same as the triangle's "rotateMat" //step 2 rotate..

//       translateMat2.translate(v2NonNormalized.m_x,(v2NonNormalized.m_y),v2NonNormalized.m_z);//step3 ..translate pointToRotate back to its original position in 3d space

//       std::cout<<translateMat2.m_30<<","<<translateMat2.m_31<<","<<translateMat2.m_32<<std::endl;



//  //     std::cout<<"mat Matrix():\n"<<"  "<<rotateMat.m_00<<"  "<< rotateMat.m_01<<"  "<<rotateMat.m_02 <<"  "<<rotateMat.m_03<<"  "<<
//  //                                    rotateMat.m_10<<"  "<< rotateMat.m_11<<"  "<<rotateMat.m_12 <<"  "<<rotateMat.m_13<<"  "<< rotateMat.m_20<<"  "<< rotateMat.m_21<<"  "<<rotateMat.m_22 <<"  "<<rotateMat.m_23<<"  "<< rotateMat.m_30<<"  "<<
//  //                                    rotateMat.m_31<<"  "<<rotateMat.m_32 <<"  "<<rotateMat.m_33<<"  "<<std::endl;

//  //     std::cout<<angle<<std::endl;

//       //place one one sphere-primitive in the tip of the triangle, but we translate first and then rotate (this effectively shows the "trajectory of the triangle rotation")

//       /*
//        * In order to calculate the rotation about any arbitrary point we need to calculate its new rotation and translation.
//        * In other words rotation about a point is an 'proper' isometry transformation' which means that it has a linear
//        *  and a rotational component.
//        * [resulting transform] = [+Px,+Py,+Pz] * [rotation] * [-Px,-Py,-Pz]
//        */


//        M=  translateMat*rotateMat*translateMat2 /**scaleMat*/; //in ngl multiplication happens from left to right

//        M= M*m_mouseGlobalTX;
//        MV=  M*m_cam->getViewMatrix();
//        MVP= M*m_cam->getVPMatrix();
//        normalMatrix=MV;
//        normalMatrix.inverse();
//        shader->setShaderParamFromMat4("MV",MV);
//        shader->setShaderParamFromMat4("MVP",MVP);
//        shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
//        shader->setShaderParamFromMat4("M",M);

////        ngl::VAOPrimitives::instance()->createSphere("mysphere",0.1,10);

//        ngl::VAOPrimitives::instance()->draw("cube");

//  }















//  //*********NOW********* STEP 2
//  //Calculate rotation vector from 2nd to 1st triangle
//  //then

//  m.set(ngl::BRONZE);
//  // load our material values to the shader into the structure material (see Vertex shader)
//  m.loadToShader("material");

//  //... rotate and draw the 2nd triangle as well
//  m_transform.reset();
//  {

////------------------------------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------------------------------
///**************
////not working
////     eulerAngles.m_x = atan2( rotationAxis.m_y, rotationAxis.m_z );
////     if (rotationAxis.m_z >= 0) {
////        eulerAngles.m_y = -atan2( rotationAxis.m_x * cos(eulerAngles.m_x), rotationAxis.m_z );
////     }else{
////        eulerAngles.m_y = atan2( rotationAxis.m_x * cos(eulerAngles.m_x), -rotationAxis.m_z );
////     }

////     eulerAngles.m_z = atan2( cos(eulerAngles.m_x), sin(eulerAngles.m_x) * sin(eulerAngles.m_y) );


//     //
////     eulerAngles.m_x= 0;
////     eulerAngles.m_y = atan2((v1-v2).m_x, (v1-v2).m_z);
////     float padj = sqrt(pow((v1-v2).m_x, 2) + pow((v1-v2).m_z, 2));
////     eulerAngles.m_y = atan2(padj, (v1-v2).m_y) ;

//  **************/
//     //convert axis anle to euler angles
////     toEuler(rotationAxis.m_x, rotationAxis.m_y, rotationAxis.m_z, angle);
////     m_transform.setRotation(eulerAngles.m_x*(180/M_PI),eulerAngles.m_y*(180/M_PI),eulerAngles.m_z*(180/M_PI));
//       ngl::Mat4 trs=m_transform.getMatrix();
////------------------------------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------------------------------



//    //The following work with ngl::Transformation too, as well as with individual matrices
//    //       m_transform.reset();
//    //       if(testangle<360)
//    //           testangle++;
//    //       else
//    //       {
//    //           testangle=0;
//    //       }

//    //       m_transform.setRotation(testangle,0,0);

//    //Rotate based where v1 is (make v2NonNormalized(triangle) to point towards v1-cube)
//     rotateMat=matrixFromAxisAngle(rotationAxis,angle);//m_transform.getMatrix();
//     translateMat.translate(v2NonNormalized.m_x,v2NonNormalized.m_y,v2NonNormalized.m_z);


////     std::cout<<"mat Matrix():\n"<<"  "<<rotateMat.m_00<<"  "<< rotateMat.m_01<<"  "<<rotateMat.m_02 <<"  "<<rotateMat.m_03<<"  "<<
////                                    rotateMat.m_10<<"  "<< rotateMat.m_11<<"  "<<rotateMat.m_12 <<"  "<<rotateMat.m_13<<"  "<< rotateMat.m_20<<"  "<< rotateMat.m_21<<"  "<<rotateMat.m_22 <<"  "<<rotateMat.m_23<<"  "<< rotateMat.m_30<<"  "<<
////                                    rotateMat.m_31<<"  "<<rotateMat.m_32 <<"  "<<rotateMat.m_33<<"  "<<std::endl;
////     std::cout<<angle<<std::endl;




////not quite working
//        float norm_u_norm_v = sqrt(v2NonNormalized.lengthSquared() * v1NonNormalized.lengthSquared());
//        ngl::Vec3 w = v2.cross(v1);
//        ngl::Quaternion q = ngl::Quaternion(norm_u_norm_v + v2NonNormalized.dot(v1NonNormalized), w.m_x, w.m_y, w.m_z);
//        q.normalise();
//        rotateMat=q.toMat4();

////not quite working either
//     rotateMat=ngl::lookAt(v2NonNormalized,v1NonNormalized,ngl::Vec3(0,1,0));



//      M=rotateMat*translateMat;//left to right multiplication in ngl (first rotate then translate)

//      M=/*m_transform.getMatrix()*/ M/* trs*/*m_mouseGlobalTX;
//      MV=  M*m_cam->getViewMatrix();
//      MVP= M*m_cam->getVPMatrix();
//      normalMatrix=MV;
//      normalMatrix.inverse();
//      shader->setShaderParamFromMat4("MV",MV);
//      shader->setShaderParamFromMat4("MVP",MVP);
//      shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
//      shader->setShaderParamFromMat4("M",M);


//      m_vao->bind();
//      m_vao->draw();//draw triangle now
//      m_vao->unbind();
//  }


//  //*********NOW********* STEP 3

//  //draw the tip-cube of the triangle
//{
//  m.set(ngl::GOLD);
//  // load our material values to the shader into the structure material (see Vertex shader)
//  m.loadToShader("material");

//  translateMat=1;

////  rotateMat=1;
////  scaleMat=1;

////not working
////  ngl::Vec3 v=v1.cross(v2);
////  float c=v1.dot(v2);
////  float h=1-c/v.dot(v);
////  rotateMat=ngl::Mat4(c*h*v.m_x*v.m_x,                 h*v.m_x*v.m_y-v.m_z,           h*v.m_x*v.m_z+v.m_y, 1,
////                      h*v.m_x*v.m_y+v.m_z,             c+h*v.m_y*v.m_y,               h*v.m_y*v.m_z-v.m_x, 1,
////                      h*v.m_x*v.m_z-v.m_y,       h*v.m_y*v.m_z+v.m_x,                c+h*v.m_z*v.m_z,      1,
////                      0 ,                            0 ,                                     0,            1);
////  rotateMat.transpose();
////not working



////     ngl::Mat4 trs=m_transform.getMatrix();
////     rotateMat=matrixFromAxisAngle(rotationAxis,angle);
////     translateMat.translate(-v2NonNormalized.m_x,-(v2NonNormalized.m_y),-v2NonNormalized.m_z);

//  //Based on [R] = [T].inverse * [R0] * [T] //http://www.euclideanspace.com/maths/geometry/affine/aroundPoint/
//  /*
//    translate the arbitrary point to the origin (subtract P which is translate by -Px,-Py,-Pz)
//    rotate about the origin (can use 3×3 matrix R0)
//    then translate back. (add P which is translate by +Px,+Py,+Pz)
//  */
//     translateMat.inverse();//step 1.. translate pointToRotate to origin

//     //(rotation matrix) - same as the triangle's "rotateMat" //step 2 rotate..

//     translateMat2.translate(v2NonNormalized.m_x,(v2NonNormalized.m_y),v2NonNormalized.m_z);//step3 ..translate pointToRotate back to its original position in 3d space

//     std::cout<<translateMat2.m_30<<","<<translateMat2.m_31<<","<<translateMat2.m_32<<std::endl;



////     std::cout<<"mat Matrix():\n"<<"  "<<rotateMat.m_00<<"  "<< rotateMat.m_01<<"  "<<rotateMat.m_02 <<"  "<<rotateMat.m_03<<"  "<<
////                                    rotateMat.m_10<<"  "<< rotateMat.m_11<<"  "<<rotateMat.m_12 <<"  "<<rotateMat.m_13<<"  "<< rotateMat.m_20<<"  "<< rotateMat.m_21<<"  "<<rotateMat.m_22 <<"  "<<rotateMat.m_23<<"  "<< rotateMat.m_30<<"  "<<
////                                    rotateMat.m_31<<"  "<<rotateMat.m_32 <<"  "<<rotateMat.m_33<<"  "<<std::endl;

////     std::cout<<angle<<std::endl;

//     //place one one sphere-primitive in the tip of the triangle, but we translate first and then rotate (this effectively shows the "trajectory of the triangle rotation")

//     /*
//      * In order to calculate the rotation about any arbitrary point we need to calculate its new rotation and translation.
//      * In other words rotation about a point is an 'proper' isometry transformation' which means that it has a linear
//      *  and a rotational component.
//      * [resulting transform] = [+Px,+Py,+Pz] * [rotation] * [-Px,-Py,-Pz]
//      */


//      M=  translateMat*rotateMat*translateMat2 /**scaleMat*/; //in ngl multiplication happens from left to right

//      M= M*m_mouseGlobalTX;
//      MV=  M*m_cam->getViewMatrix();
//      MVP= M*m_cam->getVPMatrix();
//      normalMatrix=MV;
//      normalMatrix.inverse();
//      shader->setShaderParamFromMat4("MV",MV);
//      shader->setShaderParamFromMat4("MVP",MVP);
//      shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
//      shader->setShaderParamFromMat4("M",M);

//      ngl::VAOPrimitives::instance()->createSphere("mysphere",0.1,10);

//      ngl::VAOPrimitives::instance()->draw("cube");

//}



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
    update();

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
    update();

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
    update();
}
//----------------------------------------------------------------------------------------------------------------------



void NGLScene::timerEvent( QTimerEvent *_event )
{
    if(_event->timerId() == m_sphereUpdateTimer)
    {
        if(currentTime.elapsed() > 100)//update every 100 millisecs
        {            

            testangle+=vary;




            /////////////////////////////////////////
            //Interpolation -back and forth motion bit
            /////////////////////////////////////////
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

        update();

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
    update();
}
