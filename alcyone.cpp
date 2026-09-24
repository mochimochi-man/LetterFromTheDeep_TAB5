// Nereid, the submersible carrier, as the Alcyone. A model study by another hand;
// only the mesh is taken, and the two methods it exposes are renamed to the ship's
// name in this game. It carries no behaviour, no route and no catalogue entry.
#include "scene.h"
namespace abyss {
void Scene::poseAlcyone(const MachinePose& p){count=0;overflow=false;lighting_=1;viewer_=p.position+Vec3{0,0,p.length*1.4f};alcyoneMesh(p);}
void Scene::alcyoneMesh(const MachinePose& a){
 material_=0;
 const float d2=dot(a.position-viewer_,a.position-viewer_);
 const bool fine=!crowded()&&d2<a.length*a.length*9;
 const bool middle=!crowded()&&d2<a.length*a.length*36;
 const Color bronze{112,111,91},dark{94,64,36},gold{204,166,95},iron{31,39,39},copper{201,125,70},glass{255,205,108};
 bool upperAssembly=false;
 constexpr float DeckLift=0.f;
 auto w=[&](Vec3 p){if(upperAssembly)p.y+=DeckLift;return a.position+rotateY(p,a.yaw)*a.length;};
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
  const int segments=radius<.005f?4:8;
  for(int j=0;j<segments;++j){float t=j*2*Pi/segments,t1=(j+1)*2*Pi/segments;tri(p,p+u*std::cos(t)+v*std::sin(t),p+u*std::cos(t1)+v*std::sin(t1),c);}material_=0;
 };
 auto rod=[&](Vec3 p,Vec3 q,float r,float end,Color c){
  Vec3 n=unit(q-p),u=unit(cross(n,{.13f,1,.21f})),v=cross(n,u),mid=(p+q)*.5f;Vec3 b[6],e[6];const int count=r<.005f?4:6;
  for(int j=0;j<count;++j){Vec3 radial=u*std::cos(j*2*Pi/count)+v*std::sin(j*2*Pi/count);b[j]=p+radial*r;e[j]=q+radial*end;}
  for(int j=0;j<count;++j){int k=(j+1)%count;Color col=j<count/2?c:scale(c,.78f);solid(b[j],b[k],e[k],col,mid);solid(b[j],e[k],e[j],col,mid);}
  for(int j=1;j<count-1;++j){solid(b[0],b[j+1],b[j],c,mid);solid(e[0],e[j],e[j+1],c,mid);}
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
  n=unit(n);disc(p,n,r*1.25f,iron);ring(p+n*.004f,n,r,.13f*r,gold,r<.012f?8:12);
  disc(p+n*.009f,n,r*.85f,iron);disc(p+n*.012f,n,r*.69f,glass,true);
  disc(p+n*.013f+Vec3{0,r*.21f,0},n,r*.27f,{255,242,199},true);
 };


