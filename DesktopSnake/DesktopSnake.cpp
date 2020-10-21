#include "DesktopSnake.h"
#include "GameObject.h"
#include "Util.h"

#include <unordered_map>
#include <algorithm>
#include <fstream>

using std::string;
using std::wstring;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::runtime_error;
using std::shared_ptr;
using std::make_shared;
using std::unordered_map;
using namespace std::chrono;

using namespace DeskCtrlUtil;

DesktopSnake::DesktopSnake(double iconUpdatesPerSecond, double gameStepsPerSecond)
    : isGameOver(false)
{
#ifdef ADD_FOOD_ICONS
    desktopDirPath = DeskCtrlUtil::desktopDirectory();
    fmt::print("Desktop directory: {}\n", wstringToOem(desktopDirPath));
#endif

    if (iconUpdatesPerSecond <= 0.0)
        throw runtime_error("iconUpdatesPerSecond must be more than 0");
    if (gameStepsPerSecond <= 0.0)
        throw runtime_error("gameStepsPerSecond must be more than 0");

    // Intervals are preferred to hertz.
    iconUpdateInterval = 1.0 / iconUpdatesPerSecond;
    gameStepInterval = 1.0 / gameStepsPerSecond;

    FolderFlags flags = dc.folderFlags();

    // Snap to grid implies small (less than the size of the icon)
    // movements will be ignored.
    if (flags.snapToGrid)
        throw runtime_error("Snap to Grid should be disabled.");

    // Autoarrange freezes the icons in place.
    if (flags.autoArrange)
        throw runtime_error("Auto arrange should be disabled.");

    // Retrieve pointers to all the icons on the desktop.
    vector<shared_ptr<DesktopIcon>> icons = dc.allIcons();
    if (icons.empty())
        throw runtime_error("No icons on desktop.");

    // Select random icon for the snake's head.
    int snakeHeadIndex = randomInt(0, static_cast<int>(icons.size() - 1));
    shared_ptr<DesktopIcon> snakeHeadIcon = icons[snakeHeadIndex];
    fmt::print("Snake head: {}\n", wstringToOem(snakeHeadIcon->displayName()));

    // snake[0] shall be the head.
    snake.push_back(make_shared<GameObject>(snakeHeadIcon, MoveDirection::Static));

    // Treat all the other icons as food. 
    for (size_t i = 0; i < icons.size(); ++i)
    {
        if (i == snakeHeadIndex)
            continue;

        food.push_back(make_shared<GameObject>(icons[i], MoveDirection::Static));

        fmt::print("Food: {}\n", wstringToOem(icons[i]->displayName()));
    }

    deskRes = dc.desktopResolution();
    iconSpacing = dc.iconSpacing();
}

DesktopSnake::~DesktopSnake()
{
#ifdef ADD_FOOD_ICONS
    for (auto& file : filesAdded)
    {
        wstring path = desktopDirPath + L"\\" + file;

        if (!DeleteFileW(path.c_str()))
            fmt::print("Failed to delete added file: {}", wstringToOem(path));
        else
            fmt::print("Deleted added file: {}\n", wstringToOem(path));
    }
#endif
}

void DesktopSnake::handleBoundaries()
{
    for (auto& snakeObj : snake)
    {
        Vec2<int> iconPt = snakeObj->position;

        long top = iconPt.y;
        long bottom = iconPt.y + iconSpacing.y;
        long left = iconPt.x;
        long right = iconPt.x + iconSpacing.x;

        bool boundaryCross = false;

        if (top <= 0)
        {
            snakeObj->position.y = static_cast<double>(static_cast<int64_t>(deskRes.y) - iconSpacing.y) - 1.0;
            boundaryCross = true;
        }
        if (bottom >= deskRes.y)
        {
            snakeObj->position.y = 1.0;
            boundaryCross = true;
        }
        if (left <= 0)
        {
            snakeObj->position.x = static_cast<double>(static_cast<int64_t>(deskRes.x) - iconSpacing.x) - 1.0;
            boundaryCross = true;
        }
        if (right >= deskRes.x)
        {
            snakeObj->position.x = 1.0;
            boundaryCross = true;
        }

        // We just crossed a boundary. Check if there are any pending events to flush to queue.
        if (boundaryCross)
        {
            snakeObj->boundaryCrossCount++;

            auto eventIt = snakeObj->pendingChangeDirEventsQueue.begin();
            while (eventIt != snakeObj->pendingChangeDirEventsQueue.end())
            {
                // Each event has a target boundary cross count which must match the current boundary
                // cross count before the event is active (placed in to the eventQueue and removed from
                // the pendingEvent queue). The unusual way of incrementing here is necessary because
                // the vector we're iterating over is modified in the loop.
                if (snakeObj->boundaryCrossCount == eventIt->boundaryCrossCountTarget)
                {
                    snakeObj->changeDirEventsQueue.push_back(*eventIt);
                    eventIt = snakeObj->pendingChangeDirEventsQueue.erase(eventIt);
                }
                else
                {
                    eventIt++;
                }
            }
        }
    }
}

