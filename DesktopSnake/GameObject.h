#pragma once

#include "DesktopController.h"
#include "Util.h"

#include <memory>
#include <deque>
#include <vector>

using namespace DcUtil;

enum class MoveDirection
{
    Static,
    Up,
    Down,
    Left,
    Right
};

// Events can be triggered by various conditions.
enum class EventCondition
{
    More_Or_Equal_X,
    Less_Or_Equal_X,
    More_Or_Equal_Y,
    Less_Or_Equal_Y,
    Never
};

struct GameObjectMoveEvent
{
    GameObjectMoveEvent(MoveDirection dir, EventCondition cond, double val, int boundaryCrossCntTgt = 0)
        : direction(dir)
        , condition(cond)
        , value(val)
        , boundaryCrossCountTarget(boundaryCrossCntTgt)
    {}

    MoveDirection direction;
    EventCondition condition;
    double value;
    int boundaryCrossCountTarget;
};

// Copyable. Everything is on display here. There is no additional logic
// that needs to be added to access these members so they can be public without issue.
struct GameObject
{
    // Note: This constructor takes ownership of deskIcon.
    GameObject(std::unique_ptr<DesktopIcon> deskIcon, MoveDirection dir = MoveDirection::Static, int boundaryCrossCnt = 0);

    void step();
    void setDirection(MoveDirection dir);

    Vec2<double> position;
    Vec2<double> directionVector;
    MoveDirection direction;
    std::unique_ptr<DesktopIcon> icon;

    // Queue of events for changing direction.
    std::deque<GameObjectMoveEvent> changeDirEventsQueue;

    // Events waiting to be added to the queue. 
    // This is used when parts of the snake 
    // have passed through the screen's boundary and other parts haven't, therefore 
    // requiring that events are only added to the queue when the objects further back 
    // have also passed through the boundary. Pending events are added to the event queue
    // when the event's boundaryCrossCountTarget matches a GameObject boundaryCrossCount.
    std::deque<GameObjectMoveEvent> pendingChangeDirEventsQueue;

    // Incremented each time a boundary is crossed.
    int boundaryCrossCount;

    Vec2<double> lastChangeDirPosition;

    double distanceTravelled;
};