 // Long armored pressure hull, broad at the hangar and tapered into a ram bow.
 const float zs[12]={-.62f,-.53f,-.43f,-.32f,-.18f,0,.18f,.33f,.45f,.53f,.60f,.63f};
 const float rx[12]={.021f,.062f,.10f,.135f,.156f,.17f,.181f,.18f,.171f,.16f,.141f,.117f};
 const float ry[12]={.027f,.048f,.078f,.10f,.12f,.123f,.123f,.119f,.106f,.09f,.074f,.063f};
 const int sides=middle?16:10;Vec3 hull[12][16];
 for(int j=0;j<12;++j)for(int k=0;k<sides;++k){float t=k*2*Pi/sides;hull[j][k]={rx[j]*std::sin(t),ry[j]*std::cos(t),zs[j]};}
 for(int j=0;j<11;++j)for(int k=0;k<sides;++k){int next=(k+1)%sides;Vec3 o{0,0,(zs[j]+zs[j+1])*.5f};Color c=k>sides/4&&k<sides*3/4?scale(bronze,.8f):bronze;solid(hull[j][k],hull[j][next],hull[j+1][next],c,o);solid(hull[j][k],hull[j+1][next],hull[j+1][k],c,o);}
 if(middle)for(int j:{2,4,6,8})for(int k=0;k<sides;++k){int next=(k+1)%sides;Vec3 n=unit(Vec3{hull[j][k].x,hull[j][k].y,0});seam(hull[j][k]+n*.002f,hull[j][next]+unit(Vec3{hull[j][next].x,hull[j][next].y,0})*.002f,n,2);}
 // Two inset fictional energy apertures. The front bulkhead is triangulated
 // around both mouths; the inside walls continue into the pressure hull.
 for(int side:{-1,1}){
  Vec3 c{side*.053f,0,.63f};float angles[48];int count=0;
  for(int k=0;k<24;++k)angles[count++]=k*Pi/12;
  for(int k=0;k<sides;++k)if(side*hull[11][k].x>=-.00001f){float t=std::atan2(hull[11][k].y,hull[11][k].x-c.x);if(t<0)t+=2*Pi;angles[count++]=t;}
  std::sort(angles,angles+count);
  auto boundary=[&](float t){Vec3 dir{std::cos(t),std::sin(t),0};float distance=1;
   for(int k=0;k<sides;++k){Vec3 p=hull[11][k],edge=hull[11][(k+1)%sides]-p;float determinant=dir.x*edge.y-dir.y*edge.x;if(std::abs(determinant)<.000001f)continue;Vec3 delta=p-c;float d=(delta.x*edge.y-delta.y*edge.x)/determinant,u=(delta.x*dir.y-delta.y*dir.x)/determinant;if(d>=0&&u>=-.00001f&&u<=1.00001f)distance=std::min(distance,d);}
   if(side*dir.x<-.00001f)distance=std::min(distance,-c.x/dir.x);
   return c+dir*distance;
  };
  for(int k=0;k<count;++k){float t=angles[k],u=k+1<count?angles[k+1]:angles[0]+2*Pi;Vec3 d{std::cos(t),std::sin(t),0},e{std::cos(u),std::sin(u),0};Vec3 lip=c+d*.030f,lip1=c+e*.030f,p=boundary(t),q=boundary(u),inner=c+d*.020f+Vec3{0,0,-.052f},inner1=c+e*.020f+Vec3{0,0,-.052f};
   tri(lip,p,q,dark);tri(lip,q,lip1,dark);tri(lip,lip1,inner1,{51,55,51});tri(lip,inner1,inner,{68,72,61});
  }
  ring(c+Vec3{0,0,.002f},{0,0,1},.032f,.004f,iron,16);
  ring(c+Vec3{0,0,.004f},{0,0,1},.034f,.0018f,gold,16);
  disc(c+Vec3{0,0,-.053f},{0,0,1},.020f,{12,20,23});
  disc(c+Vec3{0,0,-.052f},{0,0,1},.008f,{68,166,181},true);
  if(fine)for(int k=0;k<8;++k){float t=k*Pi/4;Vec3 d{std::cos(t),std::sin(t),0};rod(c+d*.028f+Vec3{0,0,-.008f},c+d*.019f+Vec3{0,0,-.047f},.0014f,.0014f,copper);}
 }
 // Hull section lookup for external attachment shoes. No hidden support endpoints.
 auto skinX=[&](float z,float y){int j=0;while(j<10&&zs[j+1]<z)++j;float f=clampf((z-zs[j])/(zs[j+1]-zs[j]),0,1),result=0;
  for(int k=0;k<sides;++k){int next=(k+1)%sides;Vec3 p=mix(hull[j][k],hull[j+1][k],f),q=mix(hull[j][next],hull[j+1][next],f);if((p.y-y)*(q.y-y)<=0&&std::abs(q.y-p.y)>.000001f){float x=p.x+(q.x-p.x)*(y-p.y)/(q.y-p.y);result=std::max(result,std::abs(x));}}
  return result;
 };
 auto deckhouse=[&](Vec3 c,Vec3 dim){
  Vec3 v[2][8];for(int j=0;j<2;++j)for(int k=0;k<8;++k){float t=k*Pi/4;v[j][k]=c+Vec3{std::sin(t)*dim.x*(j?.8f:1),j*dim.y,std::cos(t)*dim.z};}
  Vec3 o=c+Vec3{0,dim.y*.5f,0};for(int k=0;k<8;++k){int n=(k+1)%8;solid(v[0][k],v[0][n],v[1][n],bronze,o);solid(v[0][k],v[1][n],v[1][k],bronze,o);solid(c+Vec3{0,dim.y,0},v[1][k],v[1][n],dark,o);if(middle)rod(v[1][k],v[1][n],.003f,.003f,gold);}
 };