MoveDirection DesktopSnake::oppositeDirection(MoveDirection dir)
{
    switch (dir)
    {
    case MoveDirection::Up: return MoveDirection::Down;
    case MoveDirection::Down: return MoveDirection::Up;
    case MoveDirection::Left: return MoveDirection::Right;
    case MoveDirection::Right: return MoveDirection::Left;
    case MoveDirection::Static:
    default:
        return MoveDirection::Static;
    }
}

MoveDirection DesktopSnake::perpendicularClockwiseDirection(MoveDirection dir)
{
    switch (dir)
    {
    case MoveDirection::Up: return MoveDirection::Right;
    case MoveDirection::Down: return MoveDirection::Left;
    case MoveDirection::Left: return MoveDirection::Up;
    case MoveDirection::Right: return MoveDirection::Down;
    case MoveDirection::Static:
    default:
        return MoveDirection::Static;
    }
}

void DesktopSnake::prepareAndAddChangeDirectionEvent(MoveDirection curDirection, MoveDirection newDirection)
{
    // This maps the snake head's movement direction to a condition which must
    // be met before icons that make up the snake body change direction.
    // For example, if the head of the snake is moving Up and wants to 
    // move Right, any icon on the body will need to have a y position which is
    // less than or equal (Less_Or_Equal_Y) to the y coordinate of the snake head 
    // when it changes direction.
    static unordered_map<MoveDirection, EventCondition> moveDirToEvtCond{
        {MoveDirection::Right, EventCondition::More_Or_Equal_X},
        {MoveDirection::Left, EventCondition::Less_Or_Equal_X},
        {MoveDirection::Up, EventCondition::Less_Or_Equal_Y},
        {MoveDirection::Down, EventCondition::More_Or_Equal_Y},
    };

    EventCondition condition = moveDirToEvtCond[curDirection];
    double eventValue = 0.0;

    // Now we can add move events to the snake's body.
    // TODO: Use a map.
    switch (newDirection)
    {
    case MoveDirection::Up:
    case MoveDirection::Down:
        eventValue = snake[0]->position.x;
        break;

    case MoveDirection::Right:
    case MoveDirection::Left:
        eventValue = snake[0]->position.y;
        break;
    }

    // Add the event to all icons in the snake's body.
    addChangeDirectionEvent(GameObjectMoveEvent(newDirection, condition, eventValue, snake[0]->boundaryCrossCount));
}

