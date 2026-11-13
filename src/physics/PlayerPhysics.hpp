#pragma once
#include "../world/World.hpp"

namespace mc::physics {
bool player_box_clear(const world::World& world,double x,double feet_y,double z);
bool move_player(const world::World& world,double& x,double& feet_y,double& z,
                 double& motion_x,double& motion_y,double& motion_z,
                 bool sneaking,bool was_on_ground,
                 bool& collided_horizontally,bool& collided_vertically,bool& on_ground);
}
