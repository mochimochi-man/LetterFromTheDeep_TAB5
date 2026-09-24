#include "scene.h"
#if defined(__GNUC__)
#pragma GCC optimize("O3", "fast-math")
#endif
namespace abyss {
void Scene::animalMesh(const ecology::Animal& animal) {
  using ecology::Species;
  Species species=animal.species;
  if(species==Species::Nautilus || species==Species::Coelacanth) { livingRelicMesh(animal); return; }
  if(ecology::ancient(species)) { prehistoricMesh(animal); return; }
  if(species>=Species::Hammerhead && species<=Species::WhaleShark) { largeMarineMesh(animal); return; }
  if(species==Species::Turtle || species==Species::Pufferfish) { turtlePufferMesh(animal); return; }
  material_=0;
  float size=animal.length,phase=animal.phase,activity=animal.activity;
  Vec3 up{0,1,0};
  if(ecology::bottomDweller(species)) {
    float d=std::max(.12f,size*.3f),x=animal.position.x,z=animal.position.z;
    up=unit({surface(x-d,z)-surface(x+d,z),2*d,surface(x,z-d)-surface(x,z+d)});
  }
  Vec3 forward{std::sin(animal.yaw),0,std::cos(animal.yaw)};
  Vec3 right=unit(cross(up,forward)); forward=unit(cross(right,up));
  auto transform=[&](Vec3 p) { return animal.position+(right*p.x+up*p.y+forward*p.z)*size; };
  auto tri=[&](Vec3 a,Vec3 b,Vec3 c,Color color) { add(transform(a),transform(b),transform(c),color); };
  auto panel=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color color) { tri(a,b,c,color); tri(a,c,d,color); };
  // Faces on a closed body: the far side of these is never visible, so they are handed
  // to addSolid, which fixes their winding against "away from the middle of the animal"
  // and lets the renderer drop them. Fins and any other thin plate keep using tri/panel.
  auto turn=[&](Vec3 p) { return right*p.x+up*p.y+forward*p.z; };
  auto triS=[&](Vec3 a,Vec3 b,Vec3 c,Color color) {
    addSolid(transform(a),transform(b),transform(c),color,turn((a+b+c)*(1.0f/3.0f)));
  };
  auto panelS=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color color) { triS(a,b,c,color); triS(a,c,d,color); };
  // On a long thin animal the direction from the origin to a face points along the
  // body rather than out of it, and on a steep taper a radial direction is nearly
  // perpendicular to the face normal - either way the winding test has nothing to
  // work with and flanks drop out. These take the point on the body's own axis that
  // the face belongs to, then use the face's real normal, flipped to face away from
  // that axis. That is unambiguous whatever the taper.
  auto triN=[&](Vec3 a,Vec3 b,Vec3 c,Color color,Vec3 axis) {
    Vec3 n=cross(b-a,c-a);
    if(dot(n,(a+b+c)*(1.f/3.f)-axis)<0) n=n*-1.f;
    addSolid(transform(a),transform(b),transform(c),color,turn(n));
  };
  auto panelN=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color color,Vec3 axis) {
    triN(a,b,c,color,axis); triN(a,c,d,color,axis);
  };
  if(species==Species::Viperfish) {
    // Chauliodus sloani: Australian Museum lateral specimen / identification.
    // A slim silver-black body, short skull, deep gape and elongated first dorsal ray.
    // Keep the existing creature scale, flat shading and cheap distance LOD.
    Vec3 gap=animal.position-viewer_;
    int lod=size*size>dot(gap,gap)*.007f?0:size*size>dot(gap,gap)*.0011f?1:2;
    if(jammed())lod=2;else if(crowded() && lod==0)lod=1;
    const bool detail=lod==0;
    Color back{31,43,53},silver{91,112,125},belly{134,148,151},fin{97,117,126};
    auto sway=[&](float z){float t=clampf((.25f-z)/.8f,0,1);return std::sin(phase-t*2.1f)*t*t*.042f*activity;};
    struct Ring{float z,w,top,bottom;};
    static const Ring rings[]={{.345f,.034f,.055f,-.048f},{.235f,.037f,.052f,-.063f},
      {.075f,.035f,.046f,-.064f},{-.13f,.028f,.034f,-.052f},{-.32f,.016f,.022f,-.030f},
      {-.46f,.005f,.010f,-.010f}};
    auto vertex=[&](int j,int k){const auto& r=rings[j];float a=k*Pi/3;
      return Vec3{sway(r.z)+r.w*std::sin(a),std::cos(a)>=0?r.top*std::cos(a):-r.bottom*std::cos(a),r.z};};
    const Color colors[]={back,silver,belly,belly,silver,back};
    for(int j=0;j<5;++j)for(int k=0;k<6;++k){int n=(k+1)%6;
      panelN(vertex(j,k),vertex(j,n),vertex(j+1,n),vertex(j+1,k),colors[k],
             {sway((rings[j].z+rings[j+1].z)*.5f),0,(rings[j].z+rings[j+1].z)*.5f});}
    // Skull and jaws are separate volumes: an actual gap, not teeth on a generic snout.
    Vec3 nose{0,.029f,.492f},crown{0,.066f,.405f},chin{0,-.063f,.498f};
    const float neckX=sway(.345f);
    for(int side=-1;side<=1;side+=2){
      float x=side*.035f;
      Vec3 brow{x,.049f,.422f},upper{x*.66f,.012f,.470f},hinge{x,-.038f,.360f};
      Vec3 cheek{x*1.04f,.005f,.382f},lower{x*.62f,-.055f,.489f};
      Vec3 neckTop{neckX,.055f,.345f};
      Vec3 neckUpper{neckX+side*.0294f,.0275f,.345f};
      Vec3 neckLower{neckX+side*.0294f,-.024f,.345f};
      Vec3 neckBottom{neckX,-.048f,.345f};
      tri(nose,crown,brow,back);tri(nose,brow,upper,silver);
      panel(crown,neckTop,neckUpper,brow,back);
      panel(brow,neckUpper,neckLower,cheek,silver);
      panel(brow,cheek,hinge,upper,scale(silver,.82f));
      panel(cheek,neckLower,neckBottom,hinge,scale(silver,.9f));
      panel(hinge,neckBottom,chin,lower,scale(belly,.88f));
      tri(cheek,neckLower,hinge,scale(back,1.18f));
      // Inset dark mouth; the long lower fangs project outside the upper jaw.
      tri({side*.019f,.010f,.466f},{side*.019f,-.049f,.483f},{side*.019f,-.035f,.365f},{15,20,26});
      auto tooth=[&](Vec3 root,Vec3 tip,float width){
        Vec3 d{0,0,width};tri(root-d,root+d,tip,{209,216,203});
        if(detail)tri(root-Vec3{width,0,0},root+Vec3{width,0,0},tip,{170,185,182});};
      tooth(lower,{side*.019f,.056f,.465f},.004f);
      if(lod<2)for(int j=0;j<(detail?4:2);++j){float t=float(j)/(detail?4:2);
        float z=.455f-t*.071f,y=-.052f+t*.010f;
        tooth({side*.024f,y,z},{side*.022f,y+.060f-t*.020f,z-.012f},.0024f);
        tooth({side*.025f,.010f-t*.030f,z-.006f},{side*.021f,-.044f,z+.004f},.0022f);}
      // Large round eye, restrained metallic iris, black pupil. No glowing eyeballs.
      Vec3 eye{side*.0357f,.036f,.427f};
      for(int k=0;k<6;++k){float a=k*Pi/3,b=(k+1)*Pi/3;
        tri(eye,eye+Vec3{0,std::sin(a)*.015f,std::cos(a)*.015f},eye+Vec3{0,std::sin(b)*.015f,std::cos(b)*.015f},{157,166,153});
        Vec3 pupil=eye+Vec3{side*.001f,0,.001f};
        tri(pupil,pupil+Vec3{0,std::sin(a)*.010f,std::cos(a)*.010f},pupil+Vec3{0,std::sin(b)*.010f,std::cos(b)*.010f},{8,14,20});}
      if(lod<2){
        tri({neckX+side*.031f,-.011f,.326f},{neckX+side*.061f,-.052f,.253f},{neckX+side*.028f,-.035f,.294f},fin);
        tri({side*.019f,-.060f,.035f},{side*.042f,-.102f,-.067f},{side*.021f,-.057f,-.025f},fin);
      }
    }
    float tx=sway(-.46f),tip=sway(-.55f);
    tri({tx,.01f,-.46f},{tip,.056f,-.555f},{tip,0,-.514f},fin);
    tri({tx,-.01f,-.46f},{tip,0,-.514f},{tip,-.055f,-.555f},fin);
    tri({sway(-.31f),-.032f,-.31f},{sway(-.34f),-.062f,-.355f},{tx,-.01f,-.455f},fin);
    tri({sway(.28f),.052f,.28f},{sway(.26f),.124f,.26f},{sway(.17f),.050f,.17f},fin);
    // Long first dorsal ray: a fine curving filament, never an anglerfish stalk.
    Vec3 prev{sway(.28f),.055f,.28f};
    for(int j=1;j<=4;++j){float t=j*.25f;Vec3 next{sway(.28f)+std::sin(phase*.3f)*.008f*t,.055f+.27f*t,.28f-.15f*t*t};
      panel(prev+Vec3{0,0,.0015f},prev-Vec3{0,0,.0015f},next-Vec3{0,0,.001f},next+Vec3{0,0,.001f},fin);prev=next;}
    // Small ventrolateral photophores follow the actual body surface.
    material_=7;
    for(int side=-1;side<=1;side+=2)for(int j=0;j<(detail?12:5);++j){
      float z=.29f-j*(.68f/(detail?11:4));int r=0;while(r<4 && z<rings[r+1].z)++r;
      float t=clampf((rings[r].z-z)/(rings[r].z-rings[r+1].z),0,1);
      float w=rings[r].w*(1-t)+rings[r+1].w*t,b=rings[r].bottom*(1-t)+rings[r+1].bottom*t;
      Vec3 p{sway(z)+side*(w*.866f+.001f),b*.5f,z};float d=detail?.0026f:.003f;
      panel(p+Vec3{0,d,0},p+Vec3{0,0,d},p-Vec3{0,d,0},p-Vec3{0,0,d},{115,181,180});
    }
    material_=0;return;
  }
  if(species==Species::CombJelly) {
    // Cydippid body plan: continuous oval, eight comb rows, two branched tentacles.
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>dot(gap,gap)*.0026f;
    const int segments=close?8:3,meridians=close?16:8;
    auto shell=[&](float angle,float t,float offset=0.f){
      float radius=.19f*std::sin(t*Pi)*(1-.12f*std::cos(t*Pi))+offset;
      return Vec3{std::sin(angle)*radius,.31f-t*.43f,std::cos(angle)*radius};
    };
    // Sparse coverage reads as translucent without sorting or alpha buffers.
    material_=8;
    for(int m=0;m<meridians;++m)for(int j=0;j<segments;++j){
      float t=float(j)/segments,u=float(j+1)/segments,angle=m*2*Pi/meridians,next=(m+1)*2*Pi/meridians;
      Vec3 p=shell(angle,t),q=shell(next,t),r=shell(next,u),v=shell(angle,u);
      Color glass{112,170,180};
      if(j==0)triS(p,r,v,glass);
      else if(j==segments-1)triS(p,q,r,glass);
      else panelS(p,q,r,v,glass);
    }
    // A continuous soft row plus small paired comb plates; waves run along rows.
    static const Color spectrum[]={{101,219,249},{115,239,198},{180,213,252},{225,165,238}};
    material_=7;
    const int plates=close?8:3;
    for(int m=0;m<8;++m){
      float angle=m*Pi/4;
      for(int j=0;j<plates;++j){
        float t=.10f+j*.8f/plates,u=.10f+(j+1)*.8f/plates;
        Vec3 p=shell(angle,t,.002f),q=shell(angle,u,.002f),tangent{std::cos(angle),0,-std::sin(angle)};
        if(close)panel(p+tangent*.0025f,p-tangent*.0025f,q-tangent*.0025f,q+tangent*.0025f,{37,91,114});
        float wave=.5f+.5f*std::sin(phase*.7f-j*.85f+m*.1f);
        Vec3 c=shell(angle,(t+u)*.5f,.004f),w=tangent*(close?.011f:.014f);
        Color col=scale(spectrum[(m+j/2)%4],.48f+.5f*wave);
        panel(c-w,c+w,c+w+Vec3{0,-.009f,0},c-w+Vec3{0,-.009f,0},col);
      }
    }
    // Threads, not solid triangular tassels. Crossed ribbons stay visible side-on.
    material_=0;
    auto thread=[&](Vec3 p,Vec3 q,float width,Color col){
      Vec3 axis=unit(q-p),u=unit(cross(axis,{0,0,1}))*width,v=unit(cross(axis,u))*width;
      panel(p-u,p+u,q+u,q-u,col);panel(p-v,p+v,q+v,q-v,col);
    };
    for(int side=-1;side<=1;side+=2){
      const int joints=close?10:3;Vec3 prev{side*.155f,.025f,-.055f};
      for(int j=1;j<=joints;++j){float t=float(j)/joints;
        float sway=std::sin(phase*.22f-t*4+side)*.075f*t;
        Vec3 next{side*(.155f+.14f*t)+sway,.025f-.68f*t,-.055f-.30f*t+.025f*std::sin(t*5+phase*.16f)*t};
        thread(prev,next,.0028f*(1-t*.65f),{143,184,191});
        if(close && j>1 && j<joints){Vec3 tip=next+Vec3{side*(.075f+.03f*std::sin(t*8)),-.09f,.035f};
          Vec3 mid=(next+tip)*.5f+Vec3{0,.018f,0};thread(next,mid,.0016f,{126,170,180});thread(mid,tip,.001f,{126,170,180});}
        prev=next;
      }
    }
    material_=0;
    return;
  }
  if(species==Species::Squid) {
    // Swims tail first under jet power: pointed mantle ahead, the rhomboid fin pair on it,
    // then the head with its two big eyes, eight short arms and two long clubbed tentacles.
    uint32_t tint=hash(animal.id*2654435761u+11u);
    float shade=.86f+(tint&255)/255.0f*.28f;
    Color skin=scale(Color{168,104,112},shade),pale=scale(Color{214,186,184},.95f+shade*.05f);
    Color deepSkin=scale(Color{122,68,84},shade),armCol=scale(Color{196,150,156},shade);
    float jet=std::sin(phase*.9f),pulse=1+.07f*jet*activity;      // the mantle breathes
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>(gap.x*gap.x+gap.y*gap.y+gap.z*gap.z)*.0040f;
    int facets=close?6:4;
    struct Hoop { float z,r,squash; };
    static const Hoop mantle[5]={{.500f,.000f,1.00f},{.360f,.048f,1.00f},{.190f,.079f,1.02f},
      {.040f,.086f,1.04f},{-.060f,.078f,1.06f}};
    auto hoop=[&](const Hoop& h,Vec3* out) {
      float r=h.r*pulse;
      for(int k=0;k<facets;++k) {
        float a=k*2*Pi/facets;
        out[k]={std::sin(a)*r,std::cos(a)*r/h.squash,h.z};
      }
    };
    auto hide=[&](int k,Color top,Color bottom) {
      return blend(top,bottom,clampf(.5f-std::cos((k+.5f)*2*Pi/facets)*.62f,0,1));
    };
    Vec3 ringA[6],ringB[6];
    hoop(mantle[1],ringA);
    for(int k=0;k<facets;++k) triS({0,0,.5f},ringA[k],ringA[(k+1)%facets],hide(k,skin,pale));
    for(int m=2;m<5;++m) {
      hoop(mantle[m],ringB);
      for(int k=0;k<facets;++k) panelS(ringA[k],ringA[(k+1)%facets],ringB[(k+1)%facets],ringB[k],hide(k,skin,pale));
      for(int k=0;k<facets;++k) ringA[k]=ringB[k];
    }
    // One rhomboid fin each side, rippling from front to back along its own edge.
    for(int side=-1;side<=1;side+=2) {
      float w1=std::sin(phase*1.4f+.6f)*.030f*activity,w2=std::sin(phase*1.4f)*.044f*activity;
      tri({0,.012f,.500f},{side*.168f,.020f+w1,.322f},{side*.070f,.016f+w1*.5f,.150f},deepSkin);
      tri({0,.012f,.500f},{side*.070f,.016f+w1*.5f,.150f},{0,.030f,.170f},skin);
      tri({side*.168f,.020f+w1,.322f},{side*.112f,.010f+w2,.186f},{side*.070f,.016f+w1*.5f,.150f},deepSkin);
      tri({side*.168f,.020f+w1,.322f},{side*.070f,.016f+w1*.5f,.150f},{side*.112f,.010f+w2,.186f},scale(deepSkin,1.2f));
    }
    // Head: a short barrel behind the mantle, eyes set wide on it.
    Vec3 headRing[6];
    Hoop neck{-.135f,.062f,1.10f}; hoop(neck,headRing);
    for(int k=0;k<facets;++k) panelS(ringA[k],ringA[(k+1)%facets],headRing[(k+1)%facets],headRing[k],hide(k,deepSkin,pale));
    for(int side=-1;side<=1;side+=2) {
      tri({side*.056f,.030f,-.070f},{side*.086f,.006f,-.098f},{side*.056f,-.026f,-.070f},pale);
      tri({side*.086f,.006f,-.098f},{side*.056f,.030f,-.070f},{side*.058f,.004f,-.140f},pale);
      panel({side*.080f,.026f,-.082f},{side*.080f,.026f,-.118f},{side*.080f,-.022f,-.118f},{side*.080f,-.022f,-.082f},{20,22,30});
    }
    tri({-.030f,-.058f,-.040f},{.030f,-.058f,-.040f},{0,-.082f,-.115f},deepSkin);   // funnel
    // Eight arms in a crown, the pair of hunting tentacles reaching well beyond them.
    int arms=close?8:4;
    for(int k=0;k<arms;++k) {
      float a=k*2*Pi/arms+.39f,curl=std::sin(phase*1.1f+k*.8f)*.045f*activity;
      float cx=std::sin(a),cy=std::cos(a);
      Vec3 root{cx*.050f,cy*.044f,-.150f};
      Vec3 mid{cx*(.080f+curl),cy*(.072f+curl),-.255f};
      Vec3 tail{cx*(.062f+curl*1.8f),cy*(.056f+curl*1.8f),-.370f};
      Vec3 wide{-cy*.016f,cx*.014f,0};
      panel(root+wide,root-wide,mid-wide*.6f,mid+wide*.6f,armCol);
      tri(mid+wide*.6f,mid-wide*.6f,tail,scale(armCol,.88f));
    }
    for(int side=-1;side<=1;side+=2) {
      float curl=std::sin(phase*1.1f+.4f)*.05f*activity;
      Vec3 root{side*.040f,-.020f,-.150f},mid{side*(.068f+curl),-.030f,-.320f};
      Vec3 club{side*(.058f+curl*1.6f),-.036f,-.470f};
      Vec3 w{0,.010f,0};
      panel(root+w,root-w,mid-w*.7f,mid+w*.7f,scale(armCol,.8f));
      panel(mid+w*.7f,mid-w*.7f,club-w*.5f,club+w*.5f,scale(armCol,.8f));
      tri(club+Vec3{0,.019f,.012f},club-Vec3{0,.019f,-.012f},club+Vec3{side*.012f,0,-.042f},pale);
    }
    return;
  }
  if(species==Species::Ray) {
    // Dasyatid stingray. The disc outline is traced off a dorsal plate rather than
    // invented: for each spanwise station a (0 at the midline, 1 at the widest point)
    // the table gives where the leading and trailing margins fall. Measured disc is
    // 1.01 times as wide as it is long, widest a little past half way back, leading
    // margins nearly straight from a small projecting snout, trailing margins a broad
    // convex arc. The whip tail is as long again as the disc and more.
    Color topSkin{104,96,84},topDark{80,74,66},under{228,226,214},trim{202,152,84},
          gillLine{196,126,74},mouthLine{120,104,96};
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>(gap.x*gap.x+gap.y*gap.y+gap.z*gap.z)*.0022f;
    constexpr float HalfSpan=.2498f;
    //                            a      leading z  trailing z
    static const float DISC[7][3]={{0.00f,.5000f,-.0120f},
                                   {0.18f,.4569f,.0000f},
                                   {0.38f,.4199f,.0050f},
                                   {0.58f,.3852f,.0150f},
                                   {0.76f,.3516f,.0587f},
                                   {0.90f,.3174f,.1371f},
                                   {1.00f,.2424f,.2424f}};
    auto margin=[&](float a,float& lead,float& trail) {
      int i=1; while(i<6 && DISC[i][0]<a) ++i;
      float f=(a-DISC[i-1][0])/(DISC[i][0]-DISC[i-1][0]);
      lead=DISC[i-1][1]+(DISC[i][1]-DISC[i-1][1])*f;
      trail=DISC[i-1][2]+(DISC[i][2]-DISC[i-1][2])*f;
    };
    static const float fine[11]={-1.f,-.90f,-.76f,-.58f,-.30f,0,.30f,.58f,.76f,.90f,1.f};
    static const float coarse[11]={-1.f,-.76f,-.38f,0,.38f,.76f,1.f,0,0,0,0};
    const float* span=close?fine:coarse;
    int stations=close?11:7;
    // Top and underside are sampled at different points along the chord: the back
    // needs its crown near the middle, the belly needs a narrow orange rim and a
    // broad white field between, which is how the drawing of the ventral side reads.
    static const float topChord[3]={.20f,.50f,1.f};
    static const float lowChord[3]={.12f,.80f,1.f};
    static const float topAt[3]={.92f,1.00f,.10f};
    static const float lowAt[3]={.40f,.48f,.06f};
    auto rib=[&](float u,Vec3* out) {
      float a=std::abs(u),lead,trail;
      margin(a,lead,trail);
      float chord=lead-trail;
      // A disc this size is about eight parts in a hundred of its own length thick.
      float thick=.0373f*std::pow(std::max(0.f,1-a*.94f),1.4f);
      float lift=std::sin(phase*.55f-a*2.2f)*.070f*std::pow(a,1.7f)*activity;
      float x=u*HalfSpan;
      out[0]={x,lift,lead};
      out[1]={x,lift+thick*topAt[0],lead-chord*topChord[0]};
      out[2]={x,lift+thick*topAt[1],lead-chord*topChord[1]};
      out[3]={x,lift,trail};
      out[4]={x,lift-thick*lowAt[1],lead-chord*lowChord[1]};
      out[5]={x,lift-thick*lowAt[0],lead-chord*lowChord[0]};
    };
    Vec3 a6[6],b6[6];
    rib(span[0],a6);
    for(int i=1;i<stations;++i) {
      rib(span[i],b6);
      for(int k=0;k<6;++k) {
        Color c=k==0?topSkin:k<3?(k==1?topSkin:topDark):k==3?trim:k==5?trim:under;
        panel(a6[k],a6[(k+1)%6],b6[(k+1)%6],b6[k],c);
      }
      for(int k=0;k<6;++k) a6[k]=b6[k];
    }
    // Pelvic fins: rounded lobes standing either side of the tail root, which is how
    // they read from below - two paddles with the tail coming out between them.
    for(int side=-1;side<=1;side+=2) {
      // Widest where it attaches under the back of the disc, outer margin running
      // straight back and then cut in, tip swinging towards the tail: the pair close
      // on it rather than opening away.
      static const float LOBE[6][2]={{.008f,.0500f},{.052f,.0460f},{.050f,.0060f},
                                     {.036f,-.0320f},{.020f,-.0580f},{.006f,-.0180f}};
      const Vec3 hub{side*.026f,0,.0000f};
      for(int i=0;i<5;++i) {
        Vec3 a{side*LOBE[i][0],0,LOBE[i][1]},b{side*LOBE[i+1][0],0,LOBE[i+1][1]};
        tri({hub.x,-.0030f,hub.z},{a.x,-.0040f,a.z},{b.x,-.0040f,b.z},topDark);
        tri({hub.x,-.0125f,hub.z},{b.x,-.0115f,b.z},{a.x,-.0115f,a.z},blend(under,trim,.40f));
        panel({a.x,-.0040f,a.z},{b.x,-.0040f,b.z},{b.x,-.0115f,b.z},{a.x,-.0115f,a.z},
              blend(topDark,trim,.30f));
      }
    }
    for(int side=-1;side<=1;side+=2) {
      // Eyes a quarter of the way back, spiracles right behind and larger.
      tri({side*.030f,.037f,.3740f},{side*.057f,.032f,.3590f},{side*.032f,.022f,.3530f},{26,28,26});
      tri({side*.037f,.032f,.3220f},{side*.072f,.027f,.3070f},{side*.040f,.017f,.3000f},{40,36,32});
    }
    if(close) {
      // The face is all on the underside: a pair of nostrils, the transverse mouth
      // behind them, then five pairs of gill slits sweeping out and back from it.
      float belly=-.0150f;
      panel({-.046f,belly,.3520f},{.046f,belly,.3520f},
            {.040f,belly,.3410f},{-.040f,belly,.3410f},mouthLine);
      for(int side=-1;side<=1;side+=2) {
        tri({side*.014f,belly,.3800f},{side*.026f,belly,.3760f},{side*.016f,belly,.3690f},mouthLine);
        for(int i=0;i<5;++i) {
          float t=i*.25f;
          float z=.3180f-t*.1050f,x=.0420f+t*.0400f;
          tri({side*x,belly,z},{side*(x+.0170f),belly,z-.0090f},{side*(x+.0040f),belly,z-.0230f},gillLine);
        }
      }
    }
    {
      // The tail: longer than the disc, thick where it leaves the body, then a whip.
      // The sting lies back against it a quarter of the way along.
      int steps=close?7:4;
      Vec3 prev{0,0,-.0080f};
      float prevR=.0125f;
      for(int j=1;j<=steps;++j) {
        float t=float(j)/steps;
        float wag=std::sin(phase*.5f-t*2.4f)*.085f*t*t*activity;
        float r=.0125f*(1-t*.80f)+.0018f;
        Vec3 next{wag,-.010f*t,-.0080f-t*.4920f};
        for(int k=0;k<4;++k) {
          float a0=k*Pi*.5f,a1=(k+1)*Pi*.5f;
          panelN(prev+Vec3{std::sin(a0)*prevR,std::cos(a0)*prevR,0},
                 prev+Vec3{std::sin(a1)*prevR,std::cos(a1)*prevR,0},
                 next+Vec3{std::sin(a1)*r,std::cos(a1)*r,0},
                 next+Vec3{std::sin(a0)*r,std::cos(a0)*r,0},
                 (k==1||k==2)?trim:topDark,(prev+next)*.5f);
        }
        if(close && j*3==steps) {
          tri(next+Vec3{0,r,0},next+Vec3{.004f,r*.4f,0},next+Vec3{0,r+.007f,-.070f},{232,228,212});
          tri(next+Vec3{0,r,0},next+Vec3{0,r+.007f,-.070f},next+Vec3{-.004f,r*.4f,0},{214,208,190});
        }
        if(j==steps) {                                 // close the very tip
          for(int k=0;k<4;++k) {
            float a0=k*Pi*.5f,a1=(k+1)*Pi*.5f;
            triN(next+Vec3{std::sin(a0)*r,std::cos(a0)*r,0},
                 next+Vec3{std::sin(a1)*r,std::cos(a1)*r,0},
                 next+Vec3{0,0,-.014f},topDark,next+Vec3{0,0,.020f});
          }
        }
        prev=next;prevR=r;
      }
    }
    return;
  }
  if(species==Species::Flatfish) {
    // Lies on one side on the sand: an oval plate with both eyes moved onto the upper
    // face, and the dorsal and anal fins run right around the rim as one continuous
    // fringe. That fringe is what tells a flatfish from a ray.
    Color topSkin{150,138,100},mottle{119,112,84},under{221,214,186},fringe{176,166,128};
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>(gap.x*gap.x+gap.y*gap.y+gap.z*gap.z)*.0016f;
    int rimCount=close?14:8;
    auto rim=[&](int k) {
      float a=k*2*Pi/rimCount;
      float z=std::cos(a)*.330f,x=std::sin(a)*.215f;
      float ripple=std::sin(phase*.7f+z*5.0f)*.016f*activity;   // the fringe shivers
      return Vec3{x,ripple,z};
    };
    for(int k=0;k<rimCount;++k) {
      Vec3 p=rim(k),q=rim((k+1)%rimCount);
      Vec3 pi{p.x*.70f,p.y+.030f,p.z*.72f},qi{q.x*.70f,q.y+.030f,q.z*.72f};
      tri({0,.042f,0},pi,qi,k%3?topSkin:mottle);
      panel(pi,qi,q,p,scale(topSkin,.88f));
      tri({0,-.004f,0},q,p,under);
      if(close) tri(p,q,{(p.x+q.x)*.62f,(p.y+q.y)*.5f,(p.z+q.z)*.62f-(p.z+q.z)*.04f},fringe);
    }
    for(int k=0;k<2;++k)                                          // both eyes on the up side
      tri({.028f+k*.052f,.058f,.150f},{.052f+k*.052f,.048f,.118f},{.028f+k*.052f,.046f,.112f},{32,34,30});
    tri({-.030f,.010f,-.320f},{.030f,.010f,-.320f},{std::sin(phase*.6f)*.040f,.010f,-.470f},fringe);
    return;
  }
  if(species==Species::Crab) {
    // Half again as wide as it is long, carrying a domed shield with a flat underside.
    // The eight walking legs fan from forward-lateral round to backward-lateral, each
    // with its knee held above the shell and its point turned down, which is the stance
    // that makes a crab read as a crab and not as a spider. Two sets step alternately.
    Color shell{164,104,78},shellLit{196,140,106},under{212,186,152},limb{178,130,92},joint{140,92,68};
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>(gap.x*gap.x+gap.y*gap.y+gap.z*gap.z)*.0016f;
    int steps=close?12:7;
    auto rim=[&](float t) {
      float a=t*2*Pi;
      return Vec3{std::sin(a)*.530f,.060f,.340f*std::cos(a)-(std::cos(a)<0?.040f:0)};
    };
    for(int k=0;k<steps;++k) {
      float t0=float(k)/steps,t1=float(k+1)/steps;
      Vec3 r0=rim(t0),r1=rim(t1);
      Vec3 m0{r0.x*.60f,.175f,r0.z*.60f},m1{r1.x*.60f,.175f,r1.z*.60f};
      tri({0,.225f,-.020f},m0,m1,k%2?shell:shellLit);      // the dome
      panel(m0,m1,r1,r0,k%3?shellLit:shell);               // the shoulder of the shield
      tri({0,-.012f,-.020f},r1,r0,under);                  // flat underside
    }
    if(close) for(int k=0;k<6;++k) {                       // teeth along the front margin
      float x=(k-2.5f)*.106f,z=.340f*std::sqrt(std::max(0.0f,1-(x/.530f)*(x/.530f)));
      tri({x-.044f,.060f,z},{x+.044f,.060f,z},{x,.044f,z+.058f},shellLit);
    }
    for(int side=-1;side<=1;side+=2) {                     // eyes on short stalks, forward
      Vec3 root{side*.090f,.090f,.250f},ball{side*.118f,.152f,.300f};
      panel(root+Vec3{.020f,0,0},root-Vec3{.020f,0,0},ball-Vec3{.016f,0,0},ball+Vec3{.016f,0,0},limb);
      for(int q=0;q<4;++q) {
        float a0=q*Pi*.5f,a1=(q+1)*Pi*.5f;
        tri(ball+Vec3{0,0,.030f},ball+Vec3{std::sin(a0)*.030f,std::cos(a0)*.030f,0},
                                 ball+Vec3{std::sin(a1)*.030f,std::cos(a1)*.030f,0},{30,26,24});
      }
    }
    for(int side=-1;side<=1;side+=2) for(int j=0;j<(close?4:3);++j) {
      // Legs 1 and 3 lift while 2 and 4 stay planted, and the two sides are half a beat
      // apart, so it always has a tripod on the ground.
      float phi=(.26f-j*(close?.50f:.66f));                // fan angle, sideways to back
      float cx=std::cos(phi),cz=std::sin(phi);
      float step=std::sin(phase*.9f+(j%2)*Pi+(side>0?0:Pi))*activity;
      float lift=std::max(0.0f,step)*.075f,reach=step*.070f;
      Vec3 root{side*.450f*cx,.064f,.290f*cz};
      Vec3 dir{side*cx,0,cz};
      Vec3 knee=root+dir*.150f+Vec3{0,.072f+lift,0}+dir*reach*.4f;
      Vec3 ankle=root+dir*.300f+Vec3{0,-.010f+lift*.5f,0}+dir*reach;
      Vec3 foot=root+dir*.335f+Vec3{0,-.165f,0}+dir*reach*.5f;
      auto seg=[&](Vec3 a2,Vec3 b2,float w0,float w1,Color c) {
        panel(a2+Vec3{0,w0,0},a2-Vec3{0,w0,0},b2-Vec3{0,w1,0},b2+Vec3{0,w1,0},c);
        if(close) panel(a2+Vec3{-cz*w0*side,0,cx*w0},a2+Vec3{cz*w0*side,0,-cx*w0},
                        b2+Vec3{cz*w1*side,0,-cx*w1},b2+Vec3{-cz*w1*side,0,cx*w1},scale(c,.84f));
      };
      seg(root,knee,.105f,.082f,limb);
      seg(knee,ankle,.078f,.046f,joint);
      tri(ankle+Vec3{0,.046f,0},ankle-Vec3{0,.046f,0},foot,limb);
    }
    for(int side=-1;side<=1;side+=2) {                     // chelipeds, folded up in front
      float wave=std::sin(phase*.5f+side)*.045f*activity;
      Vec3 root{side*.290f,.070f,.175f};
      Vec3 elbow{side*.415f,.140f+wave,.285f};
      Vec3 wrist{side*.285f,.055f+wave,.385f};
      panel(root+Vec3{0,.066f,0},root-Vec3{0,.066f,0},elbow-Vec3{0,.058f,0},elbow+Vec3{0,.058f,0},limb);
      panel(elbow+Vec3{0,.058f,0},elbow-Vec3{0,.058f,0},wrist-Vec3{0,.054f,0},wrist+Vec3{0,.054f,0},limb);
      if(close) {
        panel(root+Vec3{.060f,0,0},root-Vec3{.060f,0,0},elbow-Vec3{.052f,0,0},elbow+Vec3{.052f,0,0},scale(limb,.84f));
        panel(elbow+Vec3{.052f,0,0},elbow-Vec3{.052f,0,0},wrist-Vec3{.048f,0,0},wrist+Vec3{.048f,0,0},scale(limb,.84f));
      }
      // The claw: a deep palm with a fixed lower finger and a hinged upper one.
      Vec3 palm{side*.255f,.045f+wave,.470f};
      for(int q=0;q<4;++q) {
        float a0=q*Pi*.5f+.78f,a1=(q+1)*Pi*.5f+.78f;
        Vec3 p0=palm+Vec3{std::sin(a0)*.072f*side,std::cos(a0)*.112f,0};
        Vec3 p1=palm+Vec3{std::sin(a1)*.072f*side,std::cos(a1)*.112f,0};
        tri(wrist,p1,p0,shellLit);
        tri(palm+Vec3{0,.010f,.105f},p0,p1,q<2?shellLit:shell);
      }
      float open=.030f+.030f*std::sin(phase*.7f);
      tri(palm+Vec3{side*.044f,-.072f,.090f},palm+Vec3{side*-.030f,-.066f,.090f},
          palm+Vec3{side*.006f,-.052f,.235f},under);                       // fixed finger
      tri(palm+Vec3{side*.044f,open,.090f},palm+Vec3{side*-.030f,open+.014f,.090f},
          palm+Vec3{side*.006f,open-.008f,.225f},under);                   // hinged finger
    }
    return;
  }
  if(species==Species::Eel) {
    // Moray. Strongly compressed side to side: about one and a half times as deep
    // as wide at the head and well over twice as deep at the tail. The head is a deep
    // wedge whose gape runs back past the eye, the nape stands up behind it, and one
    // low fleshy ridge runs from there along the back, round the tip and forward
    // underneath. No paired fins at all. The skin is blotched, not plain.
    Color base{72,84,60},mottle{156,164,116},belly{150,152,116},maw{178,140,130};
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>(gap.x*gap.x+gap.y*gap.y+gap.z*gap.z)*.0042f;
    int joints=close?12:7,facets=close?6:4;
    // More than one wavelength on the body, which is how an eel of this length swims.
    auto bend=[&](float t) { return std::sin(phase*.85f-t*8.2f)*.075f*(.22f+t)*activity; };
    // Depth measured off the side view: thin snout, deepest at the nape, long taper.
    // Half depth as a fraction of total length, read off the plate station by station.
    // A moray is far slimmer than the old mesh had it: the body is under nine parts in
    // a hundred of its own length deep, and the fin ridge adds about a quarter again.
    auto depthAt=[&](float t) {
      if(t<.04f) return .0020f+t*.5000f;
      if(t<.16f) return .0220f+(t-.04f)*.1833f;
      if(t<.55f) return .0440f-(t-.16f)*.0100f;   // a moray holds its depth a long way
      float u=(t-.55f)/.45f;
      return .0401f*(1-u*u*.55f-u*.43f);
    };
    auto ringAt=[&](float t,Vec3* out) {
      float z=.50f-t,x=bend(t);
      float d=std::max(.003f,depthAt(t));
      float w=d/(1.55f+t*.95f);                   // compressed, and more so towards the tail
      float top=1.16f-t*.26f;                     // the back carries the nape hump
      for(int k=0;k<facets;++k) {
        float a=k*2*Pi/facets;
        float c=std::cos(a);
        out[k]={x+std::sin(a)*w,c*d*(c>0?top:2.f-top),z};
      }
    };
    Vec3 a8[12],b8[12];                          // must hold facets, which is 10 near
    Vec3 snout{bend(0),.004f,.500f};
    float ta=.030f;
    ringAt(ta,a8);
    for(int k=0;k<facets;++k)
      triN(snout,a8[k],a8[(k+1)%facets],scale(base,1.05f),
           Vec3{bend(ta*.5f),0,.50f-ta*.5f});
    for(int j=1;j<=joints;++j) {
      float t=float(j)/joints;
      ringAt(t,b8);
      Vec3 axis=(Vec3{bend(ta),0,.50f-ta}+Vec3{bend(t),0,.50f-t})*.5f;
      for(int k=0;k<facets;++k) {
        int n=(k+1)%facets;
        float c=std::cos((k+.5f)*2*Pi/facets);
        Color skin=blend(base,belly,clampf(.5f-c*.85f,0,1));
        // A cheap blotch: every third patch along each row goes pale, so the animal
        // is marbled the way a moray is instead of one flat green.
        if(((j*7+k*5)%11)<4) skin=blend(skin,mottle,.55f);
        panelN(a8[k],a8[n],b8[n],b8[k],skin,axis);
      }
      for(int k=0;k<facets;++k) a8[k]=b8[k];
      ta=t;
    }
    { Vec3 tip{bend(1.f),0,-.500f};                 // close the tail so it is not a pipe
      for(int k=0;k<facets;++k)
        triN(tip,a8[(k+1)%facets],a8[k],scale(base,.9f),Vec3{bend(.96f),0,-.460f}); }
    {
      // One continuous ridge: along the back from the nape, round the tip, then
      // forward along the underside as far as the vent.
      // Sampled at exactly the same stations as the body rings: at this wavelength a
      // second, coarser polyline drifts away from the flank and reads as a loose plate.
      for(int j=2;j<=joints;++j) {
        float t0=float(j-1)/joints,t1=float(j)/joints;
        if(t0<.14f) continue;
        auto crest=[&](float t) { return std::max(.004f,depthAt(t)*.30f); };
        float d0=depthAt(t0)*(1.16f-t0*.26f),d1=depthAt(t1)*(1.16f-t1*.26f);
        panel({bend(t0),d0,.50f-t0},{bend(t1),d1,.50f-t1},
              {bend(t1),d1+crest(t1),.50f-t1},{bend(t0),d0+crest(t0),.50f-t0},scale(base,1.18f));
        if(t0<.45f) continue;
        float u0=depthAt(t0)*(.84f+t0*.26f),u1=depthAt(t1)*(.84f+t1*.26f);
        float v0=std::max(.003f,depthAt(t0)*.24f),v1=std::max(.003f,depthAt(t1)*.24f);
        panel({bend(t0),-u0-v0,.50f-t0},{bend(t1),-u1-v1,.50f-t1},
              {bend(t1),-u1,.50f-t1},{bend(t0),-u0,.50f-t0},scale(belly,.92f));
      }
    }
    {
      // The mouth: a long gape that reaches back past the eye, cut into the side of
      // the head rather than built as separate plates - the jaw is part of the same
      // tube, so nothing can come adrift from it.
      float gape=.010f+.006f*std::sin(phase*.4f);
      const float cz=.427f;
      float hd=.0300f,hw=hd/1.58f;                      // head at the jaw corner
      float x0=bend(0);
      for(int side=-1;side<=1;side+=2) {
        Vec3 tip{x0,.002f,.500f};
        Vec3 corner{side*hw*1.02f,-.004f,cz};
        Vec3 drop{x0,-.006f-gape,.492f};
        tri(tip,corner,drop,maw);                       // the dark line of the gape
        tri(tip,{side*hw*.70f,.012f,.470f},corner,scale(base,.92f));
        if(close) for(int k=0;k<4;++k) {                // the teeth that show through
          float f=.22f+k*.20f;
          Vec3 u=mix(tip,corner,f);
          float len=.013f*(1-f*.45f);
          tri({u.x-side*.004f,u.y+.002f,u.z},{u.x+side*.004f,u.y+.002f,u.z},
              {u.x,u.y-len,u.z-.003f},{228,224,206});
        }
      }
      for(int side=-1;side<=1;side+=2) {
        // Small round eye over the middle of the gape, gill hole well behind it.
        tri({side*hw*.86f,.020f,.458f},{side*hw*1.06f,.013f,.448f},{side*hw*.88f,.006f,.452f},{20,22,18});
        if(close) tri({side*.022f,-.003f,.372f},{side*.028f,-.012f,.362f},{side*.020f,-.016f,.369f},{28,32,26});
        if(close) tri({side*.006f,.012f,.498f},{side*.010f,.008f,.510f},{side*.004f,.003f,.498f},scale(base,1.1f));
      }
    }
    return;
  }
  if(species==Species::Lanternfish) {
    // Myctophum punctatum. The outline is not estimated: it is the silhouette traced
    // off a specimen plate, opened to separate the body from the fins, and written
    // down as fractions of total length. Station t runs from the snout at t=0 to the
    // tail tip at t=1, cy is where the body's own axis sits (the whole fish rides low
    // because the dorsal fin is tall), hd is half depth, hw half width.
    // Off the plate as well: a dark olive back over the top forty degrees only, and
    // below that a bright silver flank running the whole way to the belly. That flank
    // is the one thing that makes the fish show up when the lamp catches it.
    Color back{48,46,38},flank{202,210,216},bellyCol{190,202,212},fin{168,158,132};
    Vec3 gap=animal.position-viewer_;
    float far2=gap.x*gap.x+gap.y*gap.y+gap.z*gap.z;
    // A shoal of these runs to a couple of hundred, so the detailed mesh is kept for
    // the handful genuinely near the boat.
    bool close=!crowded() && size*size>far2*.0155f;
    bool mid=!jammed() && size*size>far2*.0034f;
    // Even facet counts only: with an odd one there is a vertex on the back but none
    // on the belly, and the underside comes out a fifth too shallow.
    int facets=close?6:4;
    //                              t      hd      hw      cy
    // Half depth peaks at 0.0925 just behind the head and clear of every fin, so the
    // body is 0.185 of total length at its deepest. The snout is blunt, not a point.
    static const float STN[10][4]={{.000f,.0170f,.0130f,-.049f},
                                   {.045f,.0600f,.0300f,-.044f},
                                   {.100f,.0830f,.0395f,-.038f},
                                   {.170f,.0925f,.0425f,-.034f},
                                   {.280f,.0925f,.0415f,-.031f},
                                   {.400f,.0905f,.0395f,-.032f},
                                   {.530f,.0850f,.0355f,-.037f},
                                   {.660f,.0725f,.0290f,-.041f},
                                   {.780f,.0355f,.0145f,-.030f},
                                   {.880f,.0165f,.0072f,-.024f}};
    const int step=close?1:mid?2:3;
    auto bend=[&](float t) { return std::sin(phase*1.15f-t*3.1f)*.026f*(.10f+t*t)*activity; };
    auto ringAt=[&](int i,Vec3* out) {
      float t=STN[i][0],d=STN[i][1],w=STN[i][2],cy=STN[i][3];
      for(int k=0;k<facets;++k) {
        float a=k*2*Pi/facets;
        out[k]={bend(t)+std::sin(a)*w,cy+std::cos(a)*d,.50f-t};
      }
    };
    auto axisAt=[&](int i,int j) {
      return Vec3{(bend(STN[i][0])+bend(STN[j][0]))*.5f,(STN[i][3]+STN[j][3])*.5f,
                  .50f-(STN[i][0]+STN[j][0])*.5f};
    };
    Vec3 a6[6],b6[6];
    ringAt(0,a6);
    int prev=0;
    for(int i=step;i<10;i+=step) {
      ringAt(i,b6);
      Vec3 axis=axisAt(prev,i);
      for(int k=0;k<facets;++k) {
        int n=(k+1)%facets;
        float c=std::cos((k+.5f)*2*Pi/facets);
        panelN(a6[k],a6[n],b6[n],b6[k],blend(bellyCol,back,clampf(c*1.60f-.25f,0,1)),axis);
      }
      for(int k=0;k<facets;++k) a6[k]=b6[k];
      prev=i;
    }
    {
      // Caudal: deeply forked, not the shallow notch this had. On the plate the
      // fork cuts 0.095 of the whole length back in from the tips and the lobes
      // spread 0.149, near enough symmetrical about the body's own axis.
      float wag=std::sin(phase*1.15f-3.1f)*.050f*activity;
      Vec3 ru{bend(.80f),-.012f,-.300f},rl{bend(.80f),-.042f,-.300f};
      Vec3 uc{wag*.7f,+.040f,-.400f},ut{wag,+.0645f,-.500f};
      Vec3 lc{wag*.7f,-.088f,-.400f},lt{wag,-.1125f,-.500f};
      Vec3 notch{wag,-.024f,-.402f};
      tri(ru,uc,ut,fin); tri(ru,ut,notch,fin);
      tri(rl,lc,lt,fin); tri(rl,lt,notch,fin);
      tri(ru,notch,rl,fin);
    }
    {
      // Dorsal fin: base 0.32 to 0.47 of length and 0.113 tall at its rear corner,
      // both further forward and half again taller than they were. Anal fin: a long
      // low one from 0.51 to 0.67. Adipose at 0.82.
      tri({bend(.32f),.0610f,.180f},{bend(.47f),.0577f,.030f},{bend(.47f),.1710f,.035f},fin);
      tri({bend(.507f),-.1230f,-.007f},{bend(.670f),-.1130f,-.170f},{bend(.56f),-.1770f,-.055f},fin);
      // The adipose sits on the back, not inside it, which is where it was.
      if(mid) tri({bend(.81f),.0020f,-.310f},{bend(.86f),-.0020f,-.360f},{bend(.835f),.0330f,-.335f},fin);
      for(int side=-1;side<=1;side+=2) {
        tri({side*.036f,-.070f,.330f},{side*.044f,-.120f,.200f},{side*.030f,-.040f,.250f},fin);
        if(mid) tri({side*.024f,-.112f,.180f},{side*.028f,-.150f,.120f},{side*.020f,-.096f,.140f},fin);
      }
    }
    {
      // Head. The eye is 0.088 of the whole animal across - about a third of the head
      // - and the mouth opens obliquely back past it, which is the give-away.
      Vec3 tip{bend(0),-.049f,.500f};
      float gape=.010f+.007f*std::sin(phase*.5f);
      for(int side=-1;side<=1;side+=2) {
        tri(tip,{side*.030f,-.072f,.392f},{side*.026f,-.014f,.412f},scale(flank,.8f));
        tri(tip,{bend(.02f),-.068f-gape,.462f},{side*.030f,-.072f,.392f},{38,30,28});
      }
      // The eye takes up most of the head and on the plate it is a wide pale ring
      // round a black pupil, which is the brightest thing on the fish after the
      // photophores. Drawn as one dark disc, as it was, it disappeared into the head.
      for(int side=-1;side<=1;side+=2) {
        const int n=close?6:mid?4:3;
        Vec3 c{side*.040f,-.024f,.428f};
        auto disc=[&](float r,float out,Color col) {
          Vec3 o=c+Vec3{side*out,0,0};
          for(int k=0;k<n;++k) {
            float a0=k*2*Pi/n,a1=(k+1)*2*Pi/n;
            tri(o,o+Vec3{0,std::cos(a1)*r,std::sin(a1)*r*.92f},
                  o+Vec3{0,std::cos(a0)*r,std::sin(a0)*r*.92f},col);
          }
        };
        disc(.044f,0,{186,192,168});
        disc(.034f,.004f,{14,14,18});
        tri(c+Vec3{side*.008f,.012f,.010f},c+Vec3{side*.008f,-.004f,.016f},
            c+Vec3{side*.008f,.002f,-.002f},{206,220,226});
      }
    }
    if(close) {
      // Photophores along the belly and the lower flank: the only light on the fish.
      material_=7;
      Color lamp{132,224,208};
      for(int j=0;j<10;++j) {
        float t=.18f+j*.068f;
        int i=1; while(i<9 && STN[i][0]<t) ++i;
        float f=(t-STN[i-1][0])/(STN[i][0]-STN[i-1][0]);
        float d=STN[i-1][1]+(STN[i][1]-STN[i-1][1])*f;
        float cy=STN[i-1][3]+(STN[i][3]-STN[i-1][3])*f;
        float w=STN[i-1][2]+(STN[i][2]-STN[i-1][2])*f;
        float z=.50f-t,x=bend(t);
        tri({x-.005f,cy-d*.98f,z-.004f},{x+.005f,cy-d*.98f,z-.004f},{x,cy-d*.98f,z+.007f},lamp);
        if(j%2==0) for(int side=-1;side<=1;side+=2)
          tri({x+side*w*.94f,cy-d*.50f,z-.004f},{x+side*w*.94f,cy-d*.50f,z+.004f},
              {x+side*w*.94f,cy-d*.30f,z},lamp);
      }
      material_=0;
    }
    return;
  }
  if(species==Species::GardenEel) {
    // Stands out of its burrow like a question mark, head at the top facing into the
    // current, and drops straight back down into the sand when something comes close.
    Color pale{203,193,158},dark{96,102,84};
    float out=.62f+.38f*std::sin(phase*.15f);        // how far it dares to come out
    const float extension=clampf(activity,0,1);
    if(extension<=.001f)return;
    const float sink=(out*.78f+.043f)*(1-extension);
    // Slide the unchanged body below its burrow rim and clip there, rather than
    // flattening it or moving the whole colony sideways when the camera approaches.
    auto buriedTri=[&](Vec3 a,Vec3 b,Vec3 c,Color col) {
      Vec3 input[3]={a,b,c},output[4];int count=0;
      for(auto& p:input)p.y-=sink;
      for(int k=0;k<3;++k){Vec3 p=input[k],q=input[(k+1)%3];
        if(p.y>=0)output[count++]=p;
        if((p.y>=0)!=(q.y>=0))output[count++]=p+(q-p)*(p.y/(p.y-q.y));
      }
      for(int k=1;k+1<count;++k)tri(output[0],output[k],output[k+1],col);
    };
    auto buriedPanel=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color col){buriedTri(a,b,c,col);buriedTri(a,c,d,col);};
    int joints=9;
    Vec3 prev{0,0,0};
    float prevR=.026f;
    for(int j=1;j<=joints;++j) {
      float t=float(j)/joints;
      float y=t*out*.78f;
      float lean=std::sin(phase*.30f+t*1.9f)*.090f*t*t;
      // Straight out of the burrow, then a long forward hook, and the head cocks back
      // into the current: the shape everyone recognises a garden eel by.
      float curve=(t<.40f?0:(t-.40f)*(t-.40f)*1.25f)-(t<.80f?0:(t-.80f)*(t-.80f)*2.4f);
      Vec3 next{lean,y,curve};
      float r=.026f*(1-t*.18f)+(t>.78f?(t-.78f)*.075f:0.0f);
      for(int k=0;k<4;++k) {
        float a=k*Pi*.5f,b=(k+1)*Pi*.5f;
        Vec3 p0=prev+Vec3{std::sin(a)*prevR,0,std::cos(a)*prevR};
        Vec3 p1=prev+Vec3{std::sin(b)*prevR,0,std::cos(b)*prevR};
        Vec3 q0=next+Vec3{std::sin(a)*r,0,std::cos(a)*r};
        Vec3 q1=next+Vec3{std::sin(b)*r,0,std::cos(b)*r};
        buriedPanel(p0,p1,q1,q0,(j+k)%3?pale:dark);        // the spotted pattern
      }
      prev=next; prevR=r;
    }
    Vec3 head=prev+Vec3{0,.042f,.052f};
    for(int k=0;k<4;++k) {                            // blunt head, big eye, small mouth
      float a=k*Pi*.5f,b=(k+1)*Pi*.5f;
      buriedTri(head,prev+Vec3{std::sin(a)*prevR,.014f,std::cos(a)*prevR},
               prev+Vec3{std::sin(b)*prevR,.014f,std::cos(b)*prevR},pale);
    }
    for(int side=-1;side<=1;side+=2)                  // the oversized eye
      buriedTri(head+Vec3{side*.020f,-.006f,.006f},head+Vec3{side*.036f,-.024f,-.008f},
          head+Vec3{side*.018f,-.030f,.004f},{28,30,28});
    buriedTri(head+Vec3{-.014f,-.028f,.026f},head+Vec3{.014f,-.028f,.026f},head+Vec3{0,-.040f,.040f},dark);
    return;
  }
  if(species==Species::Sunfish) { sunfishMesh(animal); return; }
  if(species==Species::Anglerfish) {
    // Melanocetus johnsonii, measured off a lateral engraving and two preserved
    // specimens. The numbers that decide whether it reads: the body is 0.66 of its
    // standard length deep and deepest two fifths of the way back, the gape runs
    // 0.39 of that length back from the snout, the eye is 0.04 of it across, the
    // longest teeth 0.11, the illicium a quarter. It is a mouth with a lamp hung in
    // front of it; the fins are small and crowded at the tail.
    // The face is the whole animal, and in the photographs it reads by four things:
    // a comb of pale teeth the length of both jaws, a bony margin on each jaw, a
    // small bright eye and a mouth lining that is brown rather than black. Left
    // black on black it is a wedge with a lamp over it and nothing else.
    const Color back{62,55,51},bellyCol{82,73,67},rayCol{88,100,118},web{124,138,156},
                toothCol{236,233,222},lip{88,76,62},mawRoof{48,36,32},mawFloor{27,20,18};
    Vec3 gap=animal.position-viewer_;
    float far2=gap.x*gap.x+gap.y*gap.y+gap.z*gap.z;
    bool close=!crowded() && size*size>far2*.0051f;
    bool mid=!jammed() && size*size>far2*.0011f;
    //                            z      dorsal  ventral halfwidth
    // Chosen so that the slope from one station to the next changes gradually. It is
    // not enough for the outline to fall steadily: a run that descends gently and then
    // steeply is still a hollow, and the corner ring was exactly that at both ends,
    // 0.19 against 0.62 above and 0.65 against 1.00 below. Those two kinks were the
    // dents, and they are in the body, not in the head or the jaw.
    //                             z    dorsal ventral halfwidth   slope up  down
    static const float STN[9][4]={{ .215f,.184f,-.232f,.156f},   //   .51    .73
                                  { .160f,.212f,-.272f,.180f},   //   .43    .67
                                  { .100f,.238f,-.312f,.202f},   //   .17    .26
                                  { .030f,.250f,-.330f,.214f},   //  -.13   -.10
                                  {-.050f,.240f,-.322f,.206f},   //  -.43   -.66
                                  {-.120f,.210f,-.276f,.172f},   //  -.70  -1.23
                                  {-.180f,.168f,-.202f,.126f},   // -1.20  -1.82
                                  {-.225f,.114f,-.120f,.074f},   // -1.57  -1.89
                                  {-.262f,.056f,-.050f,.032f}};
    // It hangs in the water rather than swimming: the tail end stirs, the head does
    // not move at all, which is why the sway is weighted to the back of the body.
    auto bend=[&](float z) {
      float t=clampf((.215f-z)*2.096f,0,1);
      return std::sin(phase*.50f-t*1.5f)*.030f*t*t*activity;
    };
    // Even, and divisible by two, because the head is lofted off the same ring: see
    // jawSteps below.
    const int facets=close?10:6;
    auto ringAt=[&](int i,Vec3* out) {
      float z=STN[i][0],cy=(STN[i][1]+STN[i][2])*.5f,hd=(STN[i][1]-STN[i][2])*.5f,hw=STN[i][3];
      for(int k=0;k<facets;++k) {
        float a=k*2*Pi/facets;
        out[k]={bend(z)+std::sin(a)*hw,cy+std::cos(a)*hd,z};
      }
    };
    auto axisAt=[&](int i,int j) {
      return Vec3{(bend(STN[i][0])+bend(STN[j][0]))*.5f,
                  (STN[i][1]+STN[i][2]+STN[j][1]+STN[j][2])*.25f,
                  (STN[i][0]+STN[j][0])*.5f};
    };
    Vec3 a8[8],b8[8];
    ringAt(0,a8);
    int prev=0;
    auto joinTo=[&](int i) {
      ringAt(i,b8);
      Vec3 axis=axisAt(prev,i);
      for(int k=0;k<facets;++k) {
        int n=(k+1)%facets;
        float c=std::cos((k+.5f)*2*Pi/facets);
        panelN(a8[k],a8[n],b8[n],b8[k],blend(bellyCol,back,clampf(.5f+c*.9f,0,1)),axis);
      }
      for(int k=0;k<facets;++k) a8[k]=b8[k];
      prev=i;
    };
    const int step=close?1:mid?2:3;
    for(int i=step;i<9;i+=step) joinTo(i);
    if(prev!=8) joinTo(8);
    // The jaws. Both arches are drawn across the same parameter, -1 at the left
    // corner to +1 at the front, so a tooth, the lip and the skin behind it all land
    // on the same station. Both jaws turn about the line through the two corners:
    // the drawings all show the upper one swung up as far as the snout will go, and
    // that is what puts the tip of it level with the top of the back. A lower jaw
    // dropping away from a fixed snout gives a crocodile instead. The corner is the
    // ring where the head meets the body, so the profile runs on into the dorsal hump
    // without a step in front of it.
    const float cornerX=.156f,cornerY=-.024f,cornerZ=.215f;
    float open=.5f+.5f*std::sin(phase*.30f);
    auto hinge=[&](Vec3 p,float ang) {
      float dy=p.y-cornerY,dz=p.z-cornerZ,c=std::cos(ang),sn=std::sin(ang);
      return Vec3{p.x,cornerY+dy*c+dz*sn,cornerZ-dy*sn+dz*c};
    };
    // A narrower swing than it looks as though it should be: in the plates the jaws
    // are a long pair of toothed bars opening perhaps thirty degrees, not a box
    // hinged wide. Most of the size of the mouth is the length of the jaws.
    // Held down far enough that the tip of the upper jaw always finishes below the
    // snout: swung any higher it stands proud of the forehead and the profile in
    // front of the eye goes concave, which is a dent no anglerfish has.
    const float angUp=.18f+.40f*open;
    // The jaw is a broad U seen from above, not a parabola: a high power leaves the
    // front of it nearly flat, which is what makes the snout blunt instead of beaked.
    auto reach=[&](float u,float len) { return cornerZ+len*(1-std::pow(std::abs(u),2.4f)); };
    // The upper jaw is a bar slung under the front of the head, not the top edge of
    // it. Drawing it as the edge leaves the animal with no forehead at all and the
    // head comes out as a beak on a ball; the live photographs show a blunt rounded
    // snout standing well above the tooth row.
    auto snoutAt=[&](float u) {
      return Vec3{cornerX*u,.150f-.174f*u*u,cornerZ+.115f*(1-std::pow(std::abs(u),2.2f))};
    };
    auto upperRaw=[&](float u) { return Vec3{cornerX*u,.025f-.049f*u*u,reach(u,.215f)}; };
    // Each jaw is a rolled bar, not a line: an outer margin where the skin ends and
    // an inner one a little back inside the mouth, with the bony lip between them and
    // the teeth rooted on the inner edge.
    auto upperOut=[&](float u) { return hinge(upperRaw(u),angUp); };
    auto upperIn=[&](float u) { return hinge(upperRaw(u)+Vec3{0,-.011f,-.018f},angUp); };
    // The lower jaw does not hang. In the live animal the joint is low and well back
    // and the mandible runs forward and UP from it, its tip finishing level with the
    // upper jaw's, so the mouth is a slot tilted up at the front and the fish is
    // looking and gaping upwards. What hangs below is not the jaw at all but the
    // throat, a slack pouch slung under the mandible - and building the drop into
    // the jaw instead of into the pouch is what kept giving this a hanging lower lip.
    // The mandible barely moves. Opening the mouth by swinging it down is what kept
    // flattening it: at a quarter of a radian the chin ends up level with the joint
    // whatever rise is built into the bone. It is held up at about twenty degrees at
    // every gape and the mouth is opened by lifting the upper jaw instead, which is
    // the pose in the photographs anyway.
    const float angLow=-(.01f+.05f*open);
    // Bowed, not straight. A mandible drawn as a straight bar from the joint to the
    // chin lies flat in side view, its tooth row edge on, and the mouth becomes a
    // letterbox. The live jaw sags a little between the two and comes back up, so the
    // row is presented to the side and the mouth is a scoop - while the chin itself
    // still finishes above the joint, which is the thing that matters.
    auto lowerRaw=[&](float u) {
      float t=1-std::abs(u);
      // The rise at the chin is what keeps the mandible pointing up; it has been
      // eroded twice by changes to the hinge height, so it is written as an
      // explicit clearance above the joint rather than a leftover offset.
      float y=cornerY+.089f*t-.055f*std::sin(Pi*t);
      return Vec3{cornerX*1.01f*u,y,reach(u,.232f)};
    };
    auto lowerIn=[&](float u) { return hinge(lowerRaw(u),angLow); };
    auto lowerOut=[&](float u) { return hinge(lowerRaw(u)+Vec3{0,-.013f,-.010f},angLow); };
    // The bar has real depth; below that the throat starts.
    auto lowerBase=[&](float u) { return hinge(lowerRaw(u)+Vec3{0,-.052f,-.016f},angLow); };
    auto rim=[&](float u,float sign) {
      float th=u*Pi*.5f;
      return Vec3{.156f*std::sin(th),cornerY+sign*.208f*std::cos(th),cornerZ};
    };
    // Exactly half the ring's facets. The head and the throat are lofted from the
    // corner ring, and if they are subdivided differently from the ring itself their
    // chords cut inside its chords and the join opens: a ring of slivers round the
    // head with the water showing through, which reads as a groove. At facets/2 the
    // two share every vertex round the seam.
    const int jawSteps=facets/2;
    {
      Vec3 headAxis{0,-.016f,.300f},throatAxis{0,-.042f,.276f};
      Vec3 roofHub{0,.058f,.226f},floorHub{0,-.070f,.236f};
      for(int j=0;j<jawSteps;++j) {
        float u0=-1+2.f*j/jawSteps,u1=-1+2.f*(j+1)/jawSteps;
        // Snout and cheeks, then the distensible throat from the lower lip back. The
        // snout takes what light there is; the corners of the mouth never do.
        Color cheek=scale(back,1.f+.42f*(1-std::abs(u0+u1)*.5f));
        // Two spans with the middle pushed out, so the cheek is domed. As one flat
        // panel from the rim to the jaw the head is a ramp, and a ramp is what made
        // the face read as a wedge however right the jaws were.
        // The swelling belongs on the cheeks, not on the midline: pushing the whole
        // row out just raises the snout and flattens the top again.
        auto cheekAt=[&](float u) {
          Vec3 r0=rim(u,1),t0=snoutAt(u),m=r0+(t0-r0)*.52f;
          return m+unit(m-headAxis)*(.032f*std::abs(u)*(1-u*u)+.003f);
        };
        panelN(cheekAt(u0),cheekAt(u1),rim(u1,1),rim(u0,1),scale(cheek,.92f),headAxis);
        panelN(snoutAt(u0),snoutAt(u1),cheekAt(u1),cheekAt(u0),cheek,headAxis);
        // The front of the head, from the snout down to the jaw: this is the panel
        // that stretches when the mouth opens, and it is the whole forehead.
        panelN(upperOut(u0),upperOut(u1),snoutAt(u1),snoutAt(u0),scale(cheek,.86f),headAxis);
        // Outward from the tooth edge: the bony lip, then the outside of the jaw
        // bar, then the throat, which is slung from the bar back to the belly and
        // sags in the middle. The sag is the deepest thing at the front of the fish
        // and it deepens as the mouth opens, exactly as the pouch does in the film.
        auto sagAt=[&](float u) {
          Vec3 b=lowerBase(u),r=rim(u,-1),m=b+(r-b)*.45f;
          return Vec3{m.x*.90f,m.y-(.025f+.045f*open)*(1-u*u),m.z};
        };
        panelN(sagAt(u0),sagAt(u1),rim(u1,-1),rim(u0,-1),bellyCol,throatAxis);
        panelN(lowerBase(u0),lowerBase(u1),sagAt(u1),sagAt(u0),scale(bellyCol,.94f),throatAxis);
        panelN(lowerOut(u0),lowerOut(u1),lowerBase(u1),lowerBase(u0),scale(bellyCol,.88f),throatAxis);
        panel(upperOut(u0),upperOut(u1),upperIn(u1),upperIn(u0),lip);
        panel(lowerIn(u0),lowerIn(u1),lowerOut(u1),lowerOut(u0),lip);
        // The lining is drawn both ways round: whichever way the mouth is turned the
        // inside of it has to stay closed rather than show the ocean through the skin.
        tri(upperIn(u0),upperIn(u1),roofHub,mawRoof);
        tri(lowerIn(u1),lowerIn(u0),floorHub,mawFloor);
      }
      tri(roofHub,floorHub,rim(-1,1),mawFloor);
      tri(roofHub,rim(1,1),floorHub,mawFloor);
      // The back of the throat. Without it the body is an open bag: from underneath,
      // with the jaw dropped, the mouth looks straight through the fish to the water.
      Vec3 hub{0,cornerY,cornerZ};
      for(int j=0;j<facets;++j) {
        float a0=j*2*Pi/facets,a1=(j+1)*2*Pi/facets;
        tri(hub,{.156f*std::sin(a0),cornerY+.208f*std::cos(a0),cornerZ},
                {.156f*std::sin(a1),cornerY+.208f*std::cos(a1),cornerZ},mawFloor);
      }
    }
    {
      // Teeth: a comb the whole length of both jaws, not a handful of fangs at the
      // front. The specimens carry twenty or more a side, long and short alternating,
      // all curving inward and back, and that comb is most of what makes the face
      // read at all. Two crossed blades apiece so one stays visible edge on.
      // The blades are squared about the tooth's own axis, and the reference for that
      // has to be the axis the tooth is least aligned with. Crossing against a fixed z
      // sends the front teeth, which point nearly straight back, a tenth of a length
      // sideways: unit() of a vector that has cancelled to nothing.
      auto spike=[&](Vec3 root,Vec3 tip,float w) {
        Vec3 ax=tip-root;
        float ux=std::abs(ax.x),uy=std::abs(ax.y),uz=std::abs(ax.z);
        Vec3 ref=(ux<=uy&&ux<=uz)?Vec3{1,0,0}:(uy<=uz)?Vec3{0,1,0}:Vec3{0,0,1};
        Vec3 e1=unit(cross(ax,ref))*w,e2=unit(cross(ax,e1))*w;
        tri(root-e1,root+e1,tip,toothCol); tri(root-e2,root+e2,tip,toothCol);
      };
      // A tooth grows out of the bone across the gape and then hooks back towards the
      // throat. It does not aim at a fixed spot in the mouth: a row aimed at one point
      // lies flat along the jaw at the front, where the point is nearly straight back,
      // and turns outwards at the corners, where it is nearly straight ahead. The
      // across-the-gape direction is read off the opposing jaw at the same station,
      // which is what the bone actually faces, and the hook is a fixed share of that
      // turned towards the back of the mouth. Stations stop short of the corners,
      // where the two jaws meet and the direction across has nothing left of it.
      const int n=close?11:mid?6:0;
      const Vec3 throatAt{0,-.010f,.232f};
      const float hook=.40f;
      // What a tooth stands up out of is its own jaw, so the first term is the jaw
      // bar's own thickness - outward from the bone's inner face - and not the
      // direction of the opposite jaw. Aiming across the gape looks reasonable until
      // the mandible projects past the upper jaw, as this one does: then the line
      // between the two is slanted, the hook towards the throat swamps it and the
      // front teeth end up lying flat along the bone pointing backwards, which is
      // what was wrong. The thickness turns with the gape by itself, so the fangs
      // follow the jaw without any further work.
      auto fang=[&](Vec3 root,Vec3 stand,float u,float len) {
        Vec3 d=unit(unit(stand)*(1-hook)+unit(throatAt-root)*hook+Vec3{-.22f*u,0,0});
        if(d.z>0) {
          d.z=0;
          float m=std::sqrt(d.x*d.x+d.y*d.y);
          d=m>1e-4f?d*(1.f/m):unit(throatAt-root);
        }
        return root+d*len;
      };
      for(int j=0;n && j<=n;++j) {
        float u=(-1+2.f*j/n)*.90f;
        float len=.072f*(1-.55f*u*u)*(j%2?.66f:1.f);
        Vec3 ru=upperIn(u),rl=lowerIn(u);
        spike(ru,fang(ru,ru-upperOut(u),u,len),.0034f);
        spike(rl,fang(rl,rl-lowerBase(u),u,len),.0034f);
      }
    }
    {
      // The eye. It really is only a twenty-fifth of the animal across, but in every
      // photograph it is the one bright point on the face, so it is given a pale iris
      // and a dark pupil and turned to face out, forward and up rather than lying
      // flat on the flank where nothing ever lights it.
      // Measured off the live animal, the eye is not on the snout at all: it is
      // three tenths of the way along the fish, behind the corner of the mouth and
      // high on the shoulder. Every attempt at putting it forward on the head looked
      // wrong however carefully it was drawn.
      for(int side=-1;side<=1;side+=2) {
        Vec3 c{side*.117f,.093f,.254f},n=unit(Vec3{side*.70f,.65f,-.28f});
        Vec3 e1=unit(cross(n,Vec3{0,1,0})),e2=unit(cross(n,e1));
        const int k=close?6:4;
        auto disc=[&](float r,float out,Color col) {
          Vec3 o=c+n*out;
          for(int j=0;j<k;++j) {
            float a0=j*2*Pi/k,a1=(j+1)*2*Pi/k;
            tri(o,o+e1*(std::cos(a1)*r)+e2*(std::sin(a1)*r),
                  o+e1*(std::cos(a0)*r)+e2*(std::sin(a0)*r),col);
          }
        };
        disc(.020f,0,{84,88,80});
        disc(.0125f,.0030f,{13,13,17});
        if(close) {
          Vec3 o=c+n*.0055f;
          tri(o+e1*.004f+e2*.005f,o+e1*.008f+e2*.001f,o+e1*.003f-e2*.002f,{198,214,216});
        }
      }
    }
    {
      // The illicium: a rod off the top of the snout carrying the lamp out over the
      // mouth, which is the only light anything down here ever sees.
      auto rod=[&](Vec3 p,Vec3 q,float w,Color col) {
        Vec3 ax=unit(q-p),e1=unit(cross(ax,Vec3{0,0,1}))*w,e2=unit(cross(ax,e1))*w;
        panel(p-e1,p+e1,q+e1,q-e1,col); panel(p-e2,p+e2,q+e2,q-e2,col);
      };
      float sway=std::sin(phase*.24f)*.045f;
      // The rod leaves the head two thirds of the way forward along it, close behind
      // the snout, which is where the step in the dorsal profile sits in every
      // specimen photograph - not back over the middle of the skull.
      // The rod leaves the back just behind the head, not off the snout: in the
      // live animal its base is a third of the way along the fish, behind the eye.
      Vec3 p0{0,.205f,.156f},p1{sway*.5f,.300f,.230f},p2{sway,.372f,.330f},esca{sway*1.1f,.386f,.400f};
      rod(p0,p1,.0055f,{38,34,34}); rod(p1,p2,.0042f,{38,34,34}); rod(p2,esca,.0034f,{38,34,34});
      material_=7;
      float glow=.60f+.40f*std::sin(phase*.34f);
      Color lamp=scale(Color{150,238,216},glow);
      const int n=close?6:4;
      for(int k=0;k<n;++k) {
        float a0=k*2*Pi/n,a1=(k+1)*2*Pi/n,r=.020f;
        tri(esca,esca+Vec3{std::sin(a0)*r,std::cos(a0)*r,.012f},
                 esca+Vec3{std::sin(a1)*r,std::cos(a1)*r,.012f},lamp);
      }
      if(close) for(int k=0;k<3;++k) {                       // the esca's few filaments
        float a=(k-1)*.7f;
        rod(esca,esca+Vec3{std::sin(a)*.030f,.052f,std::cos(a)*.016f},.0022f,scale(lamp,.7f));
      }
      material_=0;
    }
    {
      // Dorsal, anal and caudal, all at the back and all larger than they look as
      // though they should be: on the live animal they are broad, almost clear fans
      // with the rays showing through darker, which is the opposite way round from
      // the dark web and pale rays this had before.
      auto medianFan=[&](Vec3 b0,Vec3 b1,Vec3 t0,Vec3 t1,int n) {
        for(int j=0;j<n;++j) {
          float f0=float(j)/n,f1=float(j+1)/n;
          panel(b0+(b1-b0)*f0,b0+(b1-b0)*f1,t0+(t1-t0)*f1,t0+(t1-t0)*f0,web);
        }
        if(close) for(int j=0;j<=n;++j) {
          float f=float(j)/n;
          Vec3 b=b0+(b1-b0)*f,t=t0+(t1-t0)*f,d{0,-(t.z-b.z),t.y-b.y};
          float m=std::sqrt(d.y*d.y+d.z*d.z);
          if(m>1e-5f) { d=d*(.0055f/m); panel(b-d,b+d,t+d,t-d,rayCol); }
        }
      };
      float wag=std::sin(phase*.50f-1.5f)*.030f*activity;
      medianFan({wag*.4f,.210f,-.120f},{wag*.7f,.146f,-.200f},
                {wag*.4f,.347f,-.172f},{wag*.7f,.243f,-.269f},close?7:3);
      medianFan({wag*.4f,-.276f,-.120f},{wag*.7f,-.190f,-.200f},
                {wag*.4f,-.392f,-.178f},{wag*.7f,-.255f,-.272f},close?5:2);
      // Caudal: a rounded fan of rays off the peduncle, not a forked tail.
      const int n=close?7:4;
      Vec3 root{bend(-.266f)+wag,.002f,-.266f};
      for(int j=0;j<n;++j) {
        float a0=(float(j)/n-.5f)*2.10f,a1=(float(j+1)/n-.5f)*2.10f;
        Vec3 t0{root.x+wag*.8f,root.y+std::sin(a0)*.200f,root.z-.235f*std::cos(a0)};
        Vec3 t1{root.x+wag*.8f,root.y+std::sin(a1)*.200f,root.z-.235f*std::cos(a1)};
        tri(root,t0,t1,web);
        if(close) {
          Vec3 d{0,-(t0.z-root.z),t0.y-root.y};
          float m=std::sqrt(d.y*d.y+d.z*d.z);
          if(m>1e-5f) { d=d*(.0055f/m); panel(root-d,root+d,t0+d,t0-d,rayCol); }
        }
      }
      // Pectorals: rounded translucent fans on a short stalk halfway along the
      // flank, the same clear membrane as the rest rather than the dark paddles
      // they were. On the live animal they are as conspicuous as the dorsal.
      for(int side=-1;side<=1;side+=2) {
        Vec3 stalk{side*.186f,-.150f,-.056f},hub{side*.206f,-.166f,-.100f};
        const int n=close?4:2;
        for(int j=0;j<n;++j) {
          float a0=(float(j)/n-.5f)*1.9f,a1=(float(j+1)/n-.5f)*1.9f;
          Vec3 t0{hub.x+side*.018f,hub.y+std::sin(a0)*.084f,hub.z-.088f*std::cos(a0)};
          Vec3 t1{hub.x+side*.018f,hub.y+std::sin(a1)*.084f,hub.z-.088f*std::cos(a1)};
          tri(hub,t0,t1,scale(web,.84f));
          if(close) {
            Vec3 d{0,-(t0.z-hub.z),t0.y-hub.y};
            float m=std::sqrt(d.y*d.y+d.z*d.z);
            if(m>1e-5f) { d=d*(.0045f/m); panel(hub-d,hub+d,t0+d,t0-d,rayCol); }
          }
        }
        tri(stalk,hub+Vec3{0,.016f,.010f},hub+Vec3{0,-.016f,.010f},scale(bellyCol,.9f));
      }
    }
    return;
  }
  if(species==Species::Oarfish) {
    // A silver ribbon: barely any thickness, a scarlet dorsal fin that runs the whole
    // body and rises into a plume of long rays over the head, and a pair of oar-shaped
    // pelvic filaments. It holds itself head-up and drives the fin, not the body.
    Color silver{196,202,210},shade{146,154,166},crest{192,44,52},blotch{92,102,116};
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>(gap.x*gap.x+gap.y*gap.y+gap.z*gap.z)*.0060f;
    int joints=close?14:8;
    auto ripple=[&](float t) { return std::sin(phase*.55f-t*9.0f)*.045f*(.18f+t)*activity; };
    auto depthAt=[&](float t) { return .078f*(1-t*.78f)*(t<.08f?t*12.0f:1.0f); };
    auto midAt=[&](float t) { return .015f*(1-t*.55f); };
    for(int j=1;j<=joints;++j) {
      float t0=float(j-1)/joints,t1=float(j)/joints;
      float z0=.50f-t0,z1=.50f-t1,x0=ripple(t0),x1=ripple(t1);
      float d0=depthAt(t0),d1=depthAt(t1),w0=midAt(t0),w1=midAt(t1);
      Color body=(j%4==1)?blotch:silver;
      for(int side=-1;side<=1;side+=2) {
        tri({x0,d0,z0},{x1,d1,z1},{x1+side*w1,d1*.20f,z1},side<0?body:scale(body,.86f));
        tri({x0,d0,z0},{x1+side*w1,d1*.20f,z1},{x0+side*w0,d0*.20f,z0},side<0?body:scale(body,.86f));
        tri({x0,-d0*.92f,z0},{x0+side*w0,d0*.20f,z0},{x1+side*w1,d1*.20f,z1},side<0?shade:scale(shade,.88f));
        tri({x0,-d0*.92f,z0},{x1+side*w1,d1*.20f,z1},{x1,-d1*.92f,z1},side<0?shade:scale(shade,.88f));
      }
      // The scarlet fin, every ray of it, from the nape to the tip of the tail.
      panel({x0,d0,z0},{x1,d1,z1},{x1,d1+.055f*(1-t1*.45f),z1},{x0,d0+.055f*(1-t0*.45f),z0},crest);
    }
    if(close) for(int k=0;k<6;++k) {                  // the plume over the head
      float z=.430f-k*.055f,lean=std::sin(phase*.3f+k*.5f)*.030f;
      tri({0,depthAt(.5f-z<0?0:.5f-z),z},{lean,.300f-k*.028f,z+.030f},{0,depthAt(.06f),z-.040f},crest);
    }
    for(int side=-1;side<=1;side+=2) {                // the oars
      float swing=std::sin(phase*.25f+side)*.070f;
      Vec3 rootP{side*.012f,-.075f,.330f},tipP{side*.045f+swing,-.330f,.190f};
      panel(rootP+Vec3{0,0,.010f},rootP-Vec3{0,0,.010f},tipP-Vec3{0,0,.012f},tipP+Vec3{0,0,.012f},crest);
      tri(tipP+Vec3{0,.010f,.028f},tipP-Vec3{0,.010f,.028f},tipP+Vec3{side*.014f,-.070f,0},crest);
    }
    // An oarfish's eye is large for the sliver of a head it sits in - it lives where
    // there is nothing to see - and the head is barely wider than a blade, so the disc
    // leans hard.
    for(int side=-1;side<=1;side+=2)
      fishEye(tri,{side*.0105f,.026f,.442f},float(side),.013f,.05f,close);
    return;
  }
  if(species==Species::Tuna) {
    // Bluefin. Thunniform: the torso is a stiff torpedo and only the last third flexes,
    // driving a lunate tail on a keeled peduncle. Yellow finlets run down to it.
    uint32_t tint=hash(animal.id*2654435761u+7u);
    float shade=.85f+(tint&255)/255.0f*.30f;
    Color back=scale(Color{23,43,78},shade),flank=scale(Color{72,108,140},shade);
    Color silver=scale(Color{174,189,200},.94f+shade*.06f),belly=scale(Color{226,232,235},.94f+shade*.06f);
    Color finlet{228,190,64},fin=scale(Color{37,57,84},shade);
    float beat=std::sin(phase*1.15f)*activity;
    auto sway=[&](float z) { float t=clampf((-z-.02f)/.52f,0,1); return beat*t*t*.085f; };
    struct Station { float z,halfW,top,bottom; };
    static const Station body[8]={{.500f,.000f,.006f,-.004f},{.375f,.052f,.058f,.050f},
      {.225f,.079f,.112f,.100f},{.075f,.080f,.126f,.116f},{-.090f,.064f,.104f,.092f},
      {-.255f,.036f,.058f,.048f},{-.400f,.016f,.024f,.019f},{-.495f,.009f,.013f,.011f}};
    // Detail is spent on the fish that actually fill pixels; a school seen far off is a hull.
    Vec3 gap=animal.position-viewer_;
    bool close=!crowded() && size*size>(gap.x*gap.x+gap.y*gap.y+gap.z*gap.z)*.0052f;
    const int fine[8]={0,1,2,3,4,5,6,7},coarse[8]={0,2,3,4,5,7,7,7};
    const int* pick=close?fine:coarse;
    int stations=close?8:6,facets=close?6:4;
    auto section=[&](const Station& st,Vec3* out) {
      float x=sway(st.z);
      if(facets==6) {
        out[0]={x,st.top,st.z};                     out[1]={x+st.halfW*.88f,st.top*.40f,st.z};
        out[2]={x+st.halfW*.78f,-st.bottom*.52f,st.z}; out[3]={x,-st.bottom,st.z};
        out[4]={x-st.halfW*.78f,-st.bottom*.52f,st.z}; out[5]={x-st.halfW*.88f,st.top*.40f,st.z};
      } else {
        out[0]={x,st.top,st.z};      out[1]={x+st.halfW,st.top*.12f,st.z};
        out[2]={x,-st.bottom,st.z};  out[3]={x-st.halfW,st.top*.12f,st.z};
      }
    };
    const Color shell6[6]={back,flank,silver,belly,silver,flank};
    const Color shell4[4]={back,flank,belly,flank};
    const Color* shell=facets==6?shell6:shell4;
    Vec3 head[6],tailRing[6];
    section(body[pick[1]],head);
    Vec3 snout{sway(.5f),body[0].top,.5f};
    for(int e=0;e<facets;++e) triS(snout,head[e],head[(e+1)%facets],shell[e]);
    for(int k=2;k<stations;++k) {
      section(body[pick[k]],tailRing);
      for(int e=0;e<facets;++e) panelS(head[e],head[(e+1)%facets],tailRing[(e+1)%facets],tailRing[e],shell[e]);
      for(int e=0;e<facets;++e) head[e]=tailRing[e];
    }
    // Dorsal and ventral lines, so fins and finlets sit on the hull instead of floating.
    auto edge=[&](float z,bool top) {
      for(int k=1;k<8;++k) if(z>=body[k].z || k==7) {
        const Station& a1=body[k-1];const Station& b1=body[k];
        float t=clampf((a1.z-z)/(a1.z-b1.z),0,1);
        return top?a1.top+(b1.top-a1.top)*t:-(a1.bottom+(b1.bottom-a1.bottom)*t);
      }
      return 0.0f;
    };
    float lag=std::sin(phase*1.15f-.85f)*activity;       // the blade trails the peduncle
    float root=sway(-.495f),sweep=lag*.075f;
    Vec3 up1{root,.013f,-.492f},low1{root,-.011f,-.492f},forkc{root+sweep*.3f,.001f,-.528f};
    Vec3 leadU{root+sweep*.55f,.086f,-.556f},tipU{root+sweep,.180f,-.616f},trailU{root+sweep*.5f,.080f,-.512f};
    Vec3 leadL{root+sweep*.55f,-.082f,-.552f},tipL{root+sweep,-.172f,-.610f},trailL{root+sweep*.5f,-.076f,-.510f};
    tri(up1,leadU,tipU,fin); tri(up1,tipU,trailU,scale(fin,.9f)); tri(up1,trailU,forkc,scale(fin,.78f));
    tri(low1,tipL,leadL,fin); tri(low1,trailL,tipL,scale(fin,.9f)); tri(low1,forkc,trailL,scale(fin,.78f));
    // First dorsal is a tall spiny triangle, the second and the anal are short sickles.
    tri({sway(.20f),edge(.20f,true)-.004f,.20f},{sway(.14f),edge(.14f,true)+.100f,.135f},
        {sway(.07f),edge(.07f,true)-.004f,.07f},fin);
    tri({sway(-.03f),edge(-.03f,true)-.003f,-.03f},{sway(-.12f),edge(-.12f,true)+.070f,-.125f},
        {sway(-.10f),edge(-.10f,true)-.003f,-.10f},fin);
    tri({sway(-.09f),edge(-.09f,false)+.003f,-.09f},{sway(-.18f),edge(-.18f,false)-.062f,-.185f},
        {sway(-.16f),edge(-.16f,false)+.003f,-.16f},scale(fin,1.15f));
    for(int side=-1;side<=1;++side) if(side) {
      tri({side*.074f,.012f,.245f},{side*.132f,-.048f,.055f},{side*.066f,-.020f,.155f},scale(fin,1.1f));
      tri({side*.034f,-.104f,.185f},{side*.050f,-.140f,.098f},{side*.028f,-.098f,.128f},silver);
      // It was a flat square panel. The cheek is already turning in towards the snout
      // here, so the eye lies in that plane rather than square to the world.
      fishEye(tri,{side*.061f,.020f,.342f},float(side),.022f,.037f,close);
      float keel=sway(-.42f);
      tri({keel+side*.013f,-.001f,-.360f},{keel+side*.040f,-.003f,-.432f},{keel+side*.012f,-.001f,-.470f},silver);
    }
    if(close) for(int k=0;k<7;++k) {
      float z=-.170f-k*.033f,x=sway(z);
      tri({x,edge(z,true)-.002f,z},{x,edge(z,true)+.024f,z-.012f},{x,edge(z,true)-.002f,z-.028f},finlet);
      tri({x,edge(z-.02f,false)+.002f,z-.020f},{x,edge(z-.02f,false)-.022f,z-.032f},{x,edge(z-.02f,false)+.002f,z-.048f},finlet);
    }
    return;
  }
  // ---- Every remaining bony fish comes out of one parametric body ----------------
  // The shared mesh used to be a four-sided lozenge, so all of these read as the same
  // animal in a different colour. A plan now states where the body is deepest, how round
  // it is, which fins it carries and how the tail is cut; a few species add their own
  // trademark afterwards. The measurements follow the real fish as closely as a couple
  // of dozen triangles can.
  struct Plan {
    float depth,girth,peak,bellyFull,peduncle;   // body envelope, as fractions of length
    float d1z,d1len,d1h;                         // first dorsal, h<=0 means absent
    float d2z,d2len,d2h;                         // second dorsal
    float d3z,d3len,d3h;                         // third dorsal, only the cod carries one
    float anz,anlen,anh;                         // anal
    float an2z,an2len,an2h;                      // second anal
    float pecLen,pecDrop;                        // pectoral reach and how far it droops
    float fork,span;                             // caudal: 0 rounded to 1 deeply forked
    uint8_t finlets,style;                       // finlet pairs; 0 carangiform 1 sub- 2 benthic
  };
  static const Plan plans[]={
    {.085f,.048f,.060f,.920f,.160f, -.020f,.050f,.036f,  0,0,0,             0,0,0,             -.180f,.070f,.030f, 0,0,0,             .060f,.030f,.70f,.120f,0,1}, // 0 sprat
    {.190f,.072f,.040f,.950f,.200f,  .020f,.160f,.055f,  0,0,0,             0,0,0,             -.140f,.100f,.050f, 0,0,0,             .080f,.020f,.25f,.130f,0,0}, // 1 reef fish
    {.130f,.058f,.100f,.880f,.120f,  .100f,.060f,.055f, -.100f,.130f,.035f, 0,0,0,             -.160f,.120f,.032f, 0,0,0,             .130f,.050f,.85f,.155f,0,0}, // 2 horse mackerel
    {.075f,.052f,.060f,.900f,.140f,  .060f,.040f,.045f, -.220f,.050f,.040f, 0,0,0,             -.240f,.050f,.038f, 0,0,0,             .070f,.020f,.75f,.105f,0,1}, // 3 barracuda
    {.075f,.062f,.100f,.850f,.300f,  .100f,.050f,.045f, -.080f,.120f,.035f, 0,0,0,             -.100f,.110f,.030f, 0,0,0,             .090f,.050f,.00f,.075f,0,2}, // 4 goby
    {.105f,.050f,.120f,.900f,.160f,  .000f,.060f,.040f,  0,0,0,             0,0,0,             -.200f,.080f,.035f, 0,0,0,             .050f,.020f,.60f,.105f,0,1}, // 5 lanternfish
    {.245f,.055f,.020f,1.00f,.140f,  .000f,.180f,.065f,  0,0,0,             0,0,0,             -.140f,.120f,.060f, 0,0,0,             .070f,.000f,.10f,.115f,0,0}, // 6 butterflyfish
    {.215f,.058f,.000f,1.00f,.140f, -.020f,.190f,.055f,  0,0,0,             0,0,0,             -.140f,.140f,.050f, 0,0,0,             .080f,.000f,.35f,.135f,0,0}, // 7 surgeonfish
    {.105f,.062f,.120f,.820f,.170f,  .100f,.060f,.050f, -.100f,.070f,.040f, 0,0,0,             -.140f,.070f,.035f, 0,0,0,             .090f,.040f,.70f,.120f,0,2}, // 8 goatfish
    {.165f,.085f,.100f,.880f,.180f,  .020f,.150f,.060f,  0,0,0,             0,0,0,             -.180f,.070f,.045f, 0,0,0,             .130f,.050f,.05f,.115f,0,2}, // 9 rockfish
    {.085f,.105f,.160f,.750f,.180f,  .100f,.060f,.045f, -.080f,.140f,.035f, 0,0,0,             -.100f,.120f,.030f, 0,0,0,             .170f,.060f,.00f,.085f,0,2}, // 10 sculpin
    {.085f,.042f,.160f,.880f,.120f,  .160f,.050f,.085f,  0,0,0,             0,0,0,             -.240f,.060f,.035f, 0,0,0,             .050f,.020f,.55f,.085f,0,1}, // 11 viperfish
    {.135f,.068f,.060f,.860f,.150f,  .020f,.070f,.050f, -.220f,.022f,.020f, 0,0,0,             -.200f,.070f,.045f, 0,0,0,             .100f,.040f,.50f,.145f,0,1}, // 12 salmon
    {.225f,.078f,.080f,.950f,.150f,  .020f,.180f,.055f,  0,0,0,             0,0,0,             -.180f,.090f,.050f, 0,0,0,             .120f,.030f,.55f,.145f,0,0}, // 13 sea bream
    {.115f,.060f,.140f,.880f,.100f,  0,0,0,            -.240f,.040f,.030f,  0,0,0,             -.200f,.050f,.035f, 0,0,0,             .130f,.060f,.95f,.165f,0,0}, // 14 marlin
    {.085f,.048f,.100f,.900f,.100f,  .100f,.090f,.055f, -.060f,.050f,.035f, 0,0,0,             -.120f,.050f,.032f, 0,0,0,             .080f,.030f,.90f,.130f,7,0}, // 15 Spanish mackerel
    {.145f,.078f,.100f,.840f,.130f,  .140f,.050f,.045f,  .000f,.070f,.042f,-.160f,.060f,.038f, -.060f,.060f,.040f,-.220f,.050f,.035f, .090f,.040f,.05f,.125f,0,1}, // 16 cod
    {.095f,.050f,.080f,.900f,.110f,  .100f,.060f,.045f, -.060f,.040f,.030f, 0,0,0,             -.120f,.040f,.028f, 0,0,0,             .060f,.020f,.90f,.125f,5,0}, // 17 mackerel
    {.145f,.062f,.080f,.850f,.160f,  .040f,.170f,.050f,  0,0,0,             0,0,0,             -.160f,.110f,.042f, 0,0,0,             .110f,.040f,.45f,.135f,0,1}, // 18 Atka mackerel
    {.115f,.046f,.040f,.940f,.140f, -.020f,.060f,.040f,  0,0,0,             0,0,0,             -.240f,.080f,.030f, 0,0,0,             .060f,.030f,.80f,.130f,0,1}, // 19 herring
    {.062f,.038f,.040f,.920f,.100f, -.180f,.040f,.035f,  0,0,0,             0,0,0,             -.220f,.050f,.030f, 0,0,0,             .050f,.020f,.80f,.095f,5,1}, // 20 saury
    {.135f,.062f,.080f,.880f,.120f,  .100f,.050f,.035f, -.060f,.160f,.045f, 0,0,0,             -.180f,.130f,.040f, 0,0,0,             .090f,.040f,.85f,.150f,0,0}, // 21 yellowtail
  };
  int planId=0;
  switch(species) {
    case Species::ReefFish: planId=1; break;
    case Species::Jack: planId=2; break;
    case Species::Barracuda: planId=3; break;
    case Species::Goby: planId=4; break;
    case Species::Lanternfish: planId=5; break;
    case Species::Butterflyfish: planId=6; break;
    case Species::Surgeonfish: planId=7; break;
    case Species::Goatfish: planId=8; break;
    case Species::Rockfish: planId=9; break;
    case Species::Sculpin: planId=10; break;
    case Species::Viperfish: planId=11; break;
    case Species::Salmon: planId=12; break;
    case Species::SeaBream: planId=13; break;
    case Species::Marlin: planId=14; break;
    case Species::SpanishMackerel: planId=15; break;
    case Species::Cod: planId=16; break;
    case Species::Mackerel: planId=17; break;
    case Species::AtkaMackerel: planId=18; break;
    case Species::Herring: planId=19; break;
    case Species::Saury: planId=20; break;
    case Species::Yellowtail: planId=21; break;
    default: planId=0; break;
  }
  const Plan& P=plans[planId];
  Color back{91,139,160},belly{203,218,214};
  switch(species) {
    case Species::ReefFish: back={217,177,70}; belly={234,208,112}; break;
    case Species::Butterflyfish: back={233,196,74}; belly={247,231,171}; break;
    case Species::Surgeonfish: back={53,131,209}; belly={222,205,96}; break;
    case Species::SeaBream: back={207,123,127}; belly={240,216,208}; break;
    case Species::Goby: case Species::Goatfish: back={161,145,107}; belly={192,179,134}; break;
    case Species::Rockfish: back={179,107,80}; belly={214,182,146}; break;
    case Species::Sculpin: back={141,138,105}; belly={192,179,134}; break;
    case Species::Lanternfish: back={115,146,177}; belly={203,218,214}; break;
    case Species::Salmon: back={98,122,142}; belly={229,177,151}; break;
    case Species::Marlin: back={32,60,110}; belly={208,217,225}; break;
    case Species::SpanishMackerel: back={74,116,116}; belly={223,229,227}; break;
    case Species::Cod: back={139,127,87}; belly={215,207,177}; break;
    case Species::Mackerel: back={64,96,124}; belly={220,227,229}; break;
    case Species::AtkaMackerel: back={97,111,73}; belly={207,201,169}; break;
    case Species::Herring: back={85,117,147}; belly={229,233,235}; break;
    case Species::Saury: back={52,86,124}; belly={215,221,227}; break;
    case Species::Yellowtail: back={72,104,128}; belly={226,214,150}; break;
    default: break;
  }
  // No two fish in a shoal are stamped from the same die.
  uint32_t tint=hash(animal.id*2654435761u+7u);
  float shade=.84f+(tint&255)/255.0f*.32f;
  back=scale(back,shade); belly=scale(belly,.93f+shade*.07f);
  Color finCol=scale(back,.86f);
  float stretch=.94f+((tint>>8)&255)/255.0f*.13f;
  float depth=P.depth*(2.0f-stretch),girth=P.girth*stretch;
  float lift=ecology::bottomDweller(species)?depth*.45f:0.0f;   // bottom fish ride on their belly
  // Three levels. Shoals run to seventy fish and the static world already takes two
  // thirds of the triangle budget, so distant members are cut right back to a hull.
  Vec3 gap=animal.position-viewer_;
  float range2=gap.x*gap.x+gap.y*gap.y+gap.z*gap.z;
  int lod=size*size>range2*.0070f?0:size*size>range2*.0011f?1:2;
  if(jammed()) lod=2; else if(crowded() && lod<1) lod=1;
  bool close=lod==0;
  int facets=close?6:4;
  // Body envelope: an ellipse forward of the deepest point, a taper to the peduncle behind.
  auto fullness=[&](float z) {
    if(z>=P.peak) { float u=(z-P.peak)/(.50f-P.peak); return std::sqrt(std::max(0.0f,1-u*u)); }
    float u=clampf((P.peak-z)/(P.peak+.44f),0,1);
    return (1-u)*(1-u)*(1-P.peduncle)+P.peduncle;
  };
  float beat=std::sin(phase)*activity;
  auto sway=[&](float z) {
    float t=P.style==0?clampf((-z+.05f)/.55f,0,1)
           :P.style==1?clampf((.30f-z)/.80f,0,1)
                      :clampf((-z-.05f)/.45f,0,1);
    return beat*t*t*(P.style==2?.045f:P.style==1?.080f:.065f);
  };
  auto topAt=[&](float z) { return lift+depth*fullness(z); };
  auto botAt=[&](float z) { return lift-depth*P.bellyFull*fullness(z); };
  static const float stationZ[8]={.500f,.380f,.260f,.120f,-.020f,-.160f,-.300f,-.440f};
  auto section=[&](float z,Vec3* out) {
    float x=sway(z),w=girth*fullness(z),t=topAt(z),b=botAt(z);
    if(facets==6) {
      out[0]={x,t,z};                         out[1]={x+w*.88f,lift+(t-lift)*.36f,z};
      out[2]={x+w*.76f,lift+(b-lift)*.50f,z}; out[3]={x,b,z};
      out[4]={x-w*.76f,lift+(b-lift)*.50f,z}; out[5]={x-w*.88f,lift+(t-lift)*.36f,z};
    } else {
      out[0]={x,t,z}; out[1]={x+w,lift+(t-lift)*.10f,z};
      out[2]={x,b,z}; out[3]={x-w,lift+(t-lift)*.10f,z};
    }
  };
  auto hide=[&](int k) {
    return blend(back,belly,clampf(.5f-std::cos((k+.5f)*2*Pi/facets)*.68f,0,1));
  };
  static const int hullFine[7]={1,2,3,4,5,6,7},hullMid[4]={1,3,5,7},hullFar[3]={2,4,7};
  const int* hull=lod==0?hullFine:lod==1?hullMid:hullFar;
  int hullCount=lod==0?7:lod==1?4:3;
  Vec3 ringA[6],ringB[6];
  section(stationZ[hull[0]],ringA);
  Vec3 snout{sway(.5f),lift+depth*.04f,.5f};
  for(int k=0;k<facets;++k) triS(snout,ringA[k],ringA[(k+1)%facets],hide(k));
  for(int j=1;j<hullCount;++j) {
    section(stationZ[hull[j]],ringB);
    for(int k=0;k<facets;++k) panelS(ringA[k],ringA[(k+1)%facets],ringB[(k+1)%facets],ringB[k],hide(k));
    for(int k=0;k<facets;++k) ringA[k]=ringB[k];
  }
  // Caudal fin. The fork parameter slides it from a rounded paddle to a deep crescent.
  {
    float rz=-.440f,rx=sway(rz),tipZ=-.550f-P.fork*.050f,notchZ=-.548f+P.fork*.076f;
    float lagX=sway(-.560f);
    Vec3 rootT{rx,topAt(rz),rz},rootB{rx,botAt(rz),rz};
    Vec3 tipT{lagX,lift+P.span,tipZ},tipB{lagX,lift-P.span*.94f,tipZ};
    Vec3 notch{lagX*.7f,lift,notchZ};
    tri(rootT,tipT,notch,finCol);
    tri(rootT,notch,rootB,scale(finCol,.94f));
    tri(rootB,notch,tipB,finCol);
  }
  auto dorsal=[&](float zc,float len,float h,Color c) {
    if(h<=0) return;
    float z0=zc+len,z1=zc-len,za=zc-len*.35f;
    tri({sway(z0),topAt(z0)-.004f,z0},{sway(za),topAt(zc)+h,za},{sway(z1),topAt(z1)-.004f,z1},c);
  };
  auto ventral=[&](float zc,float len,float h,Color c) {
    if(h<=0) return;
    float z0=zc+len,z1=zc-len,za=zc-len*.35f;
    tri({sway(z0),botAt(z0)+.004f,z0},{sway(z1),botAt(z1)+.004f,z1},{sway(za),botAt(zc)-h,za},c);
  };
  dorsal(P.d1z,P.d1len,P.d1h,finCol);
  if(lod<2) {
    dorsal(P.d2z,P.d2len,P.d2h,finCol);
    dorsal(P.d3z,P.d3len,P.d3h,finCol);
    ventral(P.anz,P.anlen,P.anh,scale(finCol,1.1f));
    ventral(P.an2z,P.an2len,P.an2h,scale(finCol,1.1f));
    for(int side=-1;side<=1;side+=2) {
      float rx=side*girth*fullness(.17f)*.92f,ry=lift+(topAt(.17f)-lift)*.05f;
      tri({rx,ry,.180f},{rx+side*P.pecLen*.50f,ry-P.pecDrop,.180f-P.pecLen},{rx,ry-depth*.34f,.095f},scale(finCol,1.06f));
      if(close) tri({side*girth*.35f,botAt(.080f)+.004f,.080f},{side*girth*.55f,botAt(.080f)-.050f,.010f},
                    {side*girth*.30f,botAt(.020f)+.004f,.020f},scale(finCol,1.12f));
    }
  }
  if(P.finlets && close) for(int k=0;k<P.finlets;++k) {
    float z=-.200f-k*(.220f/P.finlets),x=sway(z);
    tri({x,topAt(z)-.002f,z},{x,topAt(z)+.020f,z-.010f},{x,topAt(z)-.002f,z-.024f},scale(finCol,1.2f));
    tri({x,botAt(z)+.002f,z},{x,botAt(z)-.020f,z-.010f},{x,botAt(z)+.002f,z-.024f},scale(finCol,1.2f));
  }
  // Trademarks: the handful of features that name a particular fish. The bill and the
  // sail change the outline, so they stay; the rest go once the fish is far enough off.
  if(lod<2) {
  if(species==Species::Goatfish) for(int side=-1;side<=1;side+=2)   // chin barbels
    tri({side*.012f,botAt(.40f),.400f},{side*.030f,botAt(.40f)-.050f,.300f},{side*.020f,botAt(.40f)-.010f,.395f},{220,200,145});
  if(species==Species::Cod)                                        // chin barbel
    tri({-.010f,botAt(.40f),.400f},{.010f,botAt(.40f),.400f},{0,botAt(.40f)-.055f,.360f},{214,203,168});
  if(species==Species::Rockfish) for(int k=0;k<5;++k) {            // spiny first dorsal rays
    float z=.120f-k*.055f;
    tri({sway(z),topAt(z),z},{sway(z),topAt(z)+.075f,z-.015f},{sway(z),topAt(z),z-.030f},scale(back,1.1f));
  }
  if(species==Species::Surgeonfish) for(int side=-1;side<=1;side+=2)  // the scalpel on the peduncle
    tri({side*girth*.3f,lift+.012f,-.330f},{side*girth*.3f,lift-.012f,-.330f},{side*girth*.3f,lift,-.400f},{238,240,236});
  if(species==Species::Sculpin) for(int side=-1;side<=1;side+=2)      // head spines
    tri({side*girth*.75f,lift+.020f,.280f},{side*girth*1.15f,lift+.045f,.215f},{side*girth*.80f,lift-.010f,.250f},belly);
  }
  if(species==Species::Marlin) {
    // It had no eye at all. A marlin's sits just behind the base of the bill, high on
    // the cheek, and the head is running away to a point by then, so the lean is steep.
    for(int side=-1;side<=1;side+=2) {
      const float z=.400f,w=girth*fullness(z);
      fishEye(tri,{sway(z)+side*w*.90f,lift+(topAt(z)-lift)*.36f,z},float(side),.015f,.20f,close);
    }
    tri({-.012f,lift,.500f},{.012f,lift,.500f},{0,lift+.004f,.860f},back);          // the bill
    for(int k=0;k<5;++k) {                                  // and the sail, one sheet of it
      float z0=.200f-k*.078f,z1=z0-.078f;
      float h0=.150f*std::sin((.200f-z0+.040f)*5.4f),h1=.150f*std::sin((.200f-z1+.040f)*5.4f);
      panel({sway(z0),topAt(z0)-.004f,z0},{sway(z1),topAt(z1)-.004f,z1},
            {sway(z1),topAt(z1)+std::max(0.0f,h1),z1},{sway(z0),topAt(z0)+std::max(0.0f,h0),z0},scale(back,1.05f));
    }
  }
  if(species==Species::Saury)                                                        // the beak
    tri({-.008f,lift-.006f,.500f},{.008f,lift-.006f,.500f},{0,lift-.016f,.580f},back);
  if(species==Species::Lanternfish) {
    material_=7;
    for(int side=-1;side<=1;side+=2) for(int j=0;j<(close?4:2);++j) {
      float z=.150f-j*.085f,w=girth*fullness(z)*1.02f;
      tri({side*w,botAt(z)+.010f,z-.014f},{side*w,botAt(z)+.010f,z+.014f},{side*w,botAt(z)+.040f,z},{99,221,221});
    }
    material_=0;
  }
}
}
