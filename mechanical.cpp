#include "scene.h"
namespace abyss {
const char* machineName(MachineKind kind) {
 switch(kind) {
  case MachineKind::ArmoredFish: return "armoured swimmer";
  case MachineKind::WingRay: return "winged drifter";
  default: return "walking tank";
 }
}
MachinePose cityMachinePose(int index,float time) {
 int kind=index/2;float speed=kind==0?.024f:kind==1?.018f:.030f;
 float angle=time*speed+(index%2)*Pi+kind*.7f;
 // All three orbits stay inside CityEdge. The widest of them used to reach 85 metres out,
 // which was open water while the ends of the place were empty shelves and is the face of
 // the cliff now that they are walls.
 float rx=kind==0?52.f:kind==1?68.f:38.f,rz=kind==0?12.f:kind==1?8.f:10.f;
 float y=kind==0?-198.f:kind==1?-183.f:-216.f,z=kind==0?-58.f:kind==1?-37.f:-55.f;
 return {{std::sin(angle)*rx,y+std::sin(angle*2)*1.2f,z+std::cos(angle)*rz},
  std::atan2(rx*std::cos(angle),-rz*std::sin(angle)),kind==0?6.4f:kind==1?5.8f:3.8f,
  time*(kind==2?1.7f:1.1f)+index*1.9f,MachineKind(kind)};
}
void Scene::poseMachine(const MachinePose& p){count=0;overflow=false;lighting_=1;viewer_=p.position+Vec3{0,0,p.length*1.4f};machineMesh(p);}
void Scene::machineMesh(const MachinePose& a) {
 material_=0;
 const float d2=dot(a.position-viewer_,a.position-viewer_);
 const bool fine=!crowded()&&d2<a.length*a.length*9;
 const bool middle=!crowded()&&d2<a.length*a.length*36;
 const Color bronze{188,135,72},dark{94,64,36},gold{233,190,118},iron{31,39,39},copper{201,125,70},glass{255,205,108};
 // The frog lurches forward on each kick and coasts between them, so its whole mesh slides
 // along its own axis. Set where the legs are worked out, read here.
 float surge=0;
 auto w=[&](Vec3 p){if(a.kind==MachineKind::TankFrog){p.y*=.78f;p.z+=surge;}return a.position+rotateY(p,a.yaw)*a.length;};
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
 auto rivet=[&](Vec3 p,Vec3 n){if(!fine)return;n=unit(n);Vec3 u=unit(cross(n,{.13f,1,.21f}))*.0050f,v=cross(n,u),tip=p+n*.0042f;for(int k=0;k<4;++k){float t=k*Pi/2,t1=(k+1)*Pi/2;tri(tip,p+u*std::cos(t)+v*std::sin(t),p+u*std::cos(t1)+v*std::sin(t1),gold);}};
 auto seam=[&](Vec3 p,Vec3 q,Vec3 n,int count){if(!middle)return;rod(p,q,.0017f,.0017f,dark);for(int k=0;k<count;++k)rivet(mix(p,q,(k+.5f)/count)+n*.003f,n);};
 auto pipe=[&](Vec3 p,Vec3 bend,Vec3 q,float r){if(!middle)return;Vec3 v[5]={p,mix(p,bend,.83f),bend,mix(bend,q,.17f),q};for(int j=0;j<4;++j)rod(v[j],v[j+1],r,r,copper);if(fine)for(int j:{0,3}){Vec3 n=unit(v[j+1]-v[j]),c=mix(v[j],v[j+1],.5f);rod(c-n*.007f,c+n*.007f,r*1.45f,r*1.45f,gold);}};
 auto ring=[&](Vec3 p,Vec3 n,float radius,float thickness,Color col,int segments=12){
  n=unit(n);Vec3 u=unit(cross(n,{.13f,1,.21f})),v=cross(n,u);const int sides=middle?segments:8;
  for(int k=0;k<sides;++k){float t=k*2*Pi/sides,t1=(k+1)*2*Pi/sides;Vec3 radial=u*std::cos(t)+v*std::sin(t),radial1=u*std::cos(t1)+v*std::sin(t1);
   for(int j=0;j<4;++j){float b=j*Pi/2,b1=(j+1)*Pi/2;Vec3 a0=p+radial*(radius+thickness*std::cos(b))+n*(thickness*std::sin(b)),a1=p+radial1*(radius+thickness*std::cos(b))+n*(thickness*std::sin(b)),b0=p+radial*(radius+thickness*std::cos(b1))+n*(thickness*std::sin(b1)),b1p=p+radial1*(radius+thickness*std::cos(b1))+n*(thickness*std::sin(b1));solid(a0,a1,b1p,col,p+radial*radius);solid(a0,b1p,b0,col,p+radial*radius);}
  }
 };
 // Parallel return loop: two penetrating necks, seated flanges and a rounded U.
 auto returnLoop=[&](Vec3 inlet,Vec3 outlet,Vec3 normal,float radius,float projection=1.f){
  if(!middle)return;
  Vec3 n=unit(normal);
  // A true U in the plane perpendicular to the hull: both open ends point
  // straight inward. No elbow is inserted between either leg and its socket.
  Vec3 separation=outlet-inlet-n*dot(outlet-inlet,n);
  float bendRadius=length(separation)*.5f;
  Vec3 along=unit(separation),binormal=unit(cross(n,along));
  float plane=std::max(dot(inlet,n),dot(outlet,n))+radius*(projection<1.f?.6f:1.2f);
  Vec3 center=(inlet+outlet)*.5f;
  center=center+n*(plane-dot(center,n));
  Vec3 path[13],tangent[13];
  path[0]=inlet-n*.045f;tangent[0]=n;
  // The semicircle starts and ends tangent to the two straight insertion legs.
  for(int j=0;j<=10;++j){float angle=j*Pi/10;
   path[j+1]=center+along*(-bendRadius*std::cos(angle))+n*(bendRadius*projection*std::sin(angle));
   tangent[j+1]=unit(along*std::sin(angle)+n*(projection*std::cos(angle)));
  }
  path[12]=outlet-n*.045f;tangent[12]=n*(-1);
  Vec3 mesh[13][8];
  for(int j=0;j<13;++j){Vec3 v=cross(tangent[j],binormal);
   for(int k=0;k<8;++k)mesh[j][k]=path[j]+(binormal*std::cos(k*Pi/4)+v*std::sin(k*Pi/4))*radius;
  }
  for(int j=0;j<12;++j)for(int k=0;k<8;++k){int next=(k+1)%8;Vec3 c=(path[j]+path[j+1])*.5f;solid(mesh[j][k],mesh[j][next],mesh[j+1][next],copper,c);solid(mesh[j][k],mesh[j+1][next],mesh[j+1][k],copper,c);}

  for(Vec3 socket:{inlet,outlet}){
   rod(socket-n*.010f,socket+n*.006f,radius*1.7f,radius*1.7f,iron);
   ring(socket+n*.008f,n,radius*1.45f,radius*.24f,gold,8);
  }

 };
 auto window=[&](Vec3 p,Vec3 n,float r){
  n=unit(n);disc(p,n,r*1.25f,iron);ring(p+n*.004f,n,r,.13f*r,gold);
  disc(p+n*.009f,n,r*.85f,iron);disc(p+n*.012f,n,r*.69f,glass,true);
  disc(p+n*.013f+Vec3{0,r*.21f,0},n,r*.27f,{255,242,199},true);
 };
 auto tower=[&](Vec3 base,float width,float depth,float height){
  Vec3 v[2][12];for(int j=0;j<2;++j)for(int k=0;k<12;++k){float t=k*Pi/6;v[j][k]=base+Vec3{std::sin(t)*width*(j?.82f:1),height*j,std::cos(t)*depth*(j?.85f:1)};}
  Vec3 c=base+Vec3{0,height*.5f,0};
  for(int k=0;k<12;++k){int n=(k+1)%12;solid(v[0][k],v[0][n],v[1][n],bronze,c);solid(v[0][k],v[1][n],v[1][k],bronze,c);solid(base+Vec3{0,height,0},v[1][k],v[1][n],gold,c);if(middle)rod(v[1][k],v[1][n],.0035f,.0035f,dark);}
  window(base+Vec3{0,height*.48f,depth*.94f},{0,.1f,1},height*.27f);
  for(int s:{-1,1})window(base+Vec3{s*width*.94f,height*.48f,0},{float(s),.1f,0},height*.27f);
  Vec3 p=base+Vec3{0,height,-depth*.32f};rod(p,p+Vec3{0,.071f,0},.010f,.008f,iron);rod(p+Vec3{0,.071f,0},p+Vec3{0,.071f,.025f},.009f,.009f,gold);disc(p+Vec3{0,.071f,.026f},{0,0,1},.006f,glass,true);
 };
 auto fin=[&](Vec3 root,Vec3 tip,Vec3 rear,Color col){Vec3 c=(root+tip+rear)*(1.f/3);tri(root,tip,c,col);tri(tip,rear,c,scale(col,.85f));tri(rear,root,c,col);if(middle){rod(root,tip,.004f,.002f,gold);rod(tip,rear,.002f,.003f,dark);seam(root,rear,{0,1,0},4);}};
 auto limb=[&](Vec3 p,Vec3 q,float radius){
  Vec3 n=unit(q-p),u=unit(cross(n,{.13f,1,.21f})),v=cross(n,u);const float profile[5]={.58f,.92f,1,.85f,.5f};Vec3 vert[5][10];
  for(int j=0;j<5;++j)for(int k=0;k<10;++k)vert[j][k]=mix(p,q,j*.25f)+(u*std::cos(k*Pi/5)+v*std::sin(k*Pi/5))*(radius*profile[j]);
  for(int j=0;j<4;++j)for(int k=0;k<10;++k){int next=(k+1)%10;Vec3 mid=mix(p,q,(j+.5f)*.25f);solid(vert[j][k],vert[j+1][k],vert[j+1][next],bronze,mid);solid(vert[j][k],vert[j+1][next],vert[j][next],bronze,mid);}
  if(middle)for(int k=0;k<10;++k)seam(vert[1][k],vert[1][(k+1)%10],unit(vert[1][k]-mix(p,q,.25f)),2);
 };
 // Plate courses lie on the same polygon rings as the pressure hull itself.
 auto belt=[&](Vec3 c,Vec3 size,int ring,int first,int last){
  if(!middle) return;
  float lat=(ring+1)*Pi/8;
  for(int k=first;k<last;++k){float t=k*Pi/8,u=(k+1)*Pi/8;Vec3 p=c+Vec3{std::sin(lat)*std::cos(t)*size.x,std::cos(lat)*size.y,std::sin(lat)*std::sin(t)*size.z},q=c+Vec3{std::sin(lat)*std::cos(u)*size.x,std::cos(lat)*size.y,std::sin(lat)*std::sin(u)*size.z};Vec3 n=unit((p+q)*.5f-c);seam(p+n*.003f,q+n*.003f,n,4);}
 };
 float wave=std::sin(a.phase);
 if(a.kind==MachineKind::ArmoredFish){
  const float zz[9]={-.39f,-.30f,-.19f,-.065f,.07f,.19f,.285f,.35f,.385f};
  const float xx[9]={.024f,.085f,.15f,.195f,.208f,.184f,.136f,.083f,.038f};
  const float yy[9]={.038f,.13f,.205f,.25f,.254f,.208f,.147f,.084f,.039f};
  Vec3 v[9][16];for(int j=0;j<9;++j)for(int k=0;k<16;++k){float t=k*Pi/8;v[j][k]={xx[j]*std::sin(t),yy[j]*std::cos(t),zz[j]};}
  for(int j=0;j<8;++j)for(int k=0;k<16;++k){int n=(k+1)%16;Color col=(j==4||j==3)?scale(bronze,.82f):bronze;Vec3 c{0,0,(zz[j]+zz[j+1])*.5f};solid(v[j][k],v[j][n],v[j+1][n],col,c);solid(v[j][k],v[j+1][n],v[j+1][k],col,c);}
  for(int k=0;k<16;++k){solid({0,0,.389f},v[8][k],v[8][(k+1)%16],bronze,{0,0,.35f});solid({0,0,-.397f},v[0][(k+1)%16],v[0][k],dark,{0,0,-.36f});}
  // Rounded jaws and lips give the nose a fish mouth, not a sharp wedge.
  ball({0,-.065f,.273f},{.132f,.072f,.124f},dark);
  rod({-.04f,-.02f,.390f},{.04f,-.02f,.390f},.012f,.012f,bronze);
  rod({-.036f,-.043f,.387f},{.036f,-.043f,.387f},.010f,.010f,gold);
  ball({0,.012f,.225f},{.178f,.179f,.164f},bronze);
  for(int s:{-1,1}){
   // Continuous forehead, with cheek bosses below the curved gill cover.
   for(int plate=0;plate<3;++plate){
    for(int k=0;k<10;++k){float t=-1.18f+k*.236f,u=t+.236f;
     auto g=[&](float angle,float rear){
      float z=.19f-plate*.069f-.13f*std::cos(angle)-rear;int j=0;while(j<7&&zz[j+1]<z)++j;float f=clampf((z-zz[j])/(zz[j+1]-zz[j]),0,1);float rx=xx[j]+(xx[j+1]-xx[j])*f,ry=yy[j]+(yy[j+1]-yy[j])*f;
      return Vec3{s*((rx+.010f)*std::cos(angle)),(ry+.010f)*std::sin(angle),z};
     };
     Vec3 p=g(t,0),q=g(u,0),b=g(t,.056f),c=g(u,.056f);Vec3 o{0,0,0};
     solid(p,q,c,plate==1?gold:bronze,o);solid(p,c,b,plate==1?gold:bronze,o);
     if(middle){rod(p,q,.004f,.004f,gold);for(int r=0;r<1;++r)rivet(mix(p,q,.5f)+Vec3{s*.004f,0,0},{float(s),0,0});}
    }
   }
   if(middle)for(int k=0;k<8;++k){
    float t=-1.08f+k*.27f,u=t+.27f;
    Vec3 p{s*.17f*std::cos(t),.012f+.17f*std::sin(t),.26f-.065f*std::cos(t)},q{s*.17f*std::cos(u),.012f+.17f*std::sin(u),.26f-.065f*std::cos(u)};
    seam(p,q,{float(s),0,0},1);
   }
   Vec3 eye{s*.174f,.065f,.24f};rod(eye-Vec3{s*.02f,0,0},eye,.051f,.047f,iron);window(eye+Vec3{s*.006f,0,0},{float(s),.12f,.22f},.040f);
   // Both pipe necks penetrate the side plating; the U runs parallel to it.
   returnLoop({s*.218f,.018f,.055f},{s*.164f,.018f,-.172f},{float(s),0,0},.017f,.42f);
   // Parallel lower run: its sockets follow the narrowing lower hull section.
   returnLoop({s*.203f,-.075f,.055f},{s*.153f,-.075f,-.172f},{float(s),0,0},.017f,.42f);
   fin({s*.15f,-.117f,.03f},{s*.35f,-.25f,-.22f},{s*.15f,-.205f,-.185f},bronze);
  }
  if(middle)for(int k=0;k<16;++k){float t=k*Pi/8,u=(k+1)*Pi/8;Vec3 p{.151f*std::sin(t),.208f*std::cos(t),-.19f},q{.151f*std::sin(u),.208f*std::cos(u),-.19f};seam(p,q,unit(Vec3{std::sin(t),std::cos(t),0}),2);}
  tower({0,.229f,-.035f},.079f,.099f,.067f);
  fin({0,.227f,-.10f},{0,.329f,-.245f},{0,.108f,-.31f},bronze);
  Vec3 hinge{wave*.028f,0,-.418f};rod({0,0,-.36f},hinge,.033f,.027f,iron);
  fin(hinge,{wave*.06f,.237f,-.575f},{wave*.05f,.038f,-.52f},bronze);
  fin(hinge,{wave*.06f,-.237f,-.575f},{wave*.05f,-.038f,-.52f},bronze);
  for(int side:{-1,1})window(hinge+Vec3{side*.032f,0,0},{float(side),0,0},.017f);
 } else if(a.kind==MachineKind::WingRay){
  ball({0,-.007f,.015f},{.18f,.085f,.278f},bronze);
  for(int side:{-1,1}){
   // A continuous cambered wing with curved leading and trailing edges.
   auto wing=[&](float span,float chord){
    float x=.105f+.53f*span;
    float front=.255f-.10f*span-.355f*span*span;
    float rear=-.225f-.065f*std::sin(span*Pi*.90f)+.065f*span;
    float y=.008f+.145f*span*span+wave*.025f*span*span;
    return Vec3{side*x,y+.057f*std::sin(chord*Pi)*(1-span*.8f),front+(rear-front)*chord};
   };
   for(int j=0;j<8;++j)for(int k=0;k<5;++k){float a0=j/8.f,a1=(j+1)/8.f,b0=k/5.f,b1=(k+1)/5.f;Vec3 p=wing(a0,b0),q=wing(a1,b0),r=wing(a1,b1),t=wing(a0,b1);Color c=(j/3+k/3)%2?bronze:scale(bronze,1.07f);tri(p,q,r,c);tri(p,r,t,c);Vec3 drop{0,-.018f*(1-a0*.8f),0};tri(p+drop,r+drop,q+drop,dark);tri(p+drop,t+drop,r+drop,dark);}
   if(middle)for(int j=0;j<8;++j){seam(wing(j/8.f,0)+Vec3{0,.004f,0},wing((j+1)/8.f,0)+Vec3{0,.004f,0},{0,1,0},3);rod(wing(j/8.f,1),wing((j+1)/8.f,1),.004f,.003f,gold);}
   if(middle)for(int station:{3,6})for(int k=0;k<5;++k)seam(wing(station/8.f,k/5.f)+Vec3{0,.003f,0},wing(station/8.f,(k+1)/5.f)+Vec3{0,.003f,0},{0,1,0},2);
   // Open rolled lobes: curved scoops, not capped cylindrical cannons.
   for(int j=0;j<4;++j)for(int k=0;k<10;++k){
    auto scoop=[&](float length,float angle){float r=.034f+length*.017f;return Vec3{side*(.115f+.025f*length)+std::cos(angle)*r,-.024f-.058f*length+std::sin(angle)*r,.22f+.16f*length};};
    float t=.2f+k*(Pi*1.75f/10),u=t+Pi*1.75f/10;Vec3 p=scoop(j/4.f,t),q=scoop((j+1)/4.f,t),r=scoop((j+1)/4.f,u),b=scoop(j/4.f,u);tri(p,q,r,{126,77,43});tri(p,r,b,{126,77,43});if(j==3&&middle)rod(q,r,.004f,.004f,{154,98,55});
   }
   returnLoop({side*.11f,.053f,.12f},{side*.12f,.053f,-.07f},{0,1,0},.018f);
   window({side*.156f,.032f,.205f},{side*.8f,.25f,.6f},.017f);
  }
  disc({0,-.025f,.289f},{0,-.15f,1},.043f,iron);
  tower({0,.071f,-.018f},.088f,.123f,.063f);
  Vec3 tail[5]={{0,0,-.22f},{wave*.014f,.012f,-.33f},{wave*.03f,.035f,-.44f},{wave*.05f,.072f,-.55f},{wave*.06f,.10f,-.64f}};
  for(int j=0;j<4;++j){rod(tail[j],tail[j+1],.019f-j*.0037f,.015f-j*.0037f,iron);if(middle){Vec3 n=unit(tail[j+1]-tail[j]);rod(tail[j]-n*.008f,tail[j]+n*.008f,.025f-j*.004f,.025f-j*.004f,gold);}}
 } else {
  // A frog does not swim by waving. It lies with its legs stretched out behind it and
  // glides; it folds them up slowly, knees out to the sides; then it kicks once, hard,
  // and glides again. A sine spends as long pushing as recovering, which is why the legs
  // read as clockwork, so the stroke is written out as its three parts instead.
  const float turn=a.phase*(1/(2*Pi));
  const float c=turn-std::floor(turn);
  float extend,spread;                      // 1 = stretched out behind, 0 = folded up
  if(c<.22f)      { const float t=c/.22f;        extend=t*t*(3-2*t);   spread=std::sin(t*Pi); }
  else if(c<.62f) {                              extend=1;             spread=0;              }
  else            { const float t=(c-.62f)/.38f; extend=1-t*t*(3-2*t); spread=0;              }
  // Quick over the kick, slow over the rest: the machine gains on its own orbit as it
  // pushes and falls back through the glide, which is what a frog's speed does.
  surge=.055f*(extend-.5f);
  ball({0,.005f,-.045f},{.24f,.19f,.24f},bronze);
  ball({0,.017f,.174f},{.245f,.155f,.177f},bronze);
  for(int s:{-1,1}){
   ball({s*.148f,.123f,.19f},{.069f,.063f,.064f},{126,77,43});
   window({s*.166f,.139f,.24f},{s*.32f,.30f,1},.045f);
   seam({0,-.019f,.34f},{s*.178f,-.032f,.284f},{s*.25f,0,1},7);
   returnLoop({s*.227f,.047f,.008f},{s*.203f,.047f,-.15f},{float(s),0,0},.015f,.40f);
   // The hip is fixed and the joints below it travel between the two poses. Folded, the
   // knee is out to the side and ahead of the hip with the heel drawn up under it;
   // stretched, the whole leg lies back along the body.
   const Vec3 kneeFold{s*.405f,.020f,.048f},  kneeBack{s*.250f,-.062f,-.175f};
   const Vec3 heelFold{s*.238f,-.120f,-.040f},heelBack{s*.215f,-.150f,-.500f};
   Vec3 hip{s*.195f,-.027f,-.11f},knee=mix(kneeFold,kneeBack,extend),ankle=mix(heelFold,heelBack,extend);
   limb(hip,knee,.066f);ball(knee,{.046f,.040f,.043f},iron);
   limb(knee,ankle,.038f);
   ring(knee+Vec3{s*.05f,0,0},{float(s),0,0},.032f,.009f,gold);
   pipe(hip+Vec3{0,.052f,0},knee+Vec3{0,.043f,0},ankle+Vec3{0,.023f,0},.009f);
   seam(hip+Vec3{0,.052f,0},knee+Vec3{0,.046f,0},{0,1,0},6);
   // The web opens into a paddle through the push and feathers shut the rest of the time,
   // which is the whole point of a webbed foot and the only part of it that shows.
   const float fan=.45f+1.15f*spread,paddle=.60f+.50f*spread;
   Vec3 tips[4];for(int j=0;j<4;++j)tips[j]=ankle+Vec3{s*((j-1.5f)*.059f*fan+.022f),-.025f,(-.15f+(std::abs(j-1.5f))*.02f)*paddle};
   for(int j=0;j<3;++j)tri(ankle,tips[j],tips[j+1],j%2?gold:bronze);
   for(int j=0;j<4;++j)rod(ankle,tips[j],.006f,.004f,gold);
   // The forelimbs do no swimming; they draw in against the hull for the glide and open
   // again while the legs are being folded, so nothing on the machine is ever quite still.
   Vec3 shoulder{s*.194f,-.048f,.166f};
   Vec3 elbow=mix(Vec3{s*.292f,-.140f,.148f},Vec3{s*.238f,-.162f,.096f},extend);
   Vec3 wrist=mix(Vec3{s*.268f,-.232f,.272f},Vec3{s*.206f,-.256f,.182f},extend);
   rod(shoulder,elbow,.030f,.024f,bronze);ball(elbow,{.027f,.024f,.027f},iron);rod(elbow,wrist,.021f,.016f,gold);ring(shoulder,{float(s),-.2f,0},.037f,.008f,gold);
   for(int j=0;j<3;++j){Vec3 tip=wrist+Vec3{s*(j-1)*.037f,-.021f,.068f};rod(wrist,tip,.005f,.003f,gold);if(j<2)tri(wrist,tip,wrist+Vec3{s*j*.037f,-.021f,.068f},bronze);}
  }
  belt({0,.005f,-.045f},{.24f,.19f,.24f},2,8,16);
  belt({0,.005f,-.045f},{.24f,.19f,.24f},4,7,17);
  belt({0,.017f,.174f},{.245f,.155f,.177f},4,0,8);
  if(middle)for(int side:{-1,1})for(int j=1;j<5;++j){
   float a0=j*Pi/8,a1=(j+1)*Pi/8,t=side>0?Pi*.25f:Pi*.75f;
   Vec3 p{.249f*std::sin(a0)*std::cos(t),.017f+.159f*std::cos(a0),.174f+.181f*std::sin(a0)*std::sin(t)},q{.249f*std::sin(a1)*std::cos(t),.017f+.159f*std::cos(a1),.174f+.181f*std::sin(a1)*std::sin(t)};
   seam(p,q,unit(p-Vec3{0,.017f,.174f}),2);
  }
  tower({0,.183f,-.056f},.087f,.095f,.077f);
 }
 material_=0;
}
}
