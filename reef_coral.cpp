#include "scene.h"
namespace abyss {
void Scene::reefCoral(Vec3 p,float size,uint32_t seed,int type) {
 const Color palette[]={{235, 80,145},{50,191,177},{166,104,221},{235,173,68},{231,124,93},{102,164,230}};
 Color color=type==6?Color{255,67,48}:type==7?Color{91,242,65}:palette[hash(seed)%6];
 Color tip=blend(color,type==7?Color{183,255,112}:Color{255,171,110},.28f);
 auto tube=[&](Vec3 a,Vec3 b,float ra,float rb,Color c) {
  Vec3 axis=unit(b-a),u=unit(cross(axis,{.3f,.9f,.2f})),v=cross(axis,u),center=(a+b)*.5f;
  Vec3 aa[3],bb[3];
  for(int k=0;k<3;++k){float t=k*2*Pi/3;Vec3 d=u*std::cos(t)+v*std::sin(t);aa[k]=a+d*ra;bb[k]=b+d*rb;}
  for(int k=0;k<3;++k){int j=(k+1)%3;quadSolid(aa[k],aa[j],bb[j],bb[k],c,(aa[k]+aa[j]+bb[j]+bb[k])*.25f-center);}
  addSolid(bb[0],bb[1],bb[2],tip,axis);
 };
 if(type==6) { // A bush of tapering branches, with pale growing tips.
  Vec3 hub=p+Vec3{0,size*.32f,0};tube(p,hub,size*.10f,size*.07f,color);
  for(int k=0;k<3;++k){float yaw=k*2*Pi/3+noise(seed)*Pi;Vec3 radial{std::cos(yaw),0,std::sin(yaw)};
   Vec3 fork=hub+radial*(size*(.29f+noise(seed+k+11)*.17f))+Vec3{0,size*(.28f+noise(seed+k+29)*.17f),0};
   tube(hub,fork,size*.067f,size*.043f,color);
   Vec3 end=fork+radial*(size*.11f)+Vec3{0,size*(.28f+noise(seed+k+41)*.16f),0};
   tube(fork,end,size*.043f,size*.014f,blend(color,tip,.18f));
   Vec3 tangent{-radial.z,0,radial.x};
   Vec3 twig=fork+radial*(size*.18f)+tangent*(size*.16f)+Vec3{0,size*.18f,0};
   tube(fork,twig,size*.034f,size*.011f,blend(color,tip,.28f));
  }
 } else if(type==7) { // Two offset tables, with an actual rim and shaded underside.
  tube(p,p+Vec3{0,size*.72f,0},size*.09f,size*.055f,scale(color,.8f));
  for(int level=0;level<2;++level){Vec3 c=p+Vec3{level*size*.23f,size*(.34f+level*.35f),level*size*.10f};float radius=size*(.72f-level*.20f);
   for(int k=0;k<6;++k){float t=k*Pi/3,u=(k+1)*Pi/3;float r0=radius*(.88f+noise(seed+k+level*13)*.17f),r1=radius*(.88f+noise(seed+(k+1)%6+level*13)*.17f);
    Vec3 a=c+Vec3{std::cos(t)*r0,0,std::sin(t)*r0},b=c+Vec3{std::cos(u)*r1,0,std::sin(u)*r1};
    addSolid(c+Vec3{0,size*.09f,0},a,b,color,{0,1,0});
    Vec3 ab=a-Vec3{0,size*.055f,0},bb=b-Vec3{0,size*.055f,0};
    quadSolid(a,ab,bb,b,tip,(a+b)*.5f-c);
    addSolid(c-Vec3{0,size*.065f,0},bb,ab,scale(color,.65f),{0,-1,0});
   }
  }
 } else if(type==8) { // Lobed massive coral: a low rounded dome, not another branch.
  const Color gold=blend({226,171,65},color,.30f);
  for(int k=0;k<10;++k){float t=k*Pi/5,u=(k+1)*Pi/5;float r=size*(.58f+noise(seed+k)*.09f),rr=size*(.58f+noise(seed+(k+1)%10)*.09f);
   Vec3 a=p+Vec3{std::cos(t)*r,0,std::sin(t)*r},b=p+Vec3{std::cos(u)*rr,0,std::sin(u)*rr};
   Vec3 m=p+Vec3{std::cos(t)*r*.80f,size*.37f,std::sin(t)*r*.80f},n=p+Vec3{std::cos(u)*rr*.80f,size*.37f,std::sin(u)*rr*.80f};
   Color shade=k%2?gold:scale(gold,.86f);quadSolid(a,b,n,m,shade,(a+b)*.5f-p);
   addSolid(p+Vec3{0,size*.57f,0},m,n,shade,{0,1,0});
  }
 } else { // An open branching fan; holes remain open, even at grazing angles.
  float yaw=noise(seed)*Pi;Vec3 sideways{std::cos(yaw),0,std::sin(yaw)};
  Color fan=blend({187, 90,213},color,.38f);
  auto ribbon=[&](Vec3 a,Vec3 b,float w){quad(a-sideways*w,a+sideways*w,b+sideways*w*.5f,b-sideways*w*.5f,fan);};
  Vec3 hub=p+Vec3{0,size*.23f,0};ribbon(p,hub,size*.055f);
  for(int k=-2;k<=2;++k){Vec3 joint=p+sideways*(k*size*.21f)+Vec3{0,size*(.68f-.055f*std::abs(k)),0};ribbon(hub,joint,size*.035f);
   for(int d=-1;d<=1;d+=2){Vec3 end=joint+sideways*(d*size*.13f)+Vec3{0,size*(.31f-.03f*std::abs(k)),0};ribbon(joint,end,size*.024f);}
  }
 }
}
}