bool DesktopSnake::handleKey(long virtualKey)
{
    if (!GetAsyncKeyState(virtualKey))
        return false;

    // At this point we know the key is pressed, whatever it is.
    // Now we can figure out what action we should take, if any.
    static unordered_map<long, MoveDirection> vkKeyToMoveDir{
        {VK_UP, MoveDirection::Up},
        {VK_DOWN, MoveDirection::Down},
        {VK_LEFT, MoveDirection::Left},
        {VK_RIGHT, MoveDirection::Right},
    };

    // Check if this virtual key is of interest.
    if (vkKeyToMoveDir.count(virtualKey) == 0)
        return false;

    // Yes it is. Find the associated movement direction.
    MoveDirection newDirection = vkKeyToMoveDir[virtualKey];

    // Store the current direction.
    MoveDirection curDirection = snake[0]->direction;

    // If the snake is already moving in the direction requested or the opposite of it, do nothing.
    // This is to prevent the snake going back on itself as well as avoiding adding redundant events.
    if (newDirection == curDirection || newDirection == oppositeDirection(curDirection))
        return false;

    // After checking for bogus directions we can set the direction of the snake head.
    snake[0]->setDirection(newDirection);

    // The icon might be static at the start of the game. In that case it will not have a 
    // body so no need to add any events. 
    if (curDirection == MoveDirection::Static)
        return true;

    // At this point we should be able to guarantee that the new direction is perpendicular to the current
    // direction but this sanity check will make sure. 
    MoveDirection perp = perpendicularClockwiseDirection(curDirection);
    if (newDirection != perp && newDirection != oppositeDirection(perp))
        throw runtime_error("Movement direction change should be perpendicular.");

    // Finally add the change direction event to the body.
    prepareAndAddChangeDirectionEvent(curDirection, newDirection);

    return true;
}

bool DesktopSnake::dirChangeWouldCauseCollision()
{
    switch (snake[0]->direction)
    {
    case MoveDirection::Up:
    case MoveDirection::Down:
        if (abs(snake[0]->lastChangeDirPosition.y - snake[0]->position.y) <= iconSpacing.y)
            return true;
        break;

    case MoveDirection::Left:
    case MoveDirection::Right:
        if (abs(snake[0]->lastChangeDirPosition.x - snake[0]->position.x) <= iconSpacing.x)
            return true;
        break;
    }
    return false;
}

void DesktopSnake::handleInput()
{
    // This blocks any input that would cause the snake to crash in to itself 
    // when its changes direction.
    if (dirChangeWouldCauseCollision())
        return;

    // Only handle one key press at a time.
    // TODO:: Refactor.
    if (handleKey(VK_UP))
        return;
    if (handleKey(VK_DOWN))
        return;
    if (handleKey(VK_LEFT))
        return;
    if (handleKey(VK_RIGHT))
        return;
}

void DesktopSnake::handleEvents()
{
    // Test change direction events.
    for (size_t i = 1; i < snake.size(); ++i)
    {
        if (snake[i]->changeDirEventsQueue.empty())
            continue;

        GameObjectMoveEvent event = snake[i]->changeDirEventsQueue.front();

        bool conditionMet = false;

        switch (event.condition)
        {
        case EventCondition::More_Or_Equal_X: conditionMet = (snake[i]->position.x >= event.value); break;
        case EventCondition::Less_Or_Equal_X: conditionMet = (snake[i]->position.x <= event.value); break;
        case EventCondition::More_Or_Equal_Y: conditionMet = (snake[i]->position.y >= event.value); break;
        case EventCondition::Less_Or_Equal_Y: conditionMet = (snake[i]->position.y <= event.value); break;
        case EventCondition::Never: break;
        }

        if (conditionMet)
        {
            snake[i]->changeDirEventsQueue.pop_front();
            snake[i]->setDirection(event.direction);
        }
    }
}

vector<shared_ptr<GameObject>>::iterator DesktopSnake::testObjCollision(
    const GameObject& obj, 
    vector<shared_ptr<GameObject>>& objList, 
    size_t startIndex)
{
    // If the start index is past the end of the object list just return end().
    if (startIndex > objList.size() - 1)
        return objList.end();

    Vec2<int> objPoint = snake[0]->position;

    auto listIt = objList.begin();
    advance(listIt, startIndex);
    for (;listIt != objList.end(); ++listIt)
    {
        Vec2<int> listObjPt = (*listIt)->position;

        // AABB collision test.
        if (objPoint.x < listObjPt.x + iconSpacing.x && // obj left < list obj right
            objPoint.x + iconSpacing.x > listObjPt.x && // obj right > list ibj left
            objPoint.y < listObjPt.y + iconSpacing.y && // obj top < list obj bottom
            objPoint.y + iconSpacing.y > listObjPt.y)   // obj bottom > list obj top
        {
            return listIt;
        }
    }

    return objList.end();
}

