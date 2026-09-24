#include "scene.h"
#if defined(__GNUC__)
#pragma GCC optimize("O3", "fast-math")
#endif
namespace abyss {
void Scene::sunfishMesh(const ecology::Animal& a) {
 material_=0;float sy=std::sin(a.yaw),cy=std::cos(a.yaw);
 auto rot=[&](Vec3 p){return Vec3{p.x*cy+p.z*sy,p.y,-p.x*sy+p.z*cy};};
 auto wp=[&](Vec3 p){return a.position+rot(p)*a.length;};
 auto tri=[&](Vec3 p,Vec3 q,Vec3 r,Color c){add(wp(p),wp(q),wp(r),c);};
 auto solid=[&](Vec3 p,Vec3 q,Vec3 r,Color c,Vec3 normal){addSolid(wp(p),wp(q),wp(r),c,rot(normal));};
 Vec3 gap=a.position-viewer_;bool detail=!crowded() && a.length*a.length>dot(gap,gap)*.003f;
 const Color back{99,123,138},flank{147,170,179},belly{204,219,218},fin{105,135,151},pale{192,209,207};
 // Side profile from Mola mola photographs. The clavus is a separate, broad rudder;
 // neither a tail peduncle nor an extra circle is hidden behind this body.
 static const Vec3 outline[16]={
  {0,-.018f,.490f},{0,.074f,.474f},{0,.196f,.400f},{0,.293f,.221f},
  {0,.338f,.040f},{0,.299f,-.168f},{0,.246f,-.305f},{0,.121f,-.348f},
  {0,-.068f,-.358f},{0,-.230f,-.312f},{0,-.306f,-.184f},{0,-.344f,.005f},
  {0,-.329f,.172f},{0,-.255f,.330f},{0,-.151f,.440f},{0,-.079f,.484f}};
 int n=detail?16:8,step=detail?1:2;
 Vec3 center{0,-.014f,.052f};
 auto rim=[&](int k,int side){Vec3 v=outline[(k%n)*step];v.x=side*(v.z>.45f?.026f:.019f);return v;};
 auto inner=[&](int k,int side){Vec3 v=mix(center,outline[(k%n)*step],.66f);v.x=side*.110f;return v;};
 for(int side=-1;side<=1;side+=2)for(int k=0;k<n;++k){
  Vec3 p=rim(k,side),q=rim(k+1,side),m=inner(k,side),o=inner(k+1,side),hub=center+Vec3{side*.138f,0,0};
  float y=(p.y+q.y)*.5f;Color c=blend(back,belly,clampf(.45f-y*1.6f,0,1));
  Vec3 normal{float(side),0,0};
  solid(hub,m,o,blend(c,flank,.25f),normal);solid(m,p,q,c,normal);solid(m,q,o,c,normal);
  if(detail && k%2==0 && p.z<.30f) {
   Vec3 spotsCenter=(hub+m+o)*(1.f/3)+Vec3{side*.0008f,0,0};
   tri(spotsCenter+(m-spotsCenter)*.12f,spotsCenter+(o-spotsCenter)*.16f,spotsCenter+(hub-spotsCenter)*.15f,pale);
  }
  if(side==1){Vec3 l=rim(k,-1),r=rim(k+1,-1),out=(p+q)*.5f-center;out.x=0;solid(l,r,q,c,out);solid(l,q,p,c,out);}
 }
 // Broad scalloped clavus, sharing the rear silhouette over its full height.
 // It bends very slightly as a rudder, without wagging like an ordinary tail.
 float bend=std::sin(a.phase*.32f)*.015f*a.activity;
 const int spans=detail?10:6;
 auto clavus=[&](int j,bool outside,int side){float t=float(j)/spans,y=.246f-t*.476f;
  float round=std::sin(t*Pi),z=-.305f-round*.052f;
  if(outside)z-=round*.132f+(j%2==0?.009f:0)*round;
  return Vec3{side*(outside?.010f:.022f)+(outside?bend*round:0),y,z};
 };
 for(int j=0;j<spans;++j){
  for(int side=-1;side<=1;side+=2){Vec3 p=clavus(j,false,side),q=clavus(j+1,false,side),r=clavus(j+1,true,side),s=clavus(j,true,side);Vec3 normal{float(side),0,0};
   Color c=j%2?fin:blend(fin,pale,.18f);solid(p,q,r,c,normal);solid(p,r,s,c,normal);
  }
  Vec3 p=clavus(j,true,-1),q=clavus(j+1,true,-1),r=clavus(j+1,true,1),s=clavus(j,true,1);
  solid(p,q,r,fin,{0,0,-1});solid(p,r,s,fin,{0,0,-1});
 }
 // Dorsal and anal fins rise well behind the centre, with broad roots and a
 // swept, rounded tip. Their synchronous transverse sculling provides propulsion.
 float stroke=std::sin(a.phase*.55f)*.105f*a.activity;
 for(int sign=-1;sign<=1;sign+=2){
  float stretch=sign>0?1.f:.91f;
  auto fp=[&](float y,float z){return Vec3{stroke*clampf((y-.27f)/.40f,0,1),sign*y*stretch,z};};
  Vec3 v[7]={fp(.308f,-.055f),fp(.489f,-.112f),fp(.688f,-.208f),fp(.712f,-.265f),fp(.650f,-.302f),fp(.403f,-.291f),fp(.250f,-.309f)};
  Vec3 hub=fp(.352f,-.205f);
  for(int k=0;k<7;++k)tri(hub,v[k],v[(k+1)%7],k%2?fin:blend(fin,flank,.2f));
  if(detail)for(int k=1;k<5;++k)tri(hub,v[k],v[k]+Vec3{.001f,0,.005f},scale(fin,.72f));
 }
 // Raised eyes, small open beak, gill slit immediately ahead of the pectoral fin.
 auto disk=[&](Vec3 c,Vec3 u,Vec3 v,Color color,int count){for(int k=0;k<count;++k){float t=k*2*Pi/count,b=(k+1)*2*Pi/count;tri(c,c+u*std::cos(t)+v*std::sin(t),c+u*std::cos(b)+v*std::sin(b),color);}};
 for(int side=-1;side<=1;side+=2){
  Vec3 eye{side*.073f,.079f,.415f};
  disk(eye,{0,.026f,0},{0,0,.026f},pale,detail?8:5);
  disk(eye+Vec3{side*.0015f,0,.001f},{0,.015f,0},{0,0,.016f},{22,37,43},detail?8:5);
  if(detail)disk(eye+Vec3{side*.002f,.005f,.005f},{0,.004f,0},{0,0,.004f},{233,242,237},4);
  Vec3 gill{side*.132f,.006f,.211f};
  disk(gill,{0,.032f,0},{0,0,.013f},{52,77, 80},detail?6:4);
  float pulse=std::sin(a.phase*.9f)*.020f*a.activity;
  Vec3 root{side*.133f,.005f,.184f},edge[5]={{side*.148f,.055f,.174f},{side*(.193f+pulse),.068f,.127f},{side*(.217f+pulse),.020f,.085f},{side*.194f,-.030f,.104f},{side*.140f,-.032f,.162f}};
  for(int k=0;k<4;++k)tri(root,edge[k],edge[k+1],k%2?fin:flank);
 }
 disk({0,-.033f,.495f},{.027f,0,0},{0,.024f,0},belly,8);
 disk({0,-.033f,.497f},{.014f,0,0},{0,.012f,0},{41,60,65},6);
}
}
