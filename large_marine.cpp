#include "scene.h"
namespace abyss {
void Scene::largeMarineMesh(const ecology::Animal& a) {
 using S=ecology::Species;
 const bool hammer=a.species==S::Hammerhead,dolphin=a.species==S::Dolphin,orca=a.species==S::Orca;
 const bool white=a.species==S::WhiteShark;
 const bool manta=a.species==S::Manta,whale=a.species==S::WhaleShark,mammal=dolphin||orca;
 Vec3 gap=a.position-viewer_;
 bool detail=!crowded() && dot(gap,gap)<a.length*a.length*45;
 material_=0;
 Vec3 f{std::sin(a.yaw),0,std::cos(a.yaw)},r{f.z,0,-f.x};
 auto deform=[&](Vec3 p) {
  float t=clampf((.12f-p.z)/.60f,0,1);
  float bend=std::sin(a.phase-t*2.4f)*t*t*(mammal?.055f:.065f);
  if(mammal) p.y+=bend;else if(!manta) p.x+=bend;
  return p;
 };
 auto point=[&](Vec3 p) {p=deform(p);if(manta)p=p*(1.f/1.235f);return a.position+(r*p.x+Vec3{0,p.y,0}+f*p.z)*a.length;};
 auto tri=[&](Vec3 p,Vec3 q,Vec3 t,Color c) {add(point(p),point(q),point(t),c);};
 auto face=[&](Vec3 p,Vec3 q,Vec3 t,Vec3 u,Color c) {tri(p,q,t,c);tri(p,t,u,c);};
 // Hull faces: one side of these is inside the animal and never seen, so they are wound
 // outward and flagged for the renderer to drop. Pass the outward direction where the
 // shape makes it obvious (radial on a barrel, along the axis on a cap); leave it out
 // and the centroid is used. Fins, lobes and lips stay on tri/face - they have no inside.
 auto turn=[&](Vec3 p) {return r*p.x+Vec3{0,p.y,0}+f*p.z;};
 auto triS=[&](Vec3 p,Vec3 q,Vec3 t,Color c,Vec3 out={}) {
  Vec3 o=(out.x||out.y||out.z)?out:(p+q+t)*(1.f/3);
  addSolid(point(p),point(q),point(t),c,turn(o));
 };
 auto faceS=[&](Vec3 p,Vec3 q,Vec3 t,Vec3 u,Color c,Vec3 out={}) {triS(p,q,t,c,out);triS(p,t,u,c,out);};
 Color back=orca?Color{20,28,36}:whale?Color{66,100,119}:dolphin?Color{113,141,154}:Color{102,121,129};
 Color belly=orca?Color{233,237,226}:Color{208,218,211},dark{18,26,30};
 // Fins have thickness and a curved outline, not single paper triangles.
 auto fin=[&](const Vec3* rim,int n,Vec3 thick,Color top,Color bottom) {
  Vec3 c{};for(int i=0;i<n;++i)c=c+rim[i];c=c*(1.f/n);
  for(int i=0;i<n;++i){tri(c+thick,rim[i],rim[(i+1)%n],top);tri(c-thick,rim[(i+1)%n],rim[i],bottom);}
 };
 auto eye=[&](Vec3 p,float radius) {
  Vec3 q[6];for(int i=0;i<6;++i){float t=i*Pi/3;q[i]=p+Vec3{0,std::sin(t)*radius,std::cos(t)*radius};}
  for(int i=0;i<6;++i)tri(p,q[i],q[(i+1)%6],dark);
 };
 if(manta) {
  // Broad rhomboid disc: trailing edge sweeps back INTO the central pelvic region.
  // Mobula birostris from above: the span is 1.92 body lengths.
  back={27,40,48};belly={215,224,218};
  const float span[]={0,.14f,.27f,.43f,.62f,.80f,.96f};
  const float leading[]={.285f,.29f,.20f,.105f,-.015f,-.15f,-.30f};
  const float trailing[]={-.49f,-.455f,-.335f,-.255f,-.245f,-.26f,-.30f};
  auto disc=[&](int side,float x,float z,float chord,bool underside) {
   float y=std::sin(a.phase-x*.9f)*x*x*.18f;
   float thickness=(.060f*std::exp(-x*x*16)+.012f*(1-x))*std::sin(chord*Pi);
   return Vec3{side*x,y+(underside?-thickness*.55f:thickness),z};
  };
  for(int side:{-1,1}) {
   Vec3 upper[7][5],lower[7][5];
   for(int i=0;i<7;++i)for(int j=0;j<5;++j){float t=j*.25f,z=leading[i]*(1-t)+trailing[i]*t;upper[i][j]=disc(side,span[i],z,t,false);lower[i][j]=disc(side,span[i],z,t,true);}
   for(int i=0;i<6;++i)for(int j=0;j<4;++j){faceS(upper[i][j],upper[i+1][j],upper[i+1][j+1],upper[i][j+1],back,{0,1,0});faceS(lower[i][j],lower[i][j+1],lower[i+1][j+1],lower[i+1][j],belly,{0,-1,0});}
   // Cephalic lobes are curled, fleshy funnels joined to either corner of the mouth.
   Vec3 curl[5]={{side*.12f,.005f,.255f},{side*.165f,.005f,.31f},{side*.18f,.030f,.38f},{side*.155f,.054f,.405f},{side*.12f,.052f,.385f}};
   for(int j=0;j<4;++j){Vec3 low=curl[j],high=curl[j+1];Vec3 w{side*.024f,0,0};face(low-w,low+w,high+w*.8f,high-w*.8f,back);face(low-w-Vec3{0,.012f,0},high-w*.8f-Vec3{0,.012f,0},high+w*.8f-Vec3{0,.012f,0},low+w-Vec3{0,.012f,0},belly);}
   eye({side*.165f,.018f,.255f},.013f);
   // Light shoulder patches sit on the disc, with the front edge across the head.
   const Vec3 patch[]={{side*.075f,.034f,.24f},{side*.18f,.040f,.24f},{side*.31f,.030f,.15f},{side*.19f,.057f,.115f}};
   face(patch[0],patch[1],patch[2],patch[3],{147,163,162});
   if(detail)for(int j=0;j<5;++j){float z=.09f-j*.052f;face({side*.055f,-.042f,z},{side*.135f,-.030f,z-.025f},{side*.135f,-.030f,z-.032f},{side*.055f,-.042f,z-.007f},dark);}
  }
  // The mouth has depth and lips; there is no pointed fish snout in its centre.
  face({-.115f,-.020f,.29f},{.115f,-.020f,.29f},{.115f,.024f,.29f},{-.115f,.024f,.29f},dark);
  face({-.115f,.024f,.29f},{.115f,.024f,.29f},{.13f,.012f,.25f},{-.13f,.012f,.25f},back);
  face({-.115f,-.020f,.29f},{-.13f,-.022f,.25f},{.13f,-.022f,.25f},{.115f,-.020f,.29f},belly);
  // A slender, tapering whip, not a broad stingray paddle.
  for(int j=0;j<5;++j){float t=j*.2f,u=(j+1)*.2f;Vec3 p{std::sin(a.phase*.4f+t)*.018f*t,0,-.45f-t*.38f},q{std::sin(a.phase*.4f+u)*.018f*u,0,-.45f-u*.38f};float w=.009f*(1-t)+.002f;face(p+Vec3{-w,0,0},p+Vec3{w,0,0},q+Vec3{w*.75f,0,0},q-Vec3{w*.75f,0,0},back);face(p+Vec3{0,w,0},p-Vec3{0,w,0},q-Vec3{0,w*.75f,0},q+Vec3{0,w*.75f,0},back);}
  Vec3 dorsal[]={{0,.025f,-.39f},{0,.082f,-.445f},{0,.01f,-.50f}};fin(dorsal,3,{.008f,0,0},back,back);return;
 }
 // Profile stations from rostrum to peduncle. Different heads are not scaled copies.
 float z[]={.50f,.43f,.34f,.21f,.03f,-.15f,-.29f,-.39f};
 float widths[8]={.006f,.050f,.092f,.108f,.100f,.070f,.036f,.013f};
 float heights[8]={.008f,.053f,.096f,.119f,.115f,.083f,.040f,.019f};
 float centers[8]={0,0,0,0,0,0,0,0};
 if(dolphin){float w[]={.015f,.024f,.068f,.091f,.083f,.056f,.027f,.012f};float h[]={.015f,.022f,.079f,.093f,.086f,.060f,.029f,.016f};for(int i=0;i<8;++i){widths[i]=w[i];heights[i]=h[i];}centers[0]=centers[1]=-.025f;}
 if(orca){float w[]={.025f,.083f,.119f,.136f,.117f,.079f,.037f,.017f};float h[]={.030f,.082f,.125f,.143f,.128f,.091f,.044f,.022f};for(int i=0;i<8;++i){widths[i]=w[i];heights[i]=h[i];}}
 if(hammer){
  const float hz[]={.43f,.37f,.28f,.14f,-.03f,-.17f,-.27f,-.32f};
  const float hw[]={.038f,.042f,.059f,.069f,.059f,.038f,.019f,.012f};
  const float hh[]={.020f,.029f,.057f,.066f,.058f,.037f,.018f,.013f};
  for(int i=0;i<8;++i){z[i]=hz[i];widths[i]=hw[i];heights[i]=hh[i];}back={107,121,113};
 }
 if(white){
  const float wz[]={.50f,.445f,.355f,.235f,.06f,-.125f,-.29f,-.37f};
  const float ww[]={.005f,.052f,.092f,.116f,.119f,.081f,.032f,.017f};
  const float wh[]={.009f,.049f,.099f,.127f,.129f,.089f,.033f,.022f};
  for(int i=0;i<8;++i){z[i]=wz[i];widths[i]=ww[i];heights[i]=wh[i];centers[i]=i<3?.013f:0;}back={78,94,99};
 }
 if(whale){float w[]={.080f,.127f,.134f,.126f,.106f,.071f,.039f,.018f};float h[]={.025f,.050f,.075f,.088f,.083f,.062f,.036f,.024f};for(int i=0;i<8;++i){widths[i]=w[i];heights[i]=h[i];}}
 int sides=detail&&(hammer||white)?16:detail?12:8;Vec3 rings[8][16];
 for(int i=0;i<8;++i)for(int k=0;k<sides;++k){float t=k*2*Pi/sides;rings[i][k]={std::cos(t)*widths[i],centers[i]+std::sin(t)*heights[i],z[i]};}
 for(int i=0;i<7;++i)for(int k=0;k<sides;++k){float y=std::sin((k+.5f)*2*Pi/sides);Color c=y<-.25f?belly:back;if(!orca && !white && y>=-.25f)c=blend(back,belly,(1-y)*.18f);faceS(rings[i][k],rings[i][(k+1)%sides],rings[i+1][(k+1)%sides],rings[i+1][k],c,{std::cos((k+.5f)*2*Pi/sides),std::sin((k+.5f)*2*Pi/sides),0});}
 for(int k=0;k<sides;++k){triS({0,centers[0],z[0]},rings[0][k],rings[0][(k+1)%sides],back,{0,0,1});triS({0,0,z[7]},rings[7][(k+1)%sides],rings[7][k],back,{0,0,-1});}
 auto skin=[&](float zz,float angle,float lift=.002f){int j=0;while(j<6&&zz<z[j+1])++j;float t=clampf((z[j]-zz)/(z[j]-z[j+1]),0,1);float w=widths[j]*(1-t)+widths[j+1]*t,h=heights[j]*(1-t)+heights[j+1]*t,cy=centers[j]*(1-t)+centers[j+1]*t;return Vec3{std::cos(angle)*(w+lift),cy+std::sin(angle)*(h+lift),zz};};
 auto patch=[&](float zz,float angle,float rz,float ra,Color c){Vec3 center=skin(zz,angle,.004f);Vec3 out{std::cos(angle),std::sin(angle),0};for(int i=0;i<10;++i){float t=i*Pi/5,u=(i+1)*Pi/5;triS(center,skin(zz+rz*std::cos(t),angle+ra*std::sin(t),.004f),skin(zz+rz*std::cos(u),angle+ra*std::sin(u),.004f),c,out);}};
 for(int side:{-1,1}) {
  float angle=side==1?0:Pi;
  if(!hammer) eye(skin(whale?.44f:.36f,angle,.004f),orca?.008f:.006f);
  float reach=orca?.235f:whale?.26f:hammer?.24f:.235f;
  Vec3 pectoral[]={{side*.070f,-.035f,.24f},{side*(reach*.77f),-.074f,.07f},{side*reach,-.10f,-.07f},{side*(reach*.77f),-.09f,-.105f},{side*.07f,-.055f,.09f}};
  if(orca){pectoral[1].z=.12f;pectoral[2].z=.01f;pectoral[3].z=-.04f;}
  if(hammer){Vec3 v[]={{side*.055f,-.025f,.255f},{side*.123f,-.059f,.155f},{side*.155f,-.110f,.055f},{side*.105f,-.08f,.08f},{side*.049f,-.039f,.165f}};for(int i=0;i<5;++i)pectoral[i]=v[i];}
  if(white){Vec3 v[]={{side*.08f,-.05f,.24f},{side*.188f,-.10f,.10f},{side*.256f,-.17f,-.028f},{side*.218f,-.144f,-.057f},{side*.071f,-.065f,.115f}};for(int i=0;i<5;++i)pectoral[i]=v[i];}
  fin(pectoral,5,{0,.009f,0},back,mammal?back:belly);
  if(!mammal){Vec3 pelvic[]={{side*.04f,-.045f,-.16f},{side*.095f,-.07f,-.24f},{side*.035f,-.05f,-.25f}};fin(pelvic,3,{0,.006f,0},back,belly);}
  if(orca){patch(.295f,side==1?.32f:Pi-.32f,.055f,.17f,belly);patch(-.13f,side==1?.88f:Pi-.88f,.065f,.29f,{140,151,151});}
  if((hammer||white) && detail)for(int j=0;j<5;++j){float zz=(hammer?.30f:.31f)-j*.020f;
   for(int k=0;k<3;++k){float t=-.70f+k*.35f,u=t+.35f;face(skin(zz,angle+t,.003f),skin(zz,angle+u,.003f),skin(zz-.005f,angle+u,.003f),skin(zz-.005f,angle+t,.003f),dark);}
  }
  if(whale && detail)for(int j=0;j<5;++j){float zz=.275f-j*.022f;face(skin(zz,angle-.52f),skin(zz,angle+.36f),skin(zz-.006f,angle+.34f),skin(zz-.006f,angle-.52f),dark);}
  if(white && detail){ // Curved closed mouth beneath the short conical rostrum.
   for(int j=0;j<4;++j){float zz=.475f-j*.024f,zz2=zz-.024f;float t=side==1?-.5f:Pi+.5f;face(skin(zz,t,.003f),skin(zz2,t-.10f*side,.003f),skin(zz2,t-.10f*side-.035f,.003f),skin(zz,t-.035f,.003f),dark);}
  }
 }
 float dh=orca?((a.id%3==0)?.36f:.26f):hammer?.21f:dolphin?.18f:whale?.15f:.205f;
 Vec3 dorsal[]={{0,heights[4]*.85f,.13f},{0,dh*.91f,.045f},{0,dh,-.005f},{0,dh*.70f,-.042f},{0,heights[4]*.85f,-.105f}};
 if(hammer){Vec3 v[]={{0,.06f,.18f},{0,.166f,.11f},{0,.194f,.042f},{0,.114f,.06f},{0,.052f,-.04f}};for(int i=0;i<5;++i)dorsal[i]=v[i];}
 if(white){Vec3 v[]={{0,.119f,.135f},{0,.217f,.055f},{0,.263f,.006f},{0,.146f,-.035f},{0,.109f,-.095f}};for(int i=0;i<5;++i)dorsal[i]=v[i];}
 if(whale) for(auto& p:dorsal) p.z-=.10f;
 fin(dorsal,5,{.012f,0,0},back,back);
 if(hammer||white){float h=white?.059f:hammer?.058f:.089f;Vec3 second[]={{0,.032f,-.23f},{0,h,-.25f},{0,.02f,-.30f}};fin(second,3,{.004f,0,0},back,back);
  Vec3 anal[]={{0,-.039f,-.21f},{0,-.077f,-.26f},{0,-.022f,-.30f}};fin(anal,3,{.004f,0,0},belly,belly);
 }
 if(whale){Vec3 second[]={{0,.047f,-.22f},{0,.089f,-.265f},{0,.030f,-.30f}};fin(second,3,{.005f,0,0},back,back);}
 for(int side:{-1,1}) {
  Vec3 tail[6];
  if(mammal){Vec3 p[]={{0,0,-.36f},{side*.105f,0,-.397f},{side*.20f,-.01f,-.49f},{side*.11f,-.005f,-.477f},{side*.025f,0,-.49f},{0,0,-.467f}};for(int i=0;i<6;++i)tail[i]=p[i];fin(tail,6,{0,.007f,0},back,belly);}
  else if(hammer){
   if(side>0){Vec3 p[]={{0,0,-.295f},{0,.036f,-.355f},{0,.127f,-.465f},{0,.132f,-.515f},{0,.082f,-.473f},{0,.010f,-.363f}};for(int i=0;i<6;++i)tail[i]=p[i];}
   else {Vec3 p[]={{0,.003f,-.305f},{0,-.070f,-.36f},{0,-.09f,-.408f},{0,-.036f,-.385f},{0,-.008f,-.36f},{0,0,-.337f}};for(int i=0;i<6;++i)tail[i]=p[i];}
   fin(tail,6,{.005f,0,0},back,belly);
  } else if(whale){float reach=side>0?.22f:.15f;Vec3 p[]={{0,0,-.36f},{0,side*reach*.68f,-.40f},{0,side*reach,-.50f},{0,side*reach*.60f,-.46f},{0,side*.027f,-.445f},{0,0,-.415f}};for(int i=0;i<6;++i)tail[i]=p[i];fin(tail,6,{.006f,0,0},back,belly);}
  else {float reach=side>0?(whale?.22f:.20f):(whale?.15f:.18f);Vec3 p[]={{0,0,-.36f},{0,side*reach*.52f,-.425f},{0,side*reach,-.52f},{0,side*reach*.68f,-.478f},{0,side*.038f,-.413f},{0,0,-.395f}};for(int i=0;i<6;++i)tail[i]=p[i];fin(tail,6,{.006f,0,0},back,belly);}

 }
 if(hammer){
  // Loft the cephalofoil across its width: thin ends stay fleshy rather than becoming
  // the zero-thickness perimeter of a diamond fan. Small scallops flank the notch.
  const float xs[]={-.155f,-.144f,-.10f,-.065f,-.038f,-.018f,0,.018f,.038f,.065f,.10f,.144f,.155f};
  const float front[]={.425f,.455f,.477f,.470f,.494f,.499f,.486f,.499f,.494f,.470f,.477f,.455f,.425f};
  const float rear[]={.385f,.380f,.389f,.411f,.424f,.43f,.43f,.43f,.424f,.411f,.389f,.380f,.385f};
  Vec3 upper[13][3],lower[13][3];
  for(int i=0;i<13;++i)for(int j=0;j<3;++j){float t=j*.5f;float h=j==1?.018f:.007f;upper[i][j]={xs[i],h,front[i]*(1-t)+rear[i]*t};lower[i][j]=upper[i][j]-Vec3{0,h*1.8f,0};}
  for(int i=0;i<12;++i){for(int j=0;j<2;++j){face(upper[i][j],upper[i+1][j],upper[i+1][j+1],upper[i][j+1],back);face(lower[i][j],lower[i][j+1],lower[i+1][j+1],lower[i+1][j],belly);}face(upper[i][0],lower[i][0],lower[i+1][0],upper[i+1][0],back);face(upper[i][2],upper[i+1][2],lower[i+1][2],lower[i][2],belly);}
  for(int end:{0,12})face(upper[end][0],upper[end][2],lower[end][2],lower[end][0],back);
  eye({.156f,.003f,.420f},.008f);eye({-.156f,.003f,.420f},.008f);
 }
 if(!mammal && !hammer)for(int side:{-1,1}) {
  Vec3 keel[]={{side*.022f,0,-.29f},{side*.044f,0,-.36f},{side*.012f,0,-.40f}};
  fin(keel,3,{0,.004f,0},back,belly);
 }
 if(mammal && detail)patch(.27f,Pi*.5f,.014f,.09f,dark);
 if(whale){
  face({-.080f,-.008f,.501f},{.080f,-.008f,.501f},{.080f,.007f,.501f},{-.080f,.007f,.501f},dark);
  // White spots follow the skin rather than floating as a separate flat plate.
  int rows=detail?13:7,around=detail?9:5;
  for(int j=0;j<rows;++j)for(int k=0;k<around;++k){float zz=.40f-j*(.66f/(rows-1)),angle=-.12f+k*(Pi+.24f)/(around-1);float radius=detail?.005f:.008f;Vec3 c=skin(zz,angle,.004f);Vec3 out{std::cos(angle),std::sin(angle),0};triS(c,skin(zz+radius,angle-.052f,.004f),skin(zz-radius,angle-.052f,.004f),belly,out);triS(c,skin(zz-radius,angle+.052f,.004f),skin(zz+radius,angle+.052f,.004f),belly,out);}
  if(detail)for(int side:{-1,1})for(int ridge=0;ridge<3;++ridge){float angle=side==1?.10f+ridge*.29f:Pi-.10f-ridge*.29f;for(int j=2;j<6;++j)face(skin(z[j],angle-.018f,.006f),skin(z[j],angle+.018f,.006f),skin(z[j+1],angle+.018f,.006f),skin(z[j+1],angle-.018f,.006f),{116,145,154});}
 }
 material_=0;
}
}