// Returns true if adding an icon to the back of the snake would be OOB of the desktop.
bool DesktopSnake::addingIconWouldGoOOB()
{
    auto& tail = snake.back();
    bool oob = false;

    switch (tail->direction)
    {
    case MoveDirection::Up:
        oob = (tail->position.y + static_cast<int64_t>(iconSpacing.y) * 2 >= deskRes.y - 1.0);
        break;
    case MoveDirection::Down:
        oob = (tail->position.y - iconSpacing.y <= 1.0);
        break;
    case MoveDirection::Left:
        oob = (tail->position.x + static_cast<int64_t>(iconSpacing.x) * 2 >= deskRes.x - 1.0);
        break;
    case MoveDirection::Right:
        oob = (tail->position.x - iconSpacing.x <= 1.0);
        break;
    }

    return oob;
}

#ifdef ADD_FOOD_ICONS
wstring DesktopSnake::randomFileExtension()
{
    static const vector<wstring> extensions{
        L".exe", L".dll", L".doc", L".docx", L".mp3", 
        L".pdf", L".bmp", L".avi", L".jpg", L".msc"};
    return extensions[randomInt(0, static_cast<int>(extensions.size() - 1))];
}

// Find a unique filename in the desktop folder with random file extension.
std::wstring DesktopSnake::findUniqueDesktopFilename()
{
    wstring fileName;
    bool exists;

    // Just a simple counter which will make up the filename.
    static int lastUniqueFileIndex = 0;

    do
    {
        fileName = std::to_wstring(++lastUniqueFileIndex) + randomFileExtension();

        exists = (ifstream(desktopDirPath + L'\\' + fileName) ? true : false);
        if (exists)
            fmt::print("'{}' exists already\n", wstringToOem(fileName));

    } while (exists);

    return fileName;
}

void DesktopSnake::addDesktopFoodIcon()
{
    // Find a unique filename.
    wstring foodFileName = findUniqueDesktopFilename();
    wstring foodFilePath = desktopDirPath + L'\\' + foodFileName;

    // Add the file the desktop folder.
    ofstream file(foodFilePath.c_str());
    if (!file.is_open())
        throw runtime_error("Failed to create file " + wstringToOem(foodFileName));
    file.close();

    // Tell windows to refresh the desktop while we keep attempting to get a pointer.
    shared_ptr<DesktopIcon> icon;
    do
    {
        dc.refresh();
        icon = dc.iconByName(foodFileName);
        std::this_thread::sleep_for(microseconds(10));
    } while (!icon);

    // Set the icon to a random initial position.
    icon->reposition(
        Vec2<int>(
            randomInt(0, deskRes.x - iconSpacing.x),
            randomInt(0, deskRes.y - iconSpacing.y)
        ));

    // Add the icon to the food vector.
    food.push_back(make_shared<GameObject>(icon, MoveDirection::Static));

    // Store the filename so we can delete the file later.
    filesAdded.push_back(foodFileName);

    fmt::print("New food added: '{}'\n", wstringToOem(foodFileName));
}
#endif

void DesktopSnake::handleCollision()
{
    // Don't test for collision if the snake isn't moving yet. Doing so causes
    // the snake's body to copy the MoveDirection of the head and hence won't move.
    if (snake[0]->direction == MoveDirection::Static)
        return;

    // Test collision for food objects. 
    auto foodIt = testObjCollision(*snake[0].get(), food);
    if (foodIt != food.end())
    {
        if (!addingIconWouldGoOOB())
        {
            consumeFood(foodIt);

#ifdef ADD_FOOD_ICONS
            if (food.empty())
            {
                addDesktopFoodIcon();
            }
#endif
        }
    }

    // Only consider elements starting from index 3 when testing collision.
    // This is to avoid collision with the second snake body part when changing direction.
    auto snakeBodyIt = testObjCollision(*snake[0].get(), snake, 3);
    if (snakeBodyIt != snake.end())
    {
        fmt::print("Game over (collision with {})\n", wstringToOem((*snakeBodyIt)->icon->displayName()));
        isGameOver = true;
    }
}

