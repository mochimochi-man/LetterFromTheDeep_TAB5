#include "scene.h"
namespace abyss {
void Scene::poseSubmarine(const MachinePose& p){count=0;overflow=false;lighting_=1;viewer_=p.position+Vec3{0,0,p.length*1.4f};submarineMesh(p);}
void Scene::submarineMesh(const MachinePose& a){
 material_=0;
 const float d2=dot(a.position-viewer_,a.position-viewer_);
 const bool fine=!crowded()&&d2<a.length*a.length*9;
 const bool middle=!crowded()&&d2<a.length*a.length*36;
 const Color bronze{164,127,82},dark{94,64,36},gold{233,190,118},iron{31,39,39},copper{201,125,70},glass{255,205,108};
 auto w=[&](Vec3 p){return a.position+rotateY(p,a.yaw)*a.length;};
 auto tri=[&](Vec3 p,Vec3 q,Vec3 r,Color c){add(w(p),w(q),w(r),c);};
 auto solid=[&](Vec3 p,Vec3 q,Vec3 r,Color c,Vec3 origin){
  Vec3 n=unit(cross(q-p,r-p)),center=(p+q+r)*(1.f/3);if(dot(n,center-origin)<0)n=n*(-1);
  // Local bronze response adds restrained cool recesses and warm metal highlights.
  float edge=std::max(0.f,dot(n,unit(Vec3{.7f,1,.5f})));
  float glint=std::pow(edge,14.f)*.35f;
  c=blend(c,Color{255,225,169},glint);
  addSolid(w(p),w(q),w(r),c,rotateY(center-origin,a.yaw));
 };
 auto disc=[&](Vec3 p,Vec3 n,float radius,Color c,bool light=false){
  n=unit(n); Vec3 u=unit(cross(n,{.13f,1,.21f}))*radius,v=cross(n,u);
  material_=light?7:0;
  for(int j=0;j<8;++j){float t=j*Pi/4,t1=(j+1)*Pi/4;tri(p,p+u*std::cos(t)+v*std::sin(t),p+u*std::cos(t1)+v*std::sin(t1),c);}material_=0;
 };
 auto rod=[&](Vec3 p,Vec3 q,float r,float end,Color c){
  Vec3 n=unit(q-p),u=unit(cross(n,{.13f,1,.21f})),v=cross(n,u),mid=(p+q)*.5f;Vec3 b[6],e[6];
  for(int j=0;j<6;++j){Vec3 radial=u*std::cos(j*Pi/3)+v*std::sin(j*Pi/3);b[j]=p+radial*r;e[j]=q+radial*end;}
  for(int j=0;j<6;++j){int k=(j+1)%6;Color col=j<3?c:scale(c,.78f);solid(b[j],b[k],e[k],col,mid);solid(b[j],e[k],e[j],col,mid);solid(p,b[k],b[j],c,mid);solid(q,e[j],e[k],c,mid);}
 };
 auto ball=[&](Vec3 p,Vec3 size,Color c){
  const int sides=middle?16:10, levels=middle?8:6;
  Vec3 ring[9][16];
  for(int j=0;j<=levels;++j){float lat=j*Pi/levels;for(int k=0;k<sides;++k){float t=k*2*Pi/sides;ring[j][k]=p+Vec3{std::sin(lat)*std::cos(t)*size.x,std::cos(lat)*size.y,std::sin(lat)*std::sin(t)*size.z};}}
  for(int j=0;j<levels;++j)for(int k=0;k<sides;++k){int n=(k+1)%sides;Color col=j>levels/2?scale(c,.86f):c;if(j)solid(ring[j][k],ring[j+1][k],ring[j][n],col,p);if(j<levels-1)solid(ring[j][n],ring[j+1][k],ring[j+1][n],col,p);}
 };
 auto rivet=[&](Vec3 p,Vec3 n){if(!fine)return;n=unit(n);Vec3 u=unit(cross(n,{.13f,1,.21f}))*.0027f,v=cross(n,u),tip=p+n*.0022f;for(int k=0;k<4;++k){float t=k*Pi/2,t1=(k+1)*Pi/2;tri(tip,p+u*std::cos(t)+v*std::sin(t),p+u*std::cos(t1)+v*std::sin(t1),gold);}};
 auto seam=[&](Vec3 p,Vec3 q,Vec3 n,int count){if(!middle)return;rod(p,q,.0017f,.0017f,dark);for(int k=0;k<count;++k)rivet(mix(p,q,(k+.5f)/count)+n*.003f,n);};
 auto pipe=[&](Vec3 p,Vec3 bend,Vec3 q,float r){if(!middle)return;Vec3 v[5]={p,mix(p,bend,.83f),bend,mix(bend,q,.17f),q};for(int j=0;j<4;++j)rod(v[j],v[j+1],r,r,copper);if(fine)for(int j:{0,3}){Vec3 n=unit(v[j+1]-v[j]),c=mix(v[j],v[j+1],.5f);rod(c-n*.007f,c+n*.007f,r*1.45f,r*1.45f,gold);}};
 auto ring=[&](Vec3 p,Vec3 n,float radius,float thickness,Color col,int segments=12){
  n=unit(n);Vec3 u=unit(cross(n,{.13f,1,.21f})),v=cross(n,u);const int sides=middle?segments:8;
  for(int k=0;k<sides;++k){float t=k*2*Pi/sides,t1=(k+1)*2*Pi/sides;Vec3 radial=u*std::cos(t)+v*std::sin(t),radial1=u*std::cos(t1)+v*std::sin(t1);
   for(int j=0;j<4;++j){float b=j*Pi/2,b1=(j+1)*Pi/2;Vec3 a0=p+radial*(radius+thickness*std::cos(b))+n*(thickness*std::sin(b)),a1=p+radial1*(radius+thickness*std::cos(b))+n*(thickness*std::sin(b)),b0=p+radial*(radius+thickness*std::cos(b1))+n*(thickness*std::sin(b1)),b1p=p+radial1*(radius+thickness*std::cos(b1))+n*(thickness*std::sin(b1));solid(a0,a1,b1p,col,p+radial*radius);solid(a0,b1p,b0,col,p+radial*radius);}
  }
 };
 auto window=[&](Vec3 p,Vec3 n,float r){
  n=unit(n);disc(p,n,r*1.25f,iron);ring(p+n*.004f,n,r,.13f*r,gold);
  disc(p+n*.009f,n,r*.85f,iron);disc(p+n*.012f,n,r*.69f,glass,true);
  disc(p+n*.013f+Vec3{0,r*.21f,0},n,r*.27f,{255,242,199},true);
 };

 const float z[11]={-.52f,-.45f,-.34f,-.21f,-.07f,.09f,.24f,.36f,.435f,.485f,.515f};
 const float width[11]={.022f,.062f,.106f,.14f,.16f,.166f,.16f,.14f,.105f,.061f,.009f};
 const float height[11]={.026f,.075f,.119f,.152f,.170f,.178f,.174f,.154f,.12f,.073f,.012f};
 const int sides=middle?20:12;Vec3 hull[11][20];
 for(int j=0;j<11;++j)for(int k=0;k<sides;++k){float t=k*2*Pi/sides;hull[j][k]={width[j]*std::sin(t),height[j]*std::cos(t),z[j]};}
 for(int j=0;j<10;++j)for(int k=0;k<sides;++k){int next=(k+1)%sides;Color c=j%3==0?scale(bronze,.93f):bronze;Vec3 o{0,0,(z[j]+z[j+1])*.5f};solid(hull[j][k],hull[j][next],hull[j+1][next],c,o);solid(hull[j][k],hull[j+1][next],hull[j+1][k],c,o);}
 for(int k=0;k<sides;++k){solid({0,0,.517f},hull[10][k],hull[10][(k+1)%sides],bronze,{0,0,.49f});solid({0,0,-.523f},hull[0][(k+1)%sides],hull[0][k],dark,{0,0,-.5f});}
 if(middle)for(int j:{2,4,6,8})for(int k=0;k<sides;++k){int next=(k+1)%sides;Vec3 n=unit(Vec3{hull[j][k].x,hull[j][k].y,0});Vec3 p=hull[j][k]+n*.003f,q=hull[j][next]+unit(Vec3{hull[j][next].x,hull[j][next].y,0})*.003f;seam(p,q,n,2);}
 if(middle)for(int k:{3,7,13,17})for(int j=2;j<9;++j){int v=k*sides/20;Vec3 p=hull[j][v],q=hull[j+1][v],n=unit(Vec3{p.x,p.y,0});seam(p+n*.002f,q+n*.002f,n,3);}
 auto cabin=[&](Vec3 c,float halfWidth,float halfLength,float rise){
  Vec3 v[2][12];for(int j=0;j<2;++j)for(int k=0;k<12;++k){float t=k*Pi/6;float x=std::sin(t),z0=std::cos(t);x=std::copysign(std::pow(std::abs(x),.48f),x);z0=std::copysign(std::pow(std::abs(z0),.48f),z0);v[j][k]=c+Vec3{x*halfWidth*(j?.92f:1),rise*j,z0*halfLength*(j?.97f:1)};}
  Vec3 o=c+Vec3{0,rise*.5f,0};for(int k=0;k<12;++k){int next=(k+1)%12;solid(v[0][k],v[0][next],v[1][next],bronze,o);solid(v[0][k],v[1][next],v[1][k],bronze,o);solid(c+Vec3{0,rise,0},v[1][k],v[1][next],dark,o);solid(c,v[0][next],v[0][k],dark,o);if(middle)rod(v[1][k],v[1][next],.004f,.004f,gold);}
 };
 // Continuous walkway supports every railing post and the bow lamp pedestal.
 cabin({0,.152f,-.005f},.109f,.350f,.014f);
 cabin({0,.135f,-.015f},.086f,.226f,.061f);
 cabin({0,.191f,.033f},.060f,.126f,.093f);
 for(int side:{-1,1}){
  for(float zz:{-.15f,-.062f,.026f,.114f})window({side*.086f,.168f,zz},{float(side),0,0},.021f);
  for(float zz:{-.042f,.028f,.098f})window({side*.058f,.239f,zz},{float(side),0,0},.020f);
 }
 window({0,.237f,.158f},{0,0,1},.022f);
 cabin({0,.169f,.295f},.045f,.044f,.029f);
 rod({0,.210f,.285f},{0,.210f,.331f},.025f,.025f,iron);
 window({0,.210f,.334f},{0,0,1},.023f);
 if(middle)for(int side:{-1,1}){
  for(int k=0;k<8;++k){float zz=-.32f+k*.09f;float ww=(k==0||k==7)?.074f:.102f;Vec3 foot{side*ww,.166f,zz},top=foot+Vec3{0,.045f,0};rod(foot,top,.0029f,.0029f,gold);
   if(k<7){float wn=k+1==7?.074f:.102f;Vec3 next{side*wn,.211f,zz+.09f};rod(top,next,.0027f,.0027f,gold);rod(top-Vec3{0,.021f,0},next-Vec3{0,.021f,0},.002f,.002f,dark);}
  }
  rod({side*.026f,.168f,.182f},{side*.026f,.284f,.160f},.0035f,.0035f,gold);
  for(int k=0;k<4;++k)if(side==1){float f=k/3.f;rod({-.026f,.177f+f*.095f,.180f-f*.018f},{.026f,.177f+f*.095f,.180f-f*.018f},.003f,.003f,gold);}
  pipe({side*.095f,.143f,-.09f},{side*.119f,.085f,-.18f},{side*.085f,.096f,-.34f},.009f);
 }
 rod({-.023f,.28f,.061f},{-.023f,.375f,.061f},.010f,.009f,dark);
 pipe({-.023f,.35f,.061f},{-.023f,.382f,.061f},{-.023f,.382f,.089f},.010f);
 disc({-.023f,.382f,.091f},{0,0,1},.008f,glass,true);
 rod({.019f,.283f,-.023f},{.019f,.329f,-.023f},.022f,.022f,bronze);
 ring({.019f,.331f,-.023f},{0,1,0},.023f,.003f,gold);
 disc({.019f,.332f,-.023f},{0,1,0},.018f,iron);
 rod({.037f,.28f,.091f},{.037f,.39f,.091f},.0018f,.0012f,gold);
 // Below the waterline: service hatches and a protected observation dome.
 cabin({0,-.17f,-.002f},.066f,.258f,.025f);
 for(float zz:{.18f,.035f,-.15f}){Vec3 c{0,-.173f,zz};
  for(int side:{-1,1})rod(c+Vec3{side*.049f,0,-.045f},c+Vec3{side*.049f,0,.045f},.003f,.003f,gold);
  for(float end:{-.045f,.045f})rod(c+Vec3{-.049f,0,end},c+Vec3{.049f,0,end},.003f,.003f,gold);
  if(fine)for(int k=0;k<4;++k)rod(c+Vec3{-.032f,-.002f,-.028f+k*.018f},c+Vec3{.032f,-.002f,-.028f+k*.018f},.004f,.004f,iron);
 }
 ball({0,-.174f,-.068f},{.048f,.037f,.048f},{62,109,112});ring({0,-.178f,-.068f},{0,1,0},.049f,.006f,gold);
 if(middle){rod({-.046f,-.185f,-.068f},{0,-.214f,-.068f},.0026f,.0026f,gold);rod({0,-.214f,-.068f},{.046f,-.185f,-.068f},.0026f,.0026f,gold);}
 // Lower flank access panels visible in the title drawing: framed louvers and return hose.
 if(middle)for(int side:{-1,1}){
  Vec3 c{side*.132f,-.087f,-.165f},up{side*.5f,.866f,0},across{0,0,1},normal{side*.866f,-.5f,0};
  Vec3 p=c-up*.029f-across*.060f,q=c+up*.029f-across*.060f,r=c+up*.029f+across*.060f,t=c-up*.029f+across*.060f;
  tri(p,q,r,dark);tri(p,r,t,dark);
  rod(p,q,.0035f,.0035f,gold);rod(q,r,.0035f,.0035f,gold);rod(r,t,.0035f,.0035f,gold);rod(t,p,.0035f,.0035f,gold);
  for(int k=0;k<4;++k){Vec3 b=c+across*(.006f+k*.011f)+normal*.003f;rod(b-up*.018f,b+up*.018f,.003f,.003f,iron);}
  pipe(c-across*.031f+normal*.008f,c-across*.039f-up*.067f+normal*.015f,c+across*.045f-up*.067f,.007f);
 }
 // Four blade screw inside a protective frame, and four independent tail planes.
 rod({0,0,-.48f},{0,0,-.584f},.020f,.016f,iron);
 ring({0,0,-.575f},{0,0,1},.126f,.009f,dark,16);ring({0,0,-.602f},{0,0,1},.126f,.006f,gold,16);
 for(int k=0;k<4;++k){float t=k*Pi*.5f;Vec3 r{std::cos(t),std::sin(t),0},v{-std::sin(t),std::cos(t),0};Vec3 base{0,0,-.454f},tip=r*.146f+Vec3{0,0,-.57f};
  tri(base+r*.042f,tip+v*.014f,tip-v*.014f,bronze);tri(base+r*.042f,tip-v*.014f,r*.07f+Vec3{0,0,-.59f},dark);
  if(middle)rod(r*.126f+Vec3{0,0,-.575f},r*.126f+Vec3{0,0,-.602f},.005f,.005f,gold);
  float spin=t+a.phase*.7f;Vec3 rr{std::cos(spin),std::sin(spin),0},vv{-std::sin(spin),std::cos(spin),0};Vec3 c{0,0,-.585f};
  tri(c+rr*.016f,c+rr*.098f+vv*.028f+Vec3{0,0,.006f},c+rr*.105f-vv*.012f,bronze);tri(c+rr*.016f,c+rr*.105f-vv*.012f,c+rr*.025f-vv*.02f,gold);
 }
 ball({0,0,-.589f},{.024f,.024f,.023f},bronze);
 material_=0;
}
}
