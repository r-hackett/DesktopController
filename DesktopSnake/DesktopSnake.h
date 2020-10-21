#pragma once

#include "DesktopController.h"
#include "GameObject.h"

#include <vector>
#include <string>
#include <iterator>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <cstdio>

#include <fmt/core.h>

using namespace DcUtil;

// Enable this to play a classic game of snake in which icons are 
// added to the desktop as food when all other icons are consumed.
#if 1
#define ADD_FOOD_ICONS
#endif

class DesktopSnake
{
public:
    // Defaults to icon updates 25 times per second and game steps 1000 times per second. 
    // See iconUpdateInterval and gameStepInterval.
    DesktopSnake(double iconUpdatesPerSecond = 25, double gameStepsPerSecond = 1000);
    ~DesktopSnake();

    // Disable copying.
    DesktopSnake(const DesktopSnake&) = delete;
    void operator=(const DesktopSnake&) = delete;

    void step();

    bool gameOver() const { return isGameOver; }

private:
    using GameObjectVec = std::vector<std::unique_ptr<GameObject>>;

    void handleBoundaries();    // Boundary conditions (reaching edge of screen).
    void handleInput();         // Keyboard input.
    void handleEvents();        // Events that happen periodically.
    void handleCollision();     // Collision (eating food and snake colliding with self).

    void consumeFood(GameObjectVec::iterator foodIt);

    void addChangeDirectionEvent(const GameObjectMoveEvent& event);
    void prepareAndAddChangeDirectionEvent(MoveDirection curDirection, MoveDirection newDirection);

    // Returns true if adding an icon to the back of the snake would be OOB of the desktop.
    bool addingIconWouldGoOOB();

    bool handleKey(long virtualKey);

    MoveDirection oppositeDirection(MoveDirection dir);
    MoveDirection perpendicularClockwiseDirection(MoveDirection dir);

    // Returns the adjacent icon's position given a position and direction (e.g. icon to the right)
    Vec2<double> adjacentIconPositionFromDirection(const Vec2<double>& position, MoveDirection dir);

    GameObjectVec::iterator testObjCollision(const GameObject& obj, GameObjectVec& objList, size_t startIndex = 0);
    bool testAABBIconCollision(const Vec2<int>& objA, const Vec2<int>& objB);

    bool dirChangeWouldCauseCollision();

    void updateIconPositions();

    DesktopController dc;
    GameObjectVec snake;
    GameObjectVec food;
    DcUtil::Vec2<int> deskRes;
    DcUtil::Vec2<int> iconSpacing;
    bool isGameOver;

    // Specifies the frequency of icon (real desktop) positional updates 
    // in seconds (e.g. 0.1 would be 10 times a second).
    double iconUpdateInterval;

    // Specifies the frequency of game logic steps in seconds.
    double gameStepInterval;

#ifdef ADD_FOOD_ICONS
    std::wstring randomFileExtension();
    std::wstring findUniqueDesktopFilename();   // Find a unique filename in the desktop folder with random file extension.
    void addDesktopFoodIcon();

    std::wstring desktopDirPath;
    std::vector<std::wstring> filesAdded;
#endif
};