Vec2<double> DesktopSnake::adjacentIconPositionFromDirection(const Vec2<double>& position, MoveDirection dir)
{
    switch (dir)
    {
    case MoveDirection::Up: return Vec2<double>(position.x, position.y + iconSpacing.y);
    case MoveDirection::Down: return Vec2<double>(position.x, position.y - iconSpacing.y);
    case MoveDirection::Left: return Vec2<double>(position.x + iconSpacing.x, position.y);
    case MoveDirection::Right: return Vec2<double>(position.x - iconSpacing.x, position.y);
    case MoveDirection::Static: 
    default:
        return position;
    }
}

void DesktopSnake::consumeFood(vector<shared_ptr<GameObject>>::iterator foodIt)
{
    shared_ptr<GameObject> foodObj = *foodIt;
    food.erase(foodIt);

    // Get movement direction and position of the snake's (old) tail 
    // before adding a new tail.
    MoveDirection tailDirection = snake.back()->direction;
    Vec2<double> tailPosition = snake.back()->position;

    snake.push_back(foodObj);

    shared_ptr<GameObject>& tail = snake.back();

    // Since this object was moved, the constructor wasn't called so the direction 
    // and position need to be set from here. 
    tail->setDirection(tailDirection);
    tail->position = adjacentIconPositionFromDirection(tailPosition, tailDirection);

    // A new object has been added but it's event queue is empty so we need to update
    // it with the events from the icon ahead of it. 
    shared_ptr<GameObject>& secondToLastObj = snake.end()[-2];
    tail->changeDirEventsQueue = secondToLastObj->changeDirEventsQueue;
    tail->pendingChangeDirEventsQueue = secondToLastObj->pendingChangeDirEventsQueue;
    tail->boundaryCrossCount = secondToLastObj->boundaryCrossCount;
}

void DesktopSnake::addChangeDirectionEvent(const GameObjectMoveEvent& event)
{
    if (snake.size() < 2)
        return;

    // Add event to all GameObjects that represent the snake's body. 
    // The snake head GameObject doesn't require this since it changes 
    // direction immediately when requested (hence we start from 1).
    for (size_t i = 1; i < snake.size(); ++i)
    {
        // If the boundary cross count of this object does not equal the snake head's
        // count, this means that the snake head has crossed more boundaries than this
        // object, so this event should not be active. In that case, it's added to the
        // pending events queue.
        if (snake[i]->boundaryCrossCount != snake[0]->boundaryCrossCount)
        {
            snake[i]->pendingChangeDirEventsQueue.push_back(event);
        }
        else
        {
            snake[i]->changeDirEventsQueue.push_back(event);
        }
    }
}

template<typename T>
static long long durationCount(const time_point<high_resolution_clock>& start)
{
    return duration_cast<T>(steady_clock::now() - start).count();
}

void DesktopSnake::step()
{
    static auto updateIconStart = steady_clock::now();
    static auto stepStart = steady_clock::now();
    static auto debugStart = steady_clock::now();

    const long long nanosecondsPerSecond = static_cast<long long>(1e9);

    // Check if it's time to update desktop icon positions.
    // This is a slow operation so should be done infrequently compared to game logic updates.
    if (durationCount<nanoseconds>(updateIconStart) >= nanosecondsPerSecond * iconUpdateInterval)
    {
        updateIconPositions();
        updateIconStart = steady_clock::now();
    }

    // Check if it's time to step the game logic. This can be done much more frequently than the above.
    if (durationCount<nanoseconds>(stepStart) >= nanosecondsPerSecond * gameStepInterval)
    {
        handleInput();
        handleBoundaries();
        handleEvents();
        handleCollision();

        // Step the snake.
        for (auto& icon : snake)
            icon->step();

        stepStart = steady_clock::now();
    }

    // Debug timer.
    if (durationCount<nanoseconds>(debugStart) >= nanosecondsPerSecond)
    {
        static int i = 0;
       // fmt::print("blah: {}\n", i++);
        debugStart = steady_clock::now();
    }
}

void DesktopSnake::updateIconPositions()
{
    vector<shared_ptr<DesktopIcon>> icons;
    vector<Vec2<int>> points;
    for (auto& snakeObj : snake)
    {
        icons.push_back(snakeObj->icon);
        points.push_back(Vec2<int>(snakeObj->position));
    }

    dc.repositionIcons(icons, points);
}
