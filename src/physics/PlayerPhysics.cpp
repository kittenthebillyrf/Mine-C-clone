#include "PlayerPhysics.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace mc::physics {
namespace {
constexpr double HalfWidth=0.3;
constexpr double Height=1.8;
constexpr double StepHeight=0.6;
struct Box { double min_x,min_y,min_z,max_x,max_y,max_z; };
Box player_box(double x,double y,double z) {
    return {x-HalfWidth,y,z-HalfWidth,x+HalfWidth,y+Height,z+HalfWidth};
}
Box offset(Box b,double x,double y,double z) {
    b.min_x+=x;b.max_x+=x;b.min_y+=y;b.max_y+=y;b.min_z+=z;b.max_z+=z;return b;
}
bool intersects(const Box& a,const Box& b) {
    return a.max_x>b.min_x&&a.min_x<b.max_x&&a.max_y>b.min_y&&a.min_y<b.max_y&&a.max_z>b.min_z&&a.min_z<b.max_z;
}
bool slab(world::BlockId id) {
    using world::BlockId;
    return id==BlockId::OakSlab||id==BlockId::SpruceSlab||id==BlockId::BirchSlab||
           id==BlockId::JungleSlab||id==BlockId::AcaciaSlab||id==BlockId::DarkOakSlab;
}
bool stairs(world::BlockId id) {
    using world::BlockId;
    return id==BlockId::OakStairs||id==BlockId::SpruceStairs||id==BlockId::BirchStairs||
           id==BlockId::JungleStairs||id==BlockId::AcaciaStairs||id==BlockId::DarkOakStairs;
}
bool fence(world::BlockId id) {
    using world::BlockId;
    return id==BlockId::OakFence||id==BlockId::SpruceFence||id==BlockId::BirchFence||
           id==BlockId::JungleFence||id==BlockId::AcaciaFence||id==BlockId::DarkOakFence;
}
bool connects(world::BlockId id) {
    using world::BlockId;
    return id!=BlockId::Air&&id!=BlockId::Water&&id!=BlockId::Lava&&id!=BlockId::Glass&&
           id!=BlockId::Ice&&id!=BlockId::Torch&&id!=BlockId::Cactus&&!slab(id)&&
           (world::block_info(id).solid||fence(id));
}
std::vector<Box> shapes(const world::World& w,const Box& q) {
    std::vector<Box> out;
    const int x0=int(std::floor(q.min_x)),x1=int(std::floor(q.max_x));
    const int y0=std::max(0,int(std::floor(q.min_y))),y1=std::min(world::Chunk::Height-1,int(std::floor(q.max_y)));
    const int z0=int(std::floor(q.min_z)),z1=int(std::floor(q.max_z));
    for(int y=y0;y<=y1;++y)for(int z=z0;z<=z1;++z)for(int x=x0;x<=x1;++x) {
        const auto id=w.get_block(x,y,z);
        if(id==world::BlockId::Air||!world::block_info(id).solid)continue;
        auto add=[&](Box b){if(intersects(b,q))out.push_back(b);};
        if(slab(id)){add({double(x),double(y),double(z),double(x+1),double(y)+0.5,double(z+1)});continue;}
        if(stairs(id)) {
            const auto* s=w.get_block_state(x,y,z);const int face=s?int(s->facing&3u):2;
            const bool upside=s&&(s->shape&1u);
            const double low=upside?double(y)+0.5:double(y),high=upside?double(y):double(y)+0.5;
            add({double(x),low,double(z),double(x+1),low+0.5,double(z+1)});
            Box half{double(x),high,double(z),double(x+1),high+0.5,double(z+1)};
            if(face==0)half.max_z=double(z)+0.5;else if(face==1)half.min_x=double(x)+0.5;
            else if(face==2)half.min_z=double(z)+0.5;else half.max_x=double(x)+0.5;
            add(half);continue;
        }
        if(fence(id)) {
            add({x+0.375,double(y),z+0.375,x+0.625,double(y)+1.5,z+0.625});
            const bool n=connects(w.get_block(x,y,z-1)),s=connects(w.get_block(x,y,z+1));
            const bool west=connects(w.get_block(x-1,y,z)),east=connects(w.get_block(x+1,y,z));
            for(const auto rail: {std::pair{0.375,0.9375},std::pair{1.0,1.5}}) {
                if(west)add({double(x),y+rail.first,z+0.375,x+0.5,y+rail.second,z+0.625});
                if(east)add({x+0.5,y+rail.first,z+0.375,double(x+1),y+rail.second,z+0.625});
                if(n)add({x+0.375,y+rail.first,double(z),x+0.625,y+rail.second,z+0.5});
                if(s)add({x+0.375,y+rail.first,z+0.5,x+0.625,y+rail.second,double(z+1)});
            }continue;
        }
        add({double(x),double(y),double(z),double(x+1),double(y+1),double(z+1)});
    }
    return out;
}
bool collides(const world::World& w,const Box& b){return !shapes(w,b).empty();}
double clip_axis(const Box& obstacle,const Box& moving,double amount,int axis) {
    if(axis==1) {
        if(obstacle.max_x>moving.min_x&&obstacle.min_x<moving.max_x&&obstacle.max_z>moving.min_z&&obstacle.min_z<moving.max_z) {
            if(amount>0&&moving.max_y<=obstacle.min_y)amount=std::min(amount,obstacle.min_y-moving.max_y);
            else if(amount<0&&moving.min_y>=obstacle.max_y)amount=std::max(amount,obstacle.max_y-moving.min_y);
        }
    } else if(axis==0) {
        if(obstacle.max_y>moving.min_y&&obstacle.min_y<moving.max_y&&obstacle.max_z>moving.min_z&&obstacle.min_z<moving.max_z) {
            if(amount>0&&moving.max_x<=obstacle.min_x)amount=std::min(amount,obstacle.min_x-moving.max_x);
            else if(amount<0&&moving.min_x>=obstacle.max_x)amount=std::max(amount,obstacle.max_x-moving.min_x);
        }
    } else {
        if(obstacle.max_x>moving.min_x&&obstacle.min_x<moving.max_x&&obstacle.max_y>moving.min_y&&obstacle.min_y<moving.max_y) {
            if(amount>0&&moving.max_z<=obstacle.min_z)amount=std::min(amount,obstacle.min_z-moving.max_z);
            else if(amount<0&&moving.min_z>=obstacle.max_z)amount=std::max(amount,obstacle.max_z-moving.min_z);
        }
    }
    return amount;
}
}
bool player_box_clear(const world::World& w,double x,double y,double z){return !collides(w,player_box(x,y,z));}
bool move_player(const world::World& w,double& x,double& y,double& z,double& dx,double& dy,double& dz,
                 bool sneaking,bool was_on_ground,bool& hit_h,bool& hit_v,bool& on_ground) {
    hit_h=false;hit_v=false;on_ground=false;
    Box box=player_box(x,y,z);
    if(collides(w,box)) {
        bool recovered=false;
        for(double lift=0.05;lift<=4.0001;lift+=0.05)if(!collides(w,offset(box,0,lift,0))){box=offset(box,0,lift,0);y+=lift;recovered=true;break;}
        if(!recovered){dx=dy=dz=0;hit_h=hit_v=true;return false;}
    }
    if(sneaking&&was_on_ground) {
        auto trim=[&](double& amount,double ox,double oz){while(std::abs(amount)>1e-8&&collides(w,offset(box,ox,-StepHeight,oz)))amount-=std::copysign(std::min(0.05,std::abs(amount)),amount);};
        trim(dx,dx,0);trim(dz,0,dz);
    }
    const double original_y=dy,original_x=dx,original_z=dz;
    const double max_delta=std::max({std::abs(dx),std::abs(dy),std::abs(dz)});
    const int steps=std::max(1,int(std::ceil(max_delta/0.1)));
    const double sx=dx/steps,sy=dy/steps,sz=dz/steps;
    dx=dy=dz=0;
    for(int i=0;i<steps;++i) {
        auto obs=shapes(w,offset(box,sx,sy,sz));
        double ay=sy;for(const auto& o:obs)ay=clip_axis(o,box,ay,1);
        box=offset(box,0,ay,0);dy+=ay;if(std::abs(ay-sy)>1e-9)hit_v=true;
        obs=shapes(w,offset(box,sx,0,0));
        double ax=sx;for(const auto& o:obs)ax=clip_axis(o,box,ax,0);
        if(std::abs(ax-sx)>1e-9)hit_h=true;
        box=offset(box,ax,0,0);dx+=ax;
        obs=shapes(w,offset(box,0,0,sz));
        double az=sz;for(const auto& o:obs)az=clip_axis(o,box,az,2);
        if(std::abs(az-sz)>1e-9)hit_h=true;
        box=offset(box,0,0,az);dz+=az;
    }
    // Try the vanilla-style 0.6-block step path when grounded horizontal
    // movement is blocked. Keep it only when it makes more horizontal progress.
    if(hit_h && was_on_ground && (std::abs(original_x)>1e-9 || std::abs(original_z)>1e-9)) {
        const Box normal_box=box;
        const double normal_dx=dx,normal_dz=dz,normal_dy=dy;
        Box candidate=player_box(x,y,z);
        auto step_obstacles=shapes(w,offset(candidate,original_x,StepHeight,original_z));
        double up=StepHeight;
        for(const auto& o:step_obstacles)up=clip_axis(o,candidate,up,1);
        candidate=offset(candidate,0,up,0);
        double step_x=original_x;
        step_obstacles=shapes(w,offset(candidate,step_x,0,0));
        for(const auto& o:step_obstacles)step_x=clip_axis(o,candidate,step_x,0);
        candidate=offset(candidate,step_x,0,0);
        double step_z=original_z;
        step_obstacles=shapes(w,offset(candidate,0,0,step_z));
        for(const auto& o:step_obstacles)step_z=clip_axis(o,candidate,step_z,2);
        candidate=offset(candidate,0,0,step_z);
        auto down_obstacles=shapes(w,offset(candidate,0,-up,0));
        double down=-up;
        for(const auto& o:down_obstacles)down=clip_axis(o,candidate,down,1);
        candidate=offset(candidate,0,down,0);
        const double normal_distance=normal_dx*normal_dx+normal_dz*normal_dz;
        const double step_distance=step_x*step_x+step_z*step_z;
        if(step_distance>normal_distance+1e-10 && !collides(w,candidate)) {
            box=candidate;dx=step_x;dz=step_z;dy=up+down;
            hit_h=false;hit_v=(std::abs(down+up)>1e-9);on_ground=hit_v;
        } else {
            box=normal_box;dx=normal_dx;dy=normal_dy;dz=normal_dz;
        }
    }
    // A final invariant check prevents committing any penetrating position.
    if(collides(w,box)){dx=dy=dz=0;hit_h=hit_v=true;return false;}
    x=(box.min_x+box.max_x)*0.5;y=box.min_y;z=(box.min_z+box.max_z)*0.5;
    if(!on_ground)on_ground=hit_v&&original_y<0;
    return true;
}
}
