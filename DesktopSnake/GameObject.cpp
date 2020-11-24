#include "GameObject.h"

using namespace std;

GameObject::GameObject(unique_ptr<DesktopIcon> deskIcon, MoveDirection dir, int boundaryCrossCnt)
    : directionVector(Vec2<double>(0.0, 0.0))
    , icon(std::move(deskIcon))
    , boundaryCrossCount(boundaryCrossCnt)
    , distanceTravelled(0.0)
{
    setDirection(dir);
    position = Vec2<double>(icon->position());
    lastChangeDirPosition = position;
}

void GameObject::step()
{
    double velocity = 0.8;

    if (direction != MoveDirection::Static)
        distanceTravelled += velocity;

    position += directionVector * velocity;
}

void GameObject::setDirection(MoveDirection dir)
{
    lastChangeDirPosition = position;
    direction = dir;

    // Velocity must be set to 1 in either x or y here.
    switch (direction)
    {
    case MoveDirection::Up:     directionVector = Vec2<double>(0.0, -1.0); break;
    case MoveDirection::Down:   directionVector = Vec2<double>(0.0, 1.0);  break;
    case MoveDirection::Left:   directionVector = Vec2<double>(-1.0, 0.0); break;
    case MoveDirection::Right:  directionVector = Vec2<double>(1.0, 0.0);  break;
    case MoveDirection::Static: directionVector = Vec2<double>(0.0, 0.0);  break;
    }
}