 upperAssembly=true;
 // Aircraft-carrier-like forward deck with chamfered corners and a thick fascia.
 Vec3 edge[8]={{-.135f,.145f,-.40f},{.135f,.145f,-.40f},{.205f,.145f,.04f},{.205f,.145f,.52f},{.17f,.145f,.65f},{-.17f,.145f,.65f},{-.205f,.145f,.52f},{-.205f,.145f,.04f}};
 Vec3 center{0,.145f,.22f};
 for(int k=0;k<8;++k){int next=(k+1)%8;tri(center,edge[k],edge[next],{89,91,78});Vec3 down{0,-.038f,0};tri(edge[k],edge[k]+down,edge[next]+down,bronze);tri(edge[k],edge[next]+down,edge[next],bronze);tri(center+down,edge[next]+down,edge[k]+down,dark);if(middle)seam(edge[k]+Vec3{0,.003f,0},edge[next]+Vec3{0,.003f,0},{0,1,0},6);}
 upperAssembly=false;
 // Continuous load path: hull saddle -> column -> crossbeam -> three longitudinal
 // girders -> deck underside. Every station supports the deck, including the bridge
 // and aft machinery. Nothing here is just a front-facing decorative lattice.
 if(middle){
  auto skinY=[&](float x,float z){int j=0;while(j<10&&zs[j+1]<z)++j;float f=clampf((z-zs[j])/(zs[j+1]-zs[j]),0,1),height=-1;
   for(int k=0;k<sides;++k){int next=(k+1)%sides;Vec3 p=mix(hull[j][k],hull[j+1][k],f),q=mix(hull[j][next],hull[j+1][next],f);if((p.x-x)*(q.x-x)<=0&&std::abs(q.x-p.x)>.000001f)height=std::max(height,p.y+(q.y-p.y)*(x-p.x)/(q.x-p.x));}return height;
  };
  auto bar=[&](Vec3 p,Vec3 q,float r,Color c){Vec3 axis=unit(q-p),u=unit(cross(axis,{.17f,1,.3f}))*r,v=unit(cross(axis,u))*r;for(Vec3 d:{u,v}){tri(p-d,p+d,q+d,c);tri(p-d,q+d,q-d,c);}};
  Vec3 previous[3],previousFoot[3];
  for(int bay=0;bay<9;++bay){float z=-.35f+bay*.12f,half=z<.04f?.135f+(z+.4f)*(.070f/.44f):z<=.52f?.205f:.205f-(z-.52f)*(.035f/.13f);
   // Narrower stern saddles follow the hull, while full-width crossbeams support
   // the overhangs through cantilever knee braces.
   int section=0;while(section<10&&zs[section+1]<z)++section;
   float f=(z-zs[section])/(zs[section+1]-zs[section]),hullWidth=rx[section]+(rx[section+1]-rx[section])*f;
   float spread=hullWidth*.57f;Vec3 foot[3],top[3];
   for(int col=0;col<3;++col){float x=(col-1)*spread;foot[col]={x,skinY(x,z)+.003f,z};top[col]={x,.105f,z};
    if(foot[col].y<top[col].y-.001f){rod(foot[col],top[col],.0022f,.0022f,bronze);disc(foot[col],{0,1,0},.0045f,iron);}
    if(bay){rod(previous[col],top[col],.0028f,.0028f,iron);
     // Fore-aft braces connect adjacent load-bearing columns above the skin.
     Vec3 start=previousFoot[col],end=top[col];float startY=start.y;
     for(int k=1;k<24;++k){float t=k/24.f;Vec3 q=mix(start,end,t);startY=std::max(startY,(skinY(q.x,q.z)+.0025f-t*end.y)/(1-t));}
     if(startY<.102f){start.y=startY;bar(start,end,.0016f,gold);}
    }
    previous[col]=top[col];previousFoot[col]=foot[col];
   }
   rod({-half+.004f,.105f,z},{half-.004f,.105f,z},.0028f,.0028f,iron);
   for(int side:{-1,1}){int col=side<0?0:2;Vec3 knee=mix(foot[col],top[col],.45f),overhang{side*(half-.009f),.104f,z};if(foot[col].y<top[col].y-.003f)bar(knee,overhang,.002f,bronze);}
   for(int col=0;col<2;++col)for(int direction=0;direction<2;++direction){int first=col+direction,last=col+1-direction;Vec3 start=foot[first],end=top[last];float startY=start.y;
    for(int k=1;k<24;++k){float t=k/24.f;Vec3 q=mix(start,end,t);startY=std::max(startY,(skinY(q.x,z)+.0025f-t*end.y)/(1-t));}
    if(startY<.102f){start.y=startY;bar(start,end,.0014f,direction?iron:gold);}
   }
  }
 }
 // Four compact ballast capsules seated against the hull below the deck.
 // White bands are painted surface segments, not floating rings.
 auto capsule=[&](Vec3 c,Vec3 axis,float length,float radius){
  axis=unit(axis);Vec3 u{0,1,0},v=cross(axis,u);
  const int n=6;float h=length*.5f-radius;
  const float t[10]={-h-radius,-h-radius*.7071f,-h,h*.38f,h*.45f,h*.62f,h*.69f,h,h+radius*.7071f,h+radius};
  const float r[10]={0,radius*.7071f,radius,radius,radius,radius,radius,radius,radius*.7071f,0};
  Vec3 points[10][6];
  for(int j=0;j<10;++j)for(int k=0;k<n;++k){float angle=k*2*Pi/n;points[j][k]=c+axis*t[j]+(u*std::cos(angle)+v*std::sin(angle))*r[j];}
  for(int j=0;j<9;++j)for(int k=0;k<n;++k){int q=(k+1)%n;Color col=(j==3||j==5)?Color{242,239,220}:Color{220,66,39};
   if(j)solid(points[j][k],points[j+1][k],points[j][q],col,c+axis*((t[j]+t[j+1])*.5f));
   if(j<8)solid(points[j][q],points[j+1][k],points[j+1][q],col,c+axis*((t[j]+t[j+1])*.5f));
  }
  // Two solid rectangular support arms per tank, from hull pads to the
  // outward lower shoulder. Replaces the former short hidden mounting feet.
  if(middle)for(float t0:{-.78f,.78f}){
   float side=c.x<0?-1.f:1.f,z=c.z+h*t0;
   Vec3 base{side*(skinX(z,.008f)+.001f),.008f,z};
   // Closed, thick box-section cradle, with mitered continuous corners.
   // Stand-off clearance makes the supports read as hardware, not paint.
   Vec3 path[5]={base,
    {c.x,c.y-radius-.016f,z},
    {c.x+side*(radius*.866025f+.016f),c.y-radius*.50f,z},
    {c.x+side*(radius*.866025f+.016f),c.y+radius*.50f,z},
    {c.x+side*(radius*.36f+.016f),c.y+radius*.82f,z}};
   Vec3 width{0,0,.012f},section[5][4];
   for(int j=0;j<5;++j){
    Vec3 before=unit(path[j?j:1]-path[j?j-1:0]);
    Vec3 after=unit(path[j<4?j+1:4]-path[j<4?j:3]);
    Vec3 n0=unit(cross(before,width))*side,n1=unit(cross(after,width))*side;
    Vec3 normal=unit(n0+n1),depth=normal*(.007f/std::max(.5f,dot(normal,n1)));
    section[j][0]=path[j]-width-depth;section[j][1]=path[j]+width-depth;
    section[j][2]=path[j]+width+depth;section[j][3]=path[j]-width+depth;
   }
   for(int j=0;j<4;++j){Vec3 origin=(path[j]+path[j+1])*.5f;
    for(int k=0;k<4;++k){int next=(k+1)%4;Color col=k==0?iron:k==2?Color{177,149,91}:bronze;
     solid(section[j][k],section[j][next],section[j+1][next],col,origin);
     solid(section[j][k],section[j+1][next],section[j+1][k],col,origin);
    }
   }
   for(int j:{0,4}){Vec3 origin=path[j==0?1:3];solid(section[j][0],section[j][1],section[j][2],gold,origin);solid(section[j][0],section[j][2],section[j][3],gold,origin);}
   Vec3 pad[4];for(int k=0;k<4;++k){float y=.008f+(k<2?-.012f:.012f),zz=z+((k==0||k==3)?-.013f:.013f);pad[k]={side*(skinX(zz,y)+.002f),y,zz};}
   tri(pad[0],pad[1],pad[2],iron);tri(pad[0],pad[2],pad[3],iron);
  }
 };
 // Two longitudinal capsules per flank, arranged fore and aft.
 // Their tops stay below the existing deck; shoulders nest against the hull.
 for(int side:{-1,1}){
  capsule({side*(skinX(.395f,.060f)+.040f),.060f,.395f},{0,0,1},.39f,.048f);
  capsule({side*(skinX(-.065f,.060f)+.040f),.060f,-.065f},{0,0,1},.39f,.048f);
 }
 upperAssembly=true;
 // Recessed elevator and the two launching rails lead out through the blunt bow.
 Vec3 e0{-.115f,.148f,.02f},e1{.025f,.148f,.02f},e2{.025f,.148f,.21f},e3{-.115f,.148f,.21f};tri(e0,e1,e2,iron);tri(e0,e2,e3,iron);
 for(float x:{-.11f,.02f})rod({x,.153f,.03f},{x,.153f,.61f},.0035f,.0035f,{242,244,237});
 if(middle)for(int k=0;k<8;++k){float z=.025f+k*.078f;disc({-.163f,.15f,z},{0,1,0},.004f,{104,226,204},true);rod({-.193f,.148f,z},{-.193f,.161f,z},.002f,.002f,gold);if(k<7)rod({-.193f,.161f,z},{-.193f,.161f,z+.078f},.002f,.002f,gold);}
 // Offset bridge frees the deck on the port side. Multiple platforms give a layered silhouette.
 deckhouse({.091f,.146f,-.074f},{.065f,.059f,.166f});
 deckhouse({.105f,.2f,-.047f},{.052f,.057f,.105f});
 deckhouse({.105f,.252f,-.033f},{.060f,.025f,.08f});
 for(int side:{-1,1})for(float z:{-.099f,-.047f,.005f})window({.105f+side*.048f,.232f,z},{float(side),0,0},.009f);
 window({.105f,.232f,.058f},{0,0,1},.018f);
 rod({.105f,.27f,-.04f},{.105f,.355f,-.04f},.006f,.005f,iron);
 rod({.054f,.323f,-.04f},{.155f,.323f,-.04f},.003f,.003f,gold);
 window({.105f,.354f,-.032f},{0,0,1},.012f);
 // Dense engineering cluster: pressure accumulators, external jackets and connected manifolds.
 for(int k=0;k<3;++k){float x=-.075f+k*.055f,z=-.225f-k*.025f,h=.068f+k*.024f;
  rod({x,.109f,z},{x,.16f+h,z},.025f,.025f,bronze);
  ball({x,.16f+h,z},{.025f,.023f,.025f},copper);
  ring({x,.155f,z},{0,1,0},.027f,.0035f,gold,8);
  ring({x,.15f+h,z},{0,1,0},.027f,.0035f,gold,8);
  pipe({x,.17f+h,z},{x-.015f,.205f+h,z-.035f},{-.12f,.18f,-.335f},.008f);
 }
 ball({-.077f,.165f,-.13f},{.044f,.046f,.086f},{137,91,52});
 for(float z:{-.175f,-.105f})ring({-.077f,.165f,z},{0,0,1},.046f,.004f,gold);
 pipe({-.11f,.18f,-.08f},{-.14f,.23f,-.05f},{.049f,.205f,-.02f},.01f);
 pipe({.145f,.195f,-.13f},{.178f,.17f,-.21f},{.10f,.11f,-.32f},.01f);
 // Fictional induction chamber: luminous core inside two structural rings, all mounted on feet.
 Vec3 core{-.112f,.19f,-.31f};
 ball(core,{.028f,.036f,.036f},{42,106,102});
 for(float x:{-.14f,-.085f}){ring({x,.19f,-.31f},{1,0,0},.044f,.007f,gold);window({x,.19f,-.31f},{x<-.1f?-1.f:1.f,0,0},.022f);rod({x,.15f,-.31f},{x,.095f,-.31f},.006f,.006f,iron);}
 // Dock-handling crane: an open triangular truss, not another gun barrel.
 rod({-.145f,.12f,-.20f},{-.145f,.28f,-.20f},.005f,.005f,gold);
 rod({-.145f,.28f,-.20f},{-.20f,.26f,.02f},.005f,.004f,gold);
 rod({-.145f,.23f,-.20f},{-.20f,.26f,.02f},.003f,.003f,iron);
 rod({-.20f,.26f,.02f},{-.20f,.177f,.02f},.0015f,.0015f,iron);
 upperAssembly=false;
 // Inset flank lights remain below the deck's overhang.
 for(int side:{-1,1})for(float z:{-.27f,-.17f,-.07f,.04f})window({side*(z<-.2f?.146f:.174f),0,z},{float(side),0,0},.007f);
 upperAssembly=true;
 // Two low naval turrets; stylized barrels are visual props only.
 for(float zz:{-.365f}){
  float base=.148f; // pedestal seats on the extended raised deck
  rod({0,base-.006f,zz},{0,base+.014f,zz},.046f,.043f,iron);
  deckhouse({0,base+.006f,zz},{.043f,.025f,.048f});
  float direction=zz>0?1.f:-1.f;
  for(int side:{-1,1}){Vec3 p{side*.014f,base+.025f,zz+direction*.02f},q=p+Vec3{0,.013f,direction*.10f};rod(p,q,.007f,.006f,bronze);ring(q,{0,0,direction},.006f,.002f,gold,8);disc(q+Vec3{0,0,direction*.001f},{0,0,direction},.004f,iron);}
 }
 upperAssembly=false;
 // Ventral hangar caisson. Flared coamings join the doors to the pressure
 // hull; visible sidewalls, end bulkheads, rails and hinge housings define the volume.
 Vec3 top[4]={{-.088f,-.072f,-.30f},{.088f,-.072f,-.30f},{.088f,-.072f,.22f},{-.088f,-.072f,.22f}};
 Vec3 bottom[4]={{-.071f,-.154f,-.30f},{.071f,-.154f,-.30f},{.071f,-.154f,.22f},{-.071f,-.154f,.22f}};
 for(int k=0;k<4;++k){int next=(k+1)%4;tri(top[k],bottom[k],bottom[next],bronze);tri(top[k],bottom[next],top[next],bronze);if(middle)rod(bottom[k],bottom[next],.003f,.003f,iron);}
 tri(bottom[0],bottom[1],bottom[2],iron);tri(bottom[0],bottom[2],bottom[3],iron);
 for(int side:{-1,1}){
  for(int j=0;j<7;++j){float z=-.285f+j*.071f;Vec3 a0{side*.003f,-.157f,z},a1{side*.063f,-.157f,z},b0=a0+Vec3{0,0,.065f},b1=a1+Vec3{0,0,.065f};tri(a0,a1,b1,bronze);tri(a0,b1,b0,bronze);
   if(middle){rod(a0,a1,.0012f,.0012f,dark);rod({side*.074f,-.15f,z+.01f},{side*.074f,-.15f,z+.045f},.003f,.003f,gold);rod({side*.085f,-.083f,z},{side*.073f,-.15f,z},.0022f,.0022f,iron);}
  }
  // One porthole in each full sidewall bay, centered between its frames.
  for(int bay=0;bay<7;++bay){
   float left=-.285f+bay*.071f,right=bay<6?left+.071f:.22f;
   float y=-.130f,x=.088f+(y+.072f)*(.017f/.082f);
   Vec3 n=unit(Vec3{float(side),-.017f/.082f,0}),u{0,0,1},v=cross(n,u);
   Vec3 center{side*x,y,(left+right)*.5f};center=center+n*.002f;
   // Octagonal circular frame and inset illuminated glass; polygon fans avoid
   // redundant center triangles while retaining the round silhouette.
   for(int layer=0;layer<2;++layer){float radius=layer?.0073f:.0105f;
    Vec3 p[8],origin=center+n*(layer?.0007f:0.f);
    for(int k=0;k<8;++k){float t=k*Pi/4;p[k]=origin+(u*std::cos(t)+v*std::sin(t))*radius;}
    material_=layer?7:0;
    for(int k=1;k<7;++k)tri(p[0],p[k],p[k+1],layer?Color{246,198,106}:gold);
   }
   material_=0;
  }
  rod({side*.062f,-.163f,-.28f},{side*.062f,-.163f,.20f},.0025f,.0025f,gold);
  if(middle)for(int j=0;j<5;++j){float z=-.26f+j*.105f;disc({side*.067f,-.166f,z},{0,-1,0},.003f,{98,213,177},true);}
  if(fine)for(float z:{-.25f,.15f}){rod({side*.088f,-.105f,z},{side*.076f,-.143f,z},.003f,.003f,iron);rod({side*.087f,-.106f,z},{side*.081f,-.125f,z},.0045f,.0045f,copper);}
 }
 // Paired aft machinery pods and independently framed screws.
 for(int side:{-1,1}){
  ball({side*.067f,-.013f,-.445f},{.039f,.037f,.105f},bronze);
  rod({side*.067f,-.013f,-.50f},{side*.067f,-.013f,-.65f},.012f,.01f,iron);
  ring({side*.067f,-.013f,-.625f},{0,0,1},.058f,.004f,gold,12);
  // Bearing housing, two-hoop propeller cage and longitudinal guard struts.
  rod({side*.067f,-.013f,-.518f},{side*.067f,-.013f,-.572f},.022f,.017f,bronze);
  ring({side*.067f,-.013f,-.563f},{0,0,1},.020f,.004f,gold,12);
  ring({side*.067f,-.013f,-.651f},{0,0,1},.060f,.005f,iron,12);
  ball({side*.067f,-.013f,-.639f},{.016f,.016f,.025f},bronze);
  if(middle){
   for(int k=0;k<6;++k){float angle=k*Pi/3;Vec3 radial{std::cos(angle),std::sin(angle),0};
    Vec3 center{side*.067f,-.013f,0};
    rod(center+radial*.058f+Vec3{0,0,-.625f},center+radial*.060f+Vec3{0,0,-.651f},.003f,.003f,bronze);
    if(k%2==0)rod(center+radial*.023f+Vec3{0,0,-.55f},center+radial*.058f+Vec3{0,0,-.625f},.003f,.004f,iron);
   }
   for(float z:{-.425f,-.477f})ring({side*.067f,-.013f,z},{0,0,1},.035f,.0035f,gold,12);
   pipe({side*.081f,.02f,-.395f},{side*.114f,.041f,-.474f},{side*.081f,.006f,-.548f},.006f);
   window({side*.086f,.013f,-.492f},{float(side),.3f,0},.007f);
  }

  for(int k=0;k<4;++k){float t=k*Pi/2+a.phase;Vec3 c{side*.067f,-.013f,-.626f},r{std::cos(t),std::sin(t),0},v{-std::sin(t),std::cos(t),0};tri(c,c+r*.047f+v*.013f,c+r*.048f-v*.009f,bronze);}
  Vec3 root{side*.04f,0,-.44f},tip{side*.18f,.008f,-.57f},back{side*.05f,0,-.61f};tri(root,tip,back,bronze);if(middle){rod(root,tip,.004f,.003f,gold);rod(tip,back,.003f,.003f,dark);}
 }
 tri({0,.024f,-.49f},{0,.151f,-.57f},{0,.02f,-.64f},bronze);
 tri({0,-.024f,-.49f},{0,-.103f,-.57f},{0,-.02f,-.64f},dark);
 upperAssembly=true;
 // Human-scale service fittings. A door is only .018 model units high,
 // deliberately much smaller than the main tanks and launch deck.
 auto plate=[&](Vec3 c,Vec3 u,Vec3 v,Color color){tri(c-u-v,c+u-v,c+u+v,color);tri(c-u-v,c+u+v,c-u+v,color);};
 if(middle){
  // Mirror the service doors on both bridge flanks, conforming to the octagonal skin.
  auto bridgeSkin=[&](int side,float y,float z){float level=clampf((y-.146f)/.059f,0,1),longitudinal=std::abs(z+.074f)/.166f;float width=.065f*(1-.2f*level)*(1-(1-.7071068f)*longitudinal/.7071068f);return Vec3{.091f+side*(width+.0008f),y,z};};
  for(int side:{-1,1})for(float z:{-.13f,-.075f,-.02f}){
   Vec3 p=bridgeSkin(side,.152f,z-.0045f),q=bridgeSkin(side,.17f,z-.0045f),r=bridgeSkin(side,.17f,z+.0045f),t=bridgeSkin(side,.152f,z+.0045f);
   tri(p,q,r,iron);tri(p,r,t,iron);
   rod(p,t,.0008f,.0008f,gold);
   disc(bridgeSkin(side,.164f,z)+Vec3{side*.0003f,0,0},{float(side),0,0},.002f,{155,197,174});
  }
  // Ladder between bridge platforms, rather than another oversized antenna.
  for(float z:{-.12f,-.105f})rod({.155f,.148f,z},{.155f,.20f,z},.001f,.001f,gold);
  for(int j=0;j<9;++j){float y=.15f+j*.006f;rod({.155f,y,-.12f},{.155f,y,-.105f},.0008f,.0008f,iron);}
  // Upper catwalk at the aft bridge face, fine railing with consistent post pitch.
  plate({.105f,.202f,-.148f},{.052f,0,0},{0,0,.009f},dark);
  for(int j=0;j<8;++j){float x=.055f+j*.014f;rod({x,.202f,-.155f},{x,.213f,-.155f},.0009f,.0009f,gold);if(j<7)rod({x,.213f,-.155f},{x+.014f,.213f,-.155f},.0009f,.0009f,gold);}
  // Regular deck access panels, grilles and recessed docking tie-downs.
  for(float z:{.275f,.41f,.545f})for(int side:{-1,1}){
   Vec3 c{side>0?.11f:-.142f,.148f,z};plate(c,{.01f,0,0},{0,0,.017f},iron);
   if(fine)for(int k=0;k<5;++k){float dz=-.012f+k*.006f;plate(c+Vec3{0,.0005f,dz},{.008f,0,0},{0,0,.001f},bronze);}
  }
  if(fine)for(int j=0;j<10;++j)for(float x:{-.075f,-.018f})disc({x,.149f,.245f+j*.036f},{0,1,0},.0017f,iron);
  if(fine){
   for(float z:{.275f,.38f,.485f,.59f})plate({.11f,.1485f,z},{.066f,0,0},{0,0,.0006f},dark);
   for(float x:{.071f,.149f})plate({x,.1485f,.435f},{.0006f,0,0},{0,0,.16f},dark);
   // Equipment lockers beside the elevator; small louver slots establish scale.
   for(int j=0;j<3;++j){Vec3 c{.026f,.152f,-.043f-j*.021f};plate(c,{.006f,0,0},{0,0,.008f},bronze);for(int k=0;k<3;++k)plate(c+Vec3{0,.0005f,-.005f+k*.005f},{.004f,0,0},{0,0,.0008f},iron);}
  }
  upperAssembly=false;
  // Hull utility lines follow its measured skin, with supports every station.
  for(int side:{-1,1})for(int j=0;j<7;++j){float z=-.24f+j*.057f,y=-.027f;
   Vec3 p{side*(skinX(z,y)+.003f),y,z},q{side*(skinX(z+.057f,y)+.003f),y,z+.057f};rod(p,q,.0014f,.0014f,copper);
   if(fine)disc(p,{float(side),0,0},.0028f,iron);
  }
  upperAssembly=true;
  // Pressure gauges, valves and short equipment-scale piping on the tank cluster.
  for(int j=0;j<3;++j){float x=-.075f+j*.055f,z=-.225f-j*.025f;
   disc({x,.185f,z+.026f},{0,0,1},.0045f,iron);disc({x,.185f,z+.027f},{0,0,1},.0031f,{221,218,181});
   rod({x,.185f,z+.028f},{x+.0018f,.186f,z+.028f},.00065f,.00065f,iron);
   ring({x+.025f,.163f,z},{1,0,0},.007f,.0011f,{151,72,46},8);
   rod({x+.025f,.156f,z},{x+.025f,.17f,z},.0009f,.0009f,gold);
  }
 }
 material_=0;
}
}
