#include "scene.h"
#if defined(__GNUC__)
#pragma GCC optimize("O3", "fast-math")
#endif
namespace abyss {
void Scene::prehistoricMesh(const ecology::Animal& a) {
  using ecology::Species;
  material_=0;
  Vec3 up{0,1,0};
  if(ecology::bottomDweller(a.species)) {
    float d=.15f,x=a.position.x,z=a.position.z;
    up=unit({surface(x-d,z)-surface(x+d,z),2*d,surface(x,z-d)-surface(x,z+d)});
  }
  Vec3 forward{std::sin(a.yaw),0,std::cos(a.yaw)},right=unit(cross(up,forward));
  forward=unit(cross(right,up));
  auto world=[&](Vec3 v) { return a.position+(right*v.x+up*v.y+forward*v.z)*a.length; };
  auto tri=[&](Vec3 x,Vec3 y,Vec3 z,Color c) { add(world(x),world(y),world(z),c); };
  auto quad=[&](Vec3 w,Vec3 x,Vec3 y,Vec3 z,Color c) { tri(w,x,y,c); tri(w,y,z,c); };
  // Closed-body faces, wound outward and cullable. Shells and hulls only; the lobes,
  // spines and fin webs stay two-sided because they have no inside.
  auto turn=[&](Vec3 p) { return right*p.x+up*p.y+forward*p.z; };
  auto triS=[&](Vec3 x,Vec3 y,Vec3 z,Color c) {
    addSolid(world(x),world(y),world(z),c,turn((x+y+z)*(1.0f/3.0f)));
  };
  auto quadS=[&](Vec3 w,Vec3 x,Vec3 y,Vec3 z,Color c) { triS(w,x,y,c); triS(w,y,z,c); };
  // Sacabambaspis and the trilobites turn up in swarms of over a hundred, so the same
  // near/far decision the fish make is taken here too, once, for every ancient animal.
  Vec3 viewGap=a.position-viewer_;
  float viewRange2=viewGap.x*viewGap.x+viewGap.y*viewGap.y+viewGap.z*viewGap.z;
  bool close=!crowded() && a.length*a.length>viewRange2*.0026f;
  if(a.species==Species::Sacabambaspis) {
    // Jawless, and wearing its skeleton on the outside. The head is not a snout but a
    // blunt bony box with a flat face, and the two eyes sit side by side on that face
    // pointing straight forward, which is the whole look of the animal. A slot of a
    // mouth under them, long scales down the body, and a hypocercal tail behind.
    Color boneTop{182,164,120},boneSide{150,134,100},plate{220,208,170},scaleDark{120,114,86};
    struct Hoop { float z,w,h,belly; };
    static const Hoop frame[7]={{.500f,.112f,.062f,.058f},{.430f,.156f,.086f,.070f},
      {.300f,.204f,.112f,.080f},{.140f,.198f,.110f,.074f},{-.020f,.150f,.096f,.062f},
      {-.170f,.088f,.068f,.044f},{-.300f,.034f,.034f,.024f}};
    int facets=close?6:4;
    auto hoop=[&](const Hoop& f,Vec3* out) {
      const float sx6[6]={0,.94f,.86f,0,-.86f,-.94f},sy6[6]={1.0f,.40f,-.70f,-1.0f,-.70f,.40f};
      const float sx4[4]={0,1.0f,0,-1.0f},sy4[4]={1.0f,.10f,-1.0f,.10f};
      const float* sx=facets==6?sx6:sx4;
      const float* sy=facets==6?sy6:sy4;
      for(int k=0;k<facets;++k) out[k]={sx[k]*f.w,sy[k]>0?sy[k]*f.h:sy[k]*f.belly,f.z};
    };
    static const int fine[7]={0,1,2,3,4,5,6},coarse[4]={0,2,4,6};
    const int* hull=close?fine:coarse;
    int hullCount=close?7:4;
    Vec3 a6[6],b6[6];
    hoop(frame[hull[0]],a6);
    for(int k=0;k<facets;++k) tri({0,.004f,.500f},a6[k],a6[(k+1)%facets],k*2<facets?plate:scale(plate,.88f));
    for(int n=1;n<hullCount;++n) {
      int j=hull[n];
      hoop(frame[j],b6);
      bool shield=j<=3;                              // head armour first, then scale rows
      for(int k=0;k<facets;++k) {
        Color c=k==0?(shield?boneTop:scaleDark)
               :k*2<facets?(shield?boneSide:blend(boneSide,scaleDark,.55f))
                   :plate;
        if(!shield && (j+k)%2) c=scale(c,.88f);
        quadS(a6[k],a6[(k+1)%facets],b6[(k+1)%facets],b6[k],c);
      }
      for(int k=0;k<facets;++k) a6[k]=b6[k];
    }
    for(int side=-1;side<=1;side+=2) {
      // Big and round and aimed straight ahead, standing proud of the face.
      // A big pale dome standing well proud of the face, with the dark pupil on the
      // front of it. Six spokes instead of a whole sphere: it is most of the look of
      // the animal for a tenth of the triangles.
      Vec3 eye{side*.060f,.014f,.500f};
      int spokes=close?6:4;
      for(int k=0;k<spokes;++k) {
        float a0=k*2*Pi/spokes,a1=(k+1)*2*Pi/spokes;
        Vec3 r0{std::cos(a0)*.062f,std::sin(a0)*.062f,.004f};
        Vec3 r1{std::cos(a1)*.062f,std::sin(a1)*.062f,.004f};
        tri(eye+Vec3{0,0,.058f},eye+r0,eye+r1,{238,232,210});
      }
      if(close) for(int k=0;k<4;++k) {
        float a0=k*Pi*.5f,a1=(k+1)*Pi*.5f;
        tri(eye+Vec3{0,0,.064f},eye+Vec3{std::cos(a0)*.026f,std::sin(a0)*.026f,.050f},
                                eye+Vec3{std::cos(a1)*.026f,std::sin(a1)*.026f,.050f},{26,24,22});
      }
      else tri(eye+Vec3{-.024f,.020f,.056f},eye+Vec3{.024f,.018f,.056f},eye+Vec3{0,-.026f,.058f},{26,24,22});
    }
    tri({-.062f,-.040f,.492f},{.062f,-.040f,.492f},{0,-.056f,.430f},scale(plate,.84f));   // mouth
    if(close) for(int k=0;k<3;++k) for(int side=-1;side<=1;side+=2)   // gill openings on the shield
      tri({side*.190f,-.024f-k*.028f,.250f},{side*.194f,-.036f-k*.028f,.214f},
          {side*.186f,-.048f-k*.028f,.252f},boneSide);
    {                                                // hypocercal tail, lower lobe leading
      float wag=std::sin(a.phase)*.038f;
      tri({0,.002f,-.300f},{wag,.090f,-.428f},{wag,-.012f,-.402f},boneSide);
      tri({0,-.014f,-.300f},{wag,-.012f,-.402f},{wag,-.178f,-.496f},boneTop);
      tri({wag,-.012f,-.402f},{wag,.090f,-.428f},{wag,-.178f,-.496f},scale(boneSide,.9f));
    }
    return;
  }
  if(a.species==Species::Dunkleosteus) {
    // Armoured head and thorax up front, soft shark-like trunk behind, heterocercal tail.
    // It had no teeth: the jaws carry self-sharpening bony blades, so those are pale plate.
    Color plate{129,137,127},hide{72,95,100},bone{176,167,138};
    auto sway=[&](float z) { return std::sin(a.phase*1.5f+z*5.0f)*.10f*std::max(0.0f,.20f-z); };
    struct Ring { float z,halfW,top,bottom; };
    const Ring body[5]={{.17f,.104f,.124f,.100f},{.06f,.100f,.119f,.094f},{-.07f,.082f,.100f,.078f},
      {-.20f,.055f,.070f,.052f},{-.31f,.028f,.038f,.026f}};
    auto section=[&](const Ring& r,Vec3 out[6]) {
      float x=sway(r.z);
      out[0]={x,r.top,r.z};            out[1]={x+r.halfW,r.top*.42f,r.z};
      out[2]={x+r.halfW*.80f,-r.bottom*.52f,r.z}; out[3]={x,-r.bottom,r.z};
      out[4]={x-r.halfW*.80f,-r.bottom*.52f,r.z}; out[5]={x-r.halfW,r.top*.42f,r.z};
    };
    Vec3 near[6],far[6];
    section(body[0],near);
    for(int k=1;k<5;++k) {
      section(body[k],far);
      for(int e=0;e<6;++e) quad(near[e],near[(e+1)%6],far[(e+1)%6],far[e],
        e==0||e==5?scale(hide,1.18f):e<3?hide:scale(hide,.72f));
      for(int e=0;e<6;++e) near[e]=far[e];
    }
    float tip=sway(-.34f);
    Vec3 stern[3]={{tip,.030f,-.34f},{tip,-.022f,-.34f},{tip,.004f,-.34f}};
    Vec3 upper{tip*1.4f,.155f,-.50f},lower{tip*1.3f,-.080f,-.44f};   // long upper lobe
    quad(stern[0],stern[2],upper,{tip*1.2f,.070f,-.42f},scale(hide,1.1f));
    tri(stern[2],stern[1],lower,hide);
    tri(stern[1],{tip*1.2f,-.020f,-.43f},lower,scale(hide,.78f));
    tri({tip*1.2f,.070f,-.42f},upper,{tip*1.2f,-.020f,-.43f},scale(hide,.9f));
    float dorsal=sway(-.10f);
    tri({dorsal,.098f,-.03f},{dorsal,.100f,-.17f},{dorsal,.178f,-.13f},scale(hide,1.12f));
    tri({sway(-.22f),-.058f,-.19f},{sway(-.26f),-.050f,-.26f},{sway(-.24f),-.115f,-.25f},scale(hide,.8f));
    for(int side=-1;side<=1;side+=2) {
      quad({side*.096f,-.020f,.13f},{side*.140f,-.068f,.02f},{side*.118f,-.098f,-.03f},{side*.080f,-.062f,.08f},scale(hide,.86f));
      quad({side*.062f,-.070f,-.08f},{side*.092f,-.104f,-.15f},{side*.070f,-.108f,-.17f},{side*.052f,-.080f,-.11f},scale(hide,.8f));
    }
    // Head shield: a tall boxy wedge that slopes down to the snout, crested along the back.
    for(int side=-1;side<=1;side+=2) {
      float s=float(side);
      Vec3 A{0,.150f,.18f},B{s*.105f,.116f,.17f},C{s*.100f,-.072f,.17f},D{0,-.104f,.17f};
      Vec3 E{0,.140f,.31f},F{s*.098f,.104f,.31f},G{s*.092f,-.084f,.30f},H{0,-.098f,.30f};
      Vec3 I{0,.084f,.45f},J{s*.042f,.060f,.45f},K{s*.046f,-.046f,.44f},L{0,-.056f,.44f};
      Vec3 M{0,.018f,.50f};
      quad(A,B,F,E,plate); quad(E,F,J,I,scale(plate,1.1f));
      quad(B,C,G,F,scale(plate,.84f)); quad(F,G,K,J,scale(plate,.9f));
      quad(C,D,H,G,scale(plate,.66f)); quad(G,H,L,K,scale(plate,.72f));
      tri(I,J,M,scale(plate,1.06f)); tri(J,K,M,scale(plate,.88f)); tri(K,L,M,scale(plate,.7f));
      // Where the eye goes. This used to be a bare dark rectangle standing in for one,
      // and leaving it behind a round eye only showed its corners. The shield falls away
      // towards the snout at .40 across, so the disc lies at that angle and stands a few
      // thousandths proud of it the whole way round.
      fishEye(tri,{s*.0845f,.031f,.355f},s,.016f,.40f,close);
      tri({s*.052f,-.060f,.408f},{s*.028f,-.064f,.448f},{s*.032f,-.100f,.452f},bone);  // upper blade fang
    }
    quad({-.048f,-.050f,.40f},{.048f,-.050f,.40f},{.040f,-.070f,.462f},{-.040f,-.070f,.462f},bone);
    float open=.019f*(1+std::sin(a.phase*.5f)),jy=-.100f-open;
    quad({-.066f,-.090f,.21f},{.066f,-.090f,.21f},{.038f,jy+.006f,.428f},{-.038f,jy+.006f,.428f},scale(bone,.86f));
    quad({-.038f,jy-.024f,.428f},{.038f,jy-.024f,.428f},{.066f,-.118f,.21f},{-.066f,-.118f,.21f},scale(plate,.62f));
    for(int side=-1;side<=1;side+=2) {
      quad({side*.066f,-.090f,.21f},{side*.038f,jy+.006f,.428f},{side*.038f,jy-.024f,.428f},{side*.066f,-.118f,.21f},scale(plate,.78f));
      tri({side*.028f,jy+.004f,.392f},{side*.028f,jy+.002f,.424f},{side*.025f,jy+.050f,.410f},bone);   // lower fang
    }
    return;
  }
  if(a.species==Species::Plesiosaur) {
    // Plesiosaurus dolichodeirus proportions: small skull, slender neck, broad
    // ribcage, two separate pairs of hydrofoils. Colour and stroke phase are artistic.
    const Color back{68,119,128},flank{112,158,161},belly{205,214,194};
    const int sides=close?10:6;
    auto solid=[&](Vec3 p,Vec3 q,Vec3 r,Color c,Vec3 center){
      addSolid(world(p),world(q),world(r),c,turn((p+q+r)*(1.f/3)-center));
    };
    struct Hoop{float z,w,h,y,x;};
    // One uninterrupted closed loft, from tail tip to the rounded muzzle.
    const float sway=std::sin(a.phase*.24f)*.009f;
    const Hoop rings[]={
      {-.50f,.001f,.001f,0,0},{-.43f,.014f,.013f,0,0},
      {-.35f,.030f,.026f,0,0},{-.29f,.066f,.049f,0,0},
      {-.22f,.103f,.068f,0,0},{-.12f,.114f,.074f,.004f,0},
      {-.035f,.085f,.061f,.007f,0},{.015f,.043f,.038f,.010f,0},
      {.085f,.030f,.028f,.015f,sway*.12f},{.16f,.024f,.023f,.021f,sway*.3f},
      {.24f,.020f,.020f,.027f,sway*.5f},{.32f,.018f,.018f,.032f,sway*.75f},
      {.405f,.016f,.017f,.035f,sway},{.425f,.023f,.024f,.038f,sway},
      {.449f,.026f,.022f,.038f,sway},{.478f,.018f,.016f,.034f,sway},
      {.50f,.011f,.010f,.031f,sway}};
    constexpr int ringCount=sizeof(rings)/sizeof(rings[0]);
    auto point=[&](int j,int k){float t=k*2*Pi/sides;const auto& h=rings[j];return Vec3{h.x+h.w*std::sin(t),h.y+h.h*std::cos(t),h.z};};
    for(int j=0;j<ringCount-1;++j)for(int k=0;k<sides;++k){
      float upness=std::cos((k+.5f)*2*Pi/sides);
      Color c=upness<-.30f?belly:upness>.45f?back:flank;
      Vec3 p=point(j,k),q=point(j,k+1),r=point(j+1,k+1),v=point(j+1,k);
      Vec3 center{(rings[j].x+rings[j+1].x)*.5f,(rings[j].y+rings[j+1].y)*.5f,(rings[j].z+rings[j+1].z)*.5f};
      solid(p,q,r,c,center);solid(p,r,v,c,center);
    }
    for(int k=0;k<sides;++k){
      solid({0,0,-.501f},point(0,k+1),point(0,k),back,{0,0,-.49f});
      solid({sway,.031f,.502f},point(ringCount-1,k),point(ringCount-1,k+1),flank,{sway,.031f,.49f});
    }
    // Eyes on the lateral skull; a fine closed jaw seam, not giant exposed teeth.
    for(int side=-1;side<=1;side+=2){
      Vec3 eye{sway+side*.0255f,.047f,.445f};
      for(int k=0;k<6;++k){float t=k*Pi/3,u=(k+1)*Pi/3;
        tri(eye,eye+Vec3{0,std::sin(t)*.007f,std::cos(t)*.008f},eye+Vec3{0,std::sin(u)*.007f,std::cos(u)*.008f},{16,27,29});
      }
      if(close)tri(eye+Vec3{side*.0004f,.003f,.002f},eye+Vec3{side*.0004f,.003f,.005f},eye+Vec3{side*.0004f,.005f,.003f},{227,230,201});
      quad({sway+side*.024f,.028f,.450f},{sway+side*.0115f,.028f,.500f},
           {sway+side*.0115f,.0265f,.500f},{sway+side*.024f,.0265f,.450f},{48,68,68});
    }
    // Closed lenticular flippers: thick roots, tapering blades with swept rounded tips.
    for(int pair=0;pair<2;++pair)for(int side=-1;side<=1;side+=2){
      float rootZ=pair?-.25f:-.055f,rootX=pair?.065f:.077f;
      float stroke=std::sin(a.phase*.72f-pair*.85f)*.36f*clampf(a.activity,0,1);
      auto fin=[&](float span,float chord,float height){return Vec3{side*(rootX+span*std::cos(stroke)),
        -.025f+span*std::sin(stroke)+height,rootZ+chord};};
      const float outline[8][2]={{0,.039f},{.065f,.055f},{.15f,.009f},{.225f,-.055f},
        {.23f,-.077f},{.204f,-.090f},{.105f,-.060f},{0,-.039f}};
      Vec3 rim[8];for(int k=0;k<8;++k)rim[k]=fin(outline[k][0],outline[k][1],0);
      Vec3 top=fin(.082f,-.009f,.015f),bottom=fin(.082f,-.009f,-.009f),center=fin(.082f,-.009f,0);
      for(int k=0;k<8;++k){solid(top,rim[k],rim[(k+1)%8],k<3?flank:back,center);solid(bottom,rim[(k+1)%8],rim[k],belly,center);}
    }
    return;
  }
  if(a.species==Species::Trilobite) {
    // Named for its three lobes: a raised axis down the middle with a flatter pleural
    // field either side of it. A domed head shield with crescent eyes in front, a run of
    // articulated thoracic segments each ending in a spine, and a fused tail shield.
    Color shellTop{168,142,96},shellSide{132,116,82},rim{190,170,124},eye{44,40,34};
    auto profile=[&](float z) { return .290f*std::sqrt(std::max(.05f,1-z*z*2.6f)); };
    int thorax=close?8:4;
    for(int k=0;k<thorax;++k) {                  // thorax, front to back
      float z0=.230f-k*(.558f/thorax),z1=z0-(.558f/thorax);
      float w0=profile(z0),w1=profile(z1);
      float crawl=std::sin(a.phase*.5f+k*.55f)*.006f;
      float ax0=.052f,ax1=.050f;                 // half width of the raised axis
      float h0=.108f+crawl,h1=.106f+crawl;
      for(int side=-1;side<=1;side+=2) {
        quad({side*ax0,h0,z0},{side*ax1,h1,z1},{side*w1*.86f,h1*.44f,z1},{side*w0*.86f,h0*.44f,z0},
             k%2?shellTop:scale(shellTop,.92f));
        quad({side*w0*.86f,h0*.44f,z0},{side*w1*.86f,h1*.44f,z1},{side*w1,.012f,z1},{side*w0,.012f,z0},shellSide);
        if(close) tri({side*w0,.012f,z0},{side*w1,.012f,z1},{side*(w0+.075f),-.010f,z1-.052f},rim);
      }
      quad({-ax0,h0,z0},{ax0,h0,z0},{ax1,h1,z1},{-ax1,h1,z1},k%2?rim:shellTop);
    }
    {                                            // cephalon: a dome with the glabella on it
      float zc=.250f;
      int wedges=close?6:3;
      for(int k=0;k<wedges;++k) {
        float a0=k*Pi/wedges,a1=(k+1)*Pi/wedges;
        Vec3 p0{-std::cos(a0)*.255f,.014f,zc+std::sin(a0)*.215f};
        Vec3 p1{-std::cos(a1)*.255f,.014f,zc+std::sin(a1)*.215f};
        tri({0,.134f,zc+.010f},p0,p1,k%2?shellTop:scale(shellTop,.9f));
        tri({0,-.006f,zc+.010f},p1,p0,scale(shellSide,.86f));
      }
      quad({-.058f,.104f,zc+.090f},{.058f,.104f,zc+.090f},{.046f,.118f,zc-.040f},{-.046f,.118f,zc-.040f},rim);
      for(int side=-1;side<=1;side+=2)           // crescent eyes, set well out on the cheeks
        tri({side*.086f,.078f,zc+.056f},{side*.124f,.050f,zc+.016f},{side*.082f,.046f,zc+.006f},eye);
      if(close) for(int side=-1;side<=1;side+=2)  // genal spines sweeping back from the corners
        tri({side*.230f,.012f,zc-.140f},{side*.250f,.008f,zc-.170f},{side*(.200f),-.014f,zc-.330f},rim);
    }
    {                                            // pygidium
      float zc=-.360f;
      int wedges=close?5:3;
      for(int k=0;k<wedges;++k) {
        float a0=Pi+k*Pi/wedges,a1=Pi+(k+1)*Pi/wedges;
        Vec3 p0{-std::cos(a0)*.150f,.012f,zc+std::sin(a0)*.130f};
        Vec3 p1{-std::cos(a1)*.150f,.012f,zc+std::sin(a1)*.130f};
        tri({0,.072f,zc+.020f},p0,p1,shellTop);
        tri({0,-.004f,zc+.020f},p1,p0,scale(shellSide,.86f));
      }
    }
    return;
  }
  // Anomalocaris and Opabinia. Both swim on rows of paired side lobes rather than by
  // bending the body, so the lobes carry a wave from the tail forward. What separates
  // them is the head: one has a pair of spined grasping arms and two stalked eyes, the
  // other a single flexible proboscis with a claw on the end and five eyes on top.
  bool op=a.species==Species::Opabinia;
  Color shell=op?Color{172,156,112}:Color{188,116,86};
  Color lobe=op?Color{206,192,150}:Color{222,158,116},dark=scale(shell,.62f);
  int segments=close?9:5;
  auto widthAt=[&](float t) { return (op?.088f:.116f)*(1-t*t*.62f); };
  auto heightAt=[&](float t) { return (op?.050f:.062f)*(1-t*.44f); };
  Vec3 topPrev,botPrev,sidePrev[2];
  for(int k=0;k<=segments;++k) {
    float t=float(k)/segments,z=.300f-t*.560f;
    float w=widthAt(t),h=heightAt(t);
    Vec3 topNow{0,h,z},botNow{0,-h*.72f,z};
    Vec3 sideNow[2]={{w,0,z},{-w,0,z}};
    if(k) {
      for(int side=0;side<2;++side) {
        quad(topPrev,topNow,sideNow[side],sidePrev[side],k%2?shell:scale(shell,1.10f));
        quad(botPrev,sidePrev[side],sideNow[side],botNow,k%2?dark:scale(dark,1.12f));
      }
    }
    topPrev=topNow; botPrev=botNow; sidePrev[0]=sideNow[0]; sidePrev[1]=sideNow[1];
    if(k<segments) {                       // the lobe pair on this segment
      float wave=std::sin(a.phase*1.15f+t*5.2f)*.052f*a.activity;
      float span=(op?.070f:.105f)*(1-t*.30f);
      for(int side=-1;side<=1;side+=2) {
        Vec3 hinge{side*w,0,z},back{side*w,0,z-.062f};
        Vec3 tip{side*(w+span),wave,z-.042f};
        tri(hinge,tip,back,lobe);
        if(close) tri(hinge,{side*(w+span*.55f),wave*.5f,z+.022f},tip,scale(lobe,.9f));
      }
    }
  }
  {                                        // head shield
    float hw=op?.082f:.140f,hh=op?.050f:.070f;
    Vec3 nose{0,op?.010f:.020f,op?.400f:.440f};
    Vec3 ring[4]={{hw,0,.300f},{0,hh,.300f},{-hw,0,.300f},{0,-hh*.8f,.300f}};
    for(int k=0;k<4;++k) tri(nose,ring[k],ring[(k+1)%4],k==1?scale(shell,1.12f):shell);
  }
  int eyes=op?5:2;
  for(int k=0;k<eyes;++k) {
    // Opabinia wears its five eyes in a row on top; Anomalocaris has one out each side.
    float x=op?(k-2)*.042f:(k?-.150f:.150f);
    float y=op?.078f+(k==2?.022f:0):.060f;
    float z=op?.368f:.352f;
    Vec3 root{x*.60f,op?.030f:.020f,z-.030f},ball{x,y,z};
    quad(root+Vec3{.010f,0,0},root-Vec3{.010f,0,0},ball-Vec3{.008f,0,0},ball+Vec3{.008f,0,0},shell);
    if(close) for(int q=0;q<4;++q) {
      float a0=q*Pi*.5f,a1=(q+1)*Pi*.5f;
      tri(ball+Vec3{0,0,.026f},ball+Vec3{std::sin(a0)*.026f,std::cos(a0)*.026f,0},
                               ball+Vec3{std::sin(a1)*.026f,std::cos(a1)*.026f,0},{34,44,40});
    }
    else tri(ball+Vec3{-.024f,.018f,0},ball+Vec3{.024f,.018f,0},ball+Vec3{0,-.026f,.014f},{34,44,40});
  }
  if(op) {
    // The proboscis: five joints hanging forward and down, with a grasping claw on the end.
    Vec3 prev{0,-.020f,.390f};
    float reach=.84f+.16f*std::sin(a.phase*.30f);
    for(int k=1;k<=5;++k) {
      float t=float(k)/5;
      float curl=std::sin(a.phase*.45f+t*2.1f)*.030f;
      Vec3 next{curl*.4f,-.020f-t*.150f*reach+t*t*.070f,.390f+t*.300f*reach};
      float r=.020f-t*.008f;
      quad(prev+Vec3{r,0,0},prev-Vec3{r,0,0},next-Vec3{r*.7f,0,0},next+Vec3{r*.7f,0,0},shell);
      quad(prev+Vec3{0,r,0},prev-Vec3{0,r,0},next-Vec3{0,r*.7f,0},next+Vec3{0,r*.7f,0},scale(shell,.88f));
      prev=next;
    }
    float grip=.026f+.018f*std::sin(a.phase*.7f);
    for(int side=-1;side<=1;side+=2)
      tri(prev,prev+Vec3{side*grip,.020f,.060f},prev+Vec3{side*grip*.5f,-.016f,.058f},dark);
  } else {
    // The two great frontal appendages, each a curl of segments carrying ventral spines.
    for(int side=-1;side<=1;side+=2) {
      Vec3 prev{side*.075f,-.032f,.420f};
      for(int k=1;k<=4;++k) {
        float t=float(k)/4;
        float sweep=std::sin(a.phase*.40f+side*.5f)*.030f*t;
        Vec3 next{side*(.075f-t*.026f)+sweep,-.032f-t*.120f+t*t*.046f,.420f+t*.210f};
        float r=.026f-t*.010f;
        quad(prev+Vec3{0,r,0},prev-Vec3{0,r,0},next-Vec3{0,r*.7f,0},next+Vec3{0,r*.7f,0},shell);
        if(close) quad(prev+Vec3{r,0,0},prev-Vec3{r,0,0},next-Vec3{r*.7f,0,0},next+Vec3{r*.7f,0,0},scale(shell,.86f));
        tri(next,next+Vec3{0,-.052f,-.012f},prev+Vec3{0,-.030f,0},{226,196,150});   // spine
        prev=next;
      }
    }
  }
  {                                        // tail fan: three blades each side
    float wag=std::sin(a.phase*.9f)*.030f*a.activity;
    for(int side=-1;side<=1;side+=2) for(int k=0;k<(close?3:2);++k) {
      float spread=.055f+k*.050f;
      tri({0,0,-.280f},{side*spread,wag*(1+k*.4f),-.470f-k*.020f},
          {side*(spread*.45f),wag*.5f,-.330f},k%2?lobe:scale(lobe,.88f));
    }
  }
}
}
