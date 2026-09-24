#include "scene.h"
#if defined(__GNUC__)
#pragma GCC optimize("O3", "fast-math")
#endif
namespace abyss {
void Scene::livingRelicMesh(const ecology::Animal& a) {
 material_=0;float sy=std::sin(a.yaw),cy=std::cos(a.yaw);
 auto rot=[&](Vec3 p){return Vec3{p.x*cy+p.z*sy,p.y,-p.x*sy+p.z*cy};};
 auto wp=[&](Vec3 p){return a.position+rot(p)*a.length;};
 auto tri=[&](Vec3 p,Vec3 q,Vec3 r,Color c){add(wp(p),wp(q),wp(r),c);};
 auto solid=[&](Vec3 p,Vec3 q,Vec3 r,Color c,Vec3 center){addSolid(wp(p),wp(q),wp(r),c,rot((p+q+r)*(1.f/3)-center));};
 auto panel=[&](Vec3 p,Vec3 q,Vec3 r,Vec3 s,Color c){tri(p,q,r,c);tri(p,r,s,c);};
 Vec3 gap=a.position-viewer_;bool detail=!crowded() && a.length*a.length>dot(gap,gap)*.0035f;
 auto eye=[&](Vec3 c,float radius,int side){
  int n=detail?8:5;
  for(int k=0;k<n;++k){float t=k*2*Pi/n,u=(k+1)*2*Pi/n;
   Vec3 p{0,std::cos(t)*radius,std::sin(t)*radius},q{0,std::cos(u)*radius,std::sin(u)*radius};
   tri(c,c+p,c+q,{171,189,170});
   tri(c+Vec3{side*.002f,0,0},c+p*.62f+Vec3{side*.002f,0,0},c+q*.62f+Vec3{side*.002f,0,0},{13,26, 30});
  }
 };
 if(a.species==ecology::Species::Nautilus) {
  const Color ivory{238,223,191},stripe{169, 80,58},hood{181,104,70},flesh{223,211,182};
  // A compressed, rounded planispiral shell in the YZ plane, NOT a flat disk.
  Vec3 center{0,.045f,-.165f};int n=detail?24:12;
  auto shell=[&](int side,int band,int k){
   float t=(k/2)*(4*Pi/n)+(k%2?4*Pi/n*.30f:0)+(band==0?-.95f:band==1?-.34f:0),r=band==0?.065f:band==1?.218f:.320f;
   float x=band==0?.109f:band==1?.143f:.032f;
   return center+Vec3{side*x,std::sin(t)*r,std::cos(t)*r};
  };
  for(int side=-1;side<=1;side+=2)for(int k=0;k<n;++k){
   Color c=(k%2==0)?stripe:ivory;
   solid(center+Vec3{side*.098f,0,0},shell(side,0,k),shell(side,0,k+1),ivory,center);
   for(int j=0;j<2;++j){Vec3 p=shell(side,j,k),q=shell(side,j,k+1),r=shell(side,j+1,k+1),s=shell(side,j+1,k);
    Color inner=j==0?blend(c,ivory,.32f):c;
    solid(p,q,r,inner,center);solid(p,r,s,inner,center);
   }
   if(side==1){Vec3 p=shell(-1,2,k),q=shell(-1,2,k+1),r=shell(1,2,k+1),s=shell(1,2,k);solid(p,q,r,c,center);solid(p,r,s,c,center);}
  }
  // Involute center: a small curved suture on each side of the smooth outer shell.
  if(detail)for(int side=-1;side<=1;side+=2)for(int j=0;j<16;++j){
   float t=j*.34f,u=(j+1)*.34f,r=.012f+j*.0034f,r1=.012f+(j+1)*.0034f;
   Vec3 p=center+Vec3{side*.112f,std::sin(t)*r,std::cos(t)*r},q=center+Vec3{side*.112f,std::sin(u)*r1,std::cos(u)*r1};
   panel(p+Vec3{0,.002f,0},p-Vec3{0,.002f,0},q-Vec3{0,.002f,0},q+Vec3{0,.002f,0},stripe);
  }
  // Aperture below the hood, eye sockets flanking the tentacle crown.
  Vec3 top{0,.104f,.170f},front{0,-.025f,.290f},rear{0,.130f,.052f};
  for(int side=-1;side<=1;side+=2){
   Vec3 edge{side*.145f,-.034f,.191f},lower{side*.096f,-.147f,.167f};
   tri(rear,top,edge,hood);tri(top,front,edge,hood);
   tri(edge,front,lower,flesh);tri(front,{0,-.171f,.240f},lower,flesh);
   eye({side*.126f,-.060f,.211f},.031f,side);
   if(detail)for(int j=0;j<5;++j){float t=(j+.6f)/6;Vec3 p=mix(top,edge,t)+Vec3{0,.003f,.003f};
    tri(p+Vec3{-.009f,0,0},p+Vec3{.009f,0,0},p+Vec3{0,.008f,.010f},ivory);
   }
  }
  // Many fine, suckerless cirri; visual representatives of the 90+ real appendages.
  const float extension=clampf(a.activity,0,1),spread=.25f+.75f*extension;
  const float baseZ=.165f+.075f*extension;
  int arms=detail?18:9,joints=detail?3:2;
  for(int k=0;k<arms;++k){float t=k*2*Pi/arms;Vec3 prev{std::sin(t)*.085f*spread,-.076f+std::cos(t)*.068f*spread,baseZ};
   float reach=(.170f+.065f*noise(a.id+k*13))*(.02f+.98f*extension);
   for(int j=1;j<=joints;++j){float f=float(j)/joints;
    Vec3 next{std::sin(t)*(.085f+f*.075f)*spread+std::sin(a.phase*.7f+k+f)*.013f*f*extension,-.076f+std::cos(t)*(.068f+f*.075f)*spread-f*.055f*extension,baseZ+reach*f};
    float w=.008f*(1-f*.67f);panel(prev+Vec3{w,w*.7f,0},prev-Vec3{w,w*.7f,0},next-Vec3{w*.5f,w*.35f,0},next+Vec3{w*.5f,w*.35f,0},flesh);prev=next;
   }
  }
  // Siphon: a short ventral funnel, distinct from the tentacles.
  panel({-.035f,-.154f,.151f},{.035f,-.154f,.151f},{.021f,-.188f,.271f},{-.021f,-.188f,.271f},ivory);
  return;
 }
 const Color skin{67,100,124},under{120,145,155},fin{79,116,138},spots{173,192,186};
 struct Hoop{float z,w,h,y;};
 const Hoop body[]={{.490f,.049f,.051f,-.015f},{.420f,.086f,.103f,.005f},{.280f,.112f,.137f,0},
  {.105f,.121f,.145f,0},{-.080f,.102f,.121f,-.003f},{-.240f,.058f,.076f,-.005f},{-.355f,.024f,.037f,-.002f}};
 const int n=detail?10:6;Vec3 rings[7][10];
 float wag=std::sin(a.phase*.55f)*.025f*a.activity;
 for(int j=0;j<7;++j)for(int k=0;k<n;++k){float t=k*2*Pi/n;rings[j][k]={body[j].w*std::sin(t)+(j>4?wag*(j-4)*.3f:0),body[j].y+body[j].h*std::cos(t),body[j].z};}
 for(int j=0;j<6;++j)for(int k=0;k<n;++k){
  Vec3 p=rings[j][k],q=rings[j][(k+1)%n],r=rings[j+1][(k+1)%n],s=rings[j+1][k];
  Vec3 c{j>4?wag*.3f:0,0,(body[j].z+body[j+1].z)*.5f};Color col=std::cos((k+.5f)*2*Pi/n)<-.35f?under:skin;
  solid(p,q,r,col,c);solid(p,r,s,col,c);
  if(detail && j>0 && k%3!=0){Vec3 m=(p+q+r)*(1.f/3),normal=unit(cross(q-p,r-p));if(dot(normal,m-c)<0)normal=normal*-1;
   float f=.10f+noise(a.id+j*19+k)*.12f;
   tri(m+(p-m)*f+normal*.0008f,m+(q-m)*f+normal*.0008f,m+(r-m)*f+normal*.0008f,spots);
  }
 }
 for(int k=0;k<n;++k){solid({0,-.015f,.496f},rings[0][k],rings[0][(k+1)%n],under,{0,0,.40f});solid({wag*.6f,0,-.357f},rings[6][k],rings[6][(k+1)%n],skin,{0,0,-.28f});}
 // Fleshy lobe plus a separate radiating fan. Alternating paired fins scull slowly.
 auto lobe=[&](Vec3 root,Vec3 end,Vec3 width,Vec3 reach){
  panel(root+width*.4f,root-width*.4f,end-width*.6f,end+width*.6f,skin);
  tri(root+width*.4f,end+width*.6f,end+Vec3{0,.014f,0},under);
  Vec3 arc[5]={end+width*.7f,end+reach*.65f+width,end+reach,end+reach*.65f-width,end-width*.7f};
  for(int j=0;j<4;++j)tri(end,arc[j],arc[j+1],j%2?fin:scale(fin,1.20f));
 };
 for(int side=-1;side<=1;side+=2){
  // Large, as a fish that lives at four hundred metres has to be, and leaning with the
  // blunt plated head. The nautilus keeps the eye() above: it is not a fish and its eye
  // is a pinhole with no lens in it at all.
  fishEye(tri,{side*.081f,.057f,.408f},float(side),.023f,.20f,detail);
  if(detail){panel({side*.113f,.085f,.261f},{side*.117f,.080f,.250f},{side*.112f,-.073f,.248f},{side*.108f,-.076f,.257f},{35,60, 70});
   panel({side*.052f,-.023f,.493f},{side*.087f,-.036f,.405f},{side*.087f,-.041f,.405f},{side*.052f,-.028f,.493f},{27, 40,51});}
  float stroke=std::sin(a.phase+side*Pi*.5f)*.035f*a.activity;
  lobe({side*.108f,-.046f,.221f},{side*.192f,-.090f+stroke,.126f},{side*.038f,0,.033f},{side*.062f,-.020f,-.112f});
  lobe({side*.063f,-.113f,-.052f},{side*.103f,-.186f-stroke,-.117f},{side*.029f,0,.030f},{side*.028f,-.018f,-.100f});
 }
 // First dorsal is ray-supported; second dorsal and anal have muscular stalks.
 Vec3 dr{0,.127f,.156f};Vec3 dv[5]={{0,.139f,.205f},{0,.282f,.142f},{0,.294f,.061f},{0,.250f,-.019f},{0,.124f,-.031f}};
 for(int j=0;j<4;++j)tri(dr,dv[j],dv[j+1],j%2?fin:scale(fin,1.20f));
 lobe({0,.091f,-.184f},{wag*.2f,.173f,-.221f},{0,.010f,.034f},{0,.062f,-.079f});
 lobe({0,-.081f,-.206f},{wag*.2f,-.159f,-.239f},{0,.012f,.031f},{0,-.066f,-.070f});
 // Broad upper/lower caudal lobes, and the characteristic projecting middle tuft.
 for(int side=-1;side<=1;side+=2){
  Vec3 root{wag*.6f,side*.019f,-.320f},v[4]={{wag*.7f,side*.112f,-.351f},{wag,side*.155f,-.419f},{wag,side*.125f,-.457f},{wag,side*.039f,-.458f}};
  for(int j=0;j<3;++j)tri(root,v[j],v[j+1],j%2?fin:scale(fin,1.15f));
 }
 panel({wag,-.025f,-.413f},{wag,.025f,-.413f},{wag,.031f,-.495f},{wag,-.031f,-.495f},skin);
 tri({wag,.031f,-.495f},{wag,-.031f,-.495f},{wag,0,-.520f},fin);
}
}
