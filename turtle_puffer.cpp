#include "scene.h"
#if defined(__GNUC__)
#pragma GCC optimize("O3", "fast-math")
#endif
namespace abyss {
// Normalized length is nose to tail. Closed volumes retain solid-face culling;
// thin flippers and pigment patches remain two-sided. No per-frame allocation.
void Scene::turtlePufferMesh(const ecology::Animal& a) {
  material_=0;
  const float sy=std::sin(a.yaw),cy=std::cos(a.yaw);
  auto rotate=[&](Vec3 p){return Vec3{cy*p.x+sy*p.z,p.y,-sy*p.x+cy*p.z};};
  auto worldPoint=[&](Vec3 p){return a.position+rotate(p)*a.length;};
  auto face=[&](Vec3 p,Vec3 q,Vec3 r,Color c){add(worldPoint(p),worldPoint(q),worldPoint(r),c);};
  auto solid=[&](Vec3 p,Vec3 q,Vec3 r,Color c,Vec3 center){
    addSolid(worldPoint(p),worldPoint(q),worldPoint(r),c,rotate((p+q+r)*(1.f/3)-center));
  };
  auto quad=[&](Vec3 p,Vec3 q,Vec3 r,Vec3 s,Color c){face(p,q,r,c);face(p,r,s,c);};
  Vec3 gap=a.position-viewer_;
  bool close=!crowded() && a.length*a.length>dot(gap,gap)*.0036f;
  // Raised circular patches also work for eyes: use a shallow cone, not a square.
  auto disk=[&](Vec3 c,Vec3 u,Vec3 v,Color color,int n) {
    for(int k=0;k<n;++k) {
      float t=k*2*Pi/n,t1=(k+1)*2*Pi/n;
      face(c,c+u*std::cos(t)+v*std::sin(t),c+u*std::cos(t1)+v*std::sin(t1),color);
    }
  };
  if(a.species==ecology::Species::Turtle) {
    const Color shell{114,100, 64},seam{184,169,116},skin{158,158,111},belly{220,213,165};
    // A flattened oval carapace, broad ahead of the middle and tapered aft.
    auto dome=[](float x,float z){
      float r=x*x/(.265f*.265f)+z*z/(.350f*.350f);
      return Vec3{x,.012f+.138f*std::sqrt(std::max(0.f,1-r)),z};
    };
    auto shellFace=[&](Vec3 p,Vec3 q,Vec3 r,Color c) {
      addSolid(worldPoint(p),worldPoint(q),worldPoint(r),c,rotate({0,1,0}));
    };
    auto scute=[&](const Vec3* v,int n,int tone) {
      Vec3 c{};for(int i=0;i<n;++i)c=c+v[i];c=c*(1.f/n);
      Vec3 peak=dome(c.x,c.z);
      Color col=scale(shell,.87f+.065f*(tone%5));
      for(int i=0;i<n;++i) {
        Vec3 p=v[i],q=v[(i+1)%n];
        Vec3 ip=p*.955f+c*.045f,iq=q*.955f+c*.045f;
        p=dome(p.x,p.z);q=dome(q.x,q.z);ip=dome(ip.x,ip.z);iq=dome(iq.x,iq.z);
        if(close)shellFace(peak,ip,iq,col);
        if(close) {shellFace(p,q,iq,seam);shellFace(p,iq,ip,seam);}
        else shellFace(peak,p,q,col);
      }
    };
    // One watertight tessellation: five central plates, four pairs of costals.
    // All neighbours share their boundary vertices; no overlapping dome underneath.
    const float z[6]={-.350f,-.220f,-.085f,.055f,.190f,.350f};
    const float w[6]={0,.088f,.105f,.106f,.083f,0};
    auto boundary=[&](int j,int side){return Vec3{side*w[j],0,z[j]};};
    auto ridge=[&](int j){return Vec3{0,0,z[j]+(j>0&&j<5?.024f:0)};};
    auto outer=[](float zz,int side){return Vec3{side*.265f*std::sqrt(std::max(0.f,1-zz*zz/(.350f*.350f))),.012f,zz};};
    for(int j=0;j<5;++j) {
      Vec3 v[6]={boundary(j,-1),ridge(j),boundary(j,1),boundary(j+1,1),ridge(j+1),boundary(j+1,-1)};
      // The end plates taper to one vertex.
      if(j==0) {Vec3 end[4]={ridge(0),boundary(1,1),ridge(1),boundary(1,-1)};scute(end,4,j);}
      else if(j==4) {Vec3 end[4]={boundary(4,-1),ridge(4),boundary(4,1),ridge(5)};scute(end,4,j);}
      else scute(v,6,j);
    }
    for(int side=-1;side<=1;side+=2) {
      const int start[4]={0,1,3,4},finish[4]={1,3,4,5};
      for(int j=0;j<4;++j) {
        int lo=start[j],hi=finish[j],nv=0;Vec3 v[12];
        for(int q=lo;q<=hi;++q)v[nv++]=boundary(q,side);
        // Endpoints on the outer edge coincide with the central tip.
        if(hi<5)v[nv++]=outer(z[hi],side);
        for(int q=hi-1;q>=lo;--q) {
          v[nv++]=outer((z[q]+z[q+1])*.5f,side);
          if(q>lo || lo>0)v[nv++]=outer(z[q],side);
        }
        scute(v,nv,j+side+6);
      }
      for(int j=0;j<5;++j)for(int h=0;h<2;++h) {
        float za=z[j]+(z[j+1]-z[j])*(h*.5f),zb=z[j]+(z[j+1]-z[j])*((h+1)*.5f);
        Vec3 p=outer(za,side),q=outer(zb,side);
        Vec3 pb=p+Vec3{0,-.037f,0},qb=q+Vec3{0,-.037f,0};
        solid(p,pb,qb,seam,{0,0,0});solid(p,qb,q,seam,{0,0,0});
        solid({0,-.060f,0},qb,pb,belly,{0,0,0});
      }
    }
    // Small blunt head on a distinct neck; each barrel's own center sets winding.
    struct Ring{float z,w,h,y;};
    const float duck=-.045f*(1-a.activity);
    const Ring head[]={{.290f,.041f,.035f,0},{.365f,.044f,.040f,.002f},
      {.405f,.064f,.052f,.011f},{.466f,.058f,.046f,.004f},{.496f,.037f,.027f,-.005f}};
    const int sides=close?8:6;
    auto hp=[&](int j,int k){float t=k*2*Pi/sides;return Vec3{head[j].w*std::sin(t),head[j].y+duck+head[j].h*std::cos(t),head[j].z};};
    for(int j=0;j<4;++j)for(int k=0;k<sides;++k) {
      Vec3 p=hp(j,k),q=hp(j,k+1),r=hp(j+1,k+1),s=hp(j+1,k);
      Color col=k>=2&&k<6?belly:skin;
      Vec3 c{0,duck+.002f,(head[j].z+head[j+1].z)*.5f};
      solid(p,q,r,col,c);solid(p,r,s,col,c);
    }
    for(int k=0;k<sides;++k)solid({0,duck-.005f,.499f},hp(4,k),hp(4,k+1),skin,{0,duck,.45f});
    for(int side=-1;side<=1;side+=2) {
      Vec3 eye{side*.059f,duck+.032f,.446f};
      disk(eye,{0,.019f,0},{0,0,.023f},{209,190,120},close?6:4);
      disk(eye+Vec3{side*.002f,0,.001f},{0,.011f,0},{0,0,.013f},{16,24,21},close?6:4);
      if(close) {
        face({side*.061f,duck+.038f,.448f},{side*.061f,duck+.041f,.443f},{side*.061f,duck+.035f,.442f},{238,238,200});
        quad({side*.035f,duck-.013f,.497f},{side*.053f,duck-.015f,.469f},
             {side*.053f,duck-.019f,.469f},{side*.035f,duck-.017f,.497f},{76,77,49});
      }
      // Foreflipper: long swept blade with a rounded, narrow tip, bent at shoulder.
      float flap=std::sin(a.phase*.85f)*.45f*a.activity;
      auto fp=[&](float x,float z){return Vec3{side*(.185f+x*std::cos(flap)),-.018f+x*std::sin(flap),z};};
      Vec3 outline[7]={fp(0,.252f),fp(.120f,.292f),fp(.290f,.206f),fp(.420f,.068f),fp(.395f,.011f),fp(.255f,.066f),fp(.050f,.111f)};
      Vec3 hub=fp(.155f,.158f);hub.y+=.015f;
      for(int k=0;k<7;++k)face(hub,outline[k],outline[(k+1)%7],k%3?skin:scale(skin,.72f));
      if(close) for(int k=1;k<5;++k) {
        Vec3 mid=(outline[k]+hub)*.5f;Vec3 v=outline[k]-hub;
        face(mid+Vec3{-.011f,.003f,.013f},mid+Vec3{.011f,.003f,.013f},mid+v*.15f+Vec3{0,.003f,-.008f},{92,101,69});
      }
      float rear=std::sin(a.phase*.85f+.8f)*.035f*a.activity;
      Vec3 root{side*.165f,-.026f,-.240f},tip{side*.283f,rear-.035f,-.414f};
      Vec3 rearV[5]={root,{side*.292f,rear-.026f,-.286f},tip,{side*.232f,rear-.034f,-.440f},{side*.133f,-.030f,-.324f}};
      Vec3 rc{side*.229f,rear*.5f-.018f,-.336f};
      for(int k=0;k<5;++k)face(rc,rearV[k],rearV[(k+1)%5],k%2?skin:scale(skin,.83f));
    }
    face({-.028f,-.028f,-.316f},{.028f,-.028f,-.316f},{0,-.025f,-.430f},skin);
    return;
  }
  // Takifugu rubripes: an uninflated, blunt-headed fish, never a spiny ball.
  const Color back{77,91, 80},flank{161,173,145},white{237,237,218},fin{170,174,134},ink{37,48,42};
  struct Ring{float z,w,h,y;};
  static const Ring body[]={{.470f,.026f,.027f,-.014f},{.410f,.086f,.088f,.002f},
    {.290f,.140f,.145f,.011f},{.120f,.165f,.160f,.002f},{-.075f,.125f,.118f,-.004f},
    {-.230f,.060f,.058f,-.003f},{-.345f,.020f,.030f,0}};
  const int n=close?12:6;
  auto bp=[&](int j,int k){float t=k*2*Pi/n;return Vec3{body[j].w*std::sin(t),body[j].y+body[j].h*std::cos(t),body[j].z};};
  for(int j=0;j<6;++j)for(int k=0;k<n;++k) {
    float vertical=std::cos((k+.5f)*2*Pi/n);
    Color col=vertical<-.12f?white:vertical>.55f?back:flank;
    Vec3 p=bp(j,k),q=bp(j,k+1),r=bp(j+1,k+1),s=bp(j+1,k);
    Vec3 center{0,(body[j].y+body[j+1].y)*.5f,(body[j].z+body[j+1].z)*.5f};
    solid(p,q,r,col,center);solid(p,r,s,col,center);
    if(close && vertical>.05f && j>0 && j<5) {
      // Pigment stays on the actual facets, so it cannot sink into the loft.
      Vec3 c=(p+q+r)*(1.f/3),normal=unit(cross(q-p,r-p));
      if(dot(normal,c-center)<0)normal=normal*-1;
      float radius=(j+k)%3==0?.28f:.16f;
      face(c+(p-c)*radius+normal*.0008f,c+(q-c)*radius+normal*.0008f,c+(r-c)*radius+normal*.0008f,ink);
    }
  }
  for(int k=0;k<n;++k) {
    solid({0,-.014f,.472f},bp(0,k),bp(0,k+1),flank,{0,0,.40f});
    solid({0,0,-.351f},bp(6,k+1),bp(6,k),back,{0,0,-.25f});
  }
  // Small puckered terminal mouth and lips; the eye sits high, well ahead of the gill.
  disk({0,-.014f,.474f},{.027f,0,0},{0,.023f,0},{214,210,170},8);
  disk({0,-.014f,.476f},{.015f,0,0},{0,.010f,0},ink,6);
  for(int side=-1;side<=1;side+=2) {
    Vec3 eye{side*.102f,.093f,.337f};
    disk(eye,{side*.009f,.031f,0},{0,0,.032f},{196,182,114},close?8:4);
    disk(eye+Vec3{side*.004f,.001f,.002f},{side*.005f,.020f,0},{0,0,.019f},{14,23,25},close?8:4);
    if(close)disk(eye+Vec3{side*.007f,.010f,.010f},{0,.005f,0},{0,0,.005f},{243,246,231},4);
    // Ocellus behind the pectoral root, with a pale outline.
    Vec3 spot{side*.162f,.043f,.106f};
    disk(spot,{side*-.008f,.049f,0},{0,0,.050f},white,close?8:4);
    disk(spot+Vec3{side*.001f,0,0},{side*-.007f,.038f,0},{0,0,.039f},ink,close?8:4);
    float wave=std::sin(a.phase*2.8f+side*.5f)*.035f*a.activity;
    Vec3 root{side*.143f,.010f,.190f},tip{side*(.233f+wave),-.043f,.105f};
    face(root,{side*.204f,-.003f,.184f},tip,fin);
    face(root,tip,{side*.171f,-.064f,.132f},scale(fin,.85f));
  }
  float beat=std::sin(a.phase*2.1f)*.021f*a.activity;
  for(int side=-1;side<=1;side+=2) {
    Vec3 root{0,side*.068f,-.198f},tip{beat,side*.166f,-.232f};
    face(root,tip,{beat*.8f,side*.145f,-.302f},fin);
    face(root,{beat*.8f,side*.145f,-.302f},{0,side*.035f,-.306f},scale(fin,.83f));
  }
  float wag=std::sin(a.phase*.65f)*.035f*a.activity;
  Vec3 tail[5]={{0,.025f,-.331f},{wag,.096f,-.474f},{wag,0,-.501f},{wag,-.089f,-.474f},{0,-.025f,-.331f}};
  for(int k=1;k<4;++k)face(tail[0],tail[k],tail[k+1],k==2?back:fin);
}
}
