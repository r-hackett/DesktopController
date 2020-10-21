#include "DesktopSnake.h"
#include "Util.h"

#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <random>

using std::string;
using std::wstring;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::runtime_error;
using std::unique_ptr;
using std::make_unique;
using std::unordered_map;
using namespace std::chrono;

using namespace DcUtil;

static int randomInt(int min, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(min, max);
    return distr(gen);
}

DesktopSnake::DesktopSnake(double iconUpdatesPerSecond, double gameStepsPerSecond)
    : isGameOver(false)
{
#ifdef ADD_FOOD_ICONS
    desktopDirPath = DcUtil::desktopDirectory();
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

    // If snap to grid is enabled, movements less than the size of an icon will be ignored.
    if (flags.snapToGrid)
        throw runtime_error("Snap to Grid should be disabled.");

    // Autoarrange freezes the icons in place.
    if (flags.autoArrange)
        throw runtime_error("Auto arrange should be disabled.");

    vector<unique_ptr<DesktopIcon>> icons = dc.allIcons();
    if (icons.empty())
        throw runtime_error("No icons on desktop.");

    int snakeHeadIndex = randomInt(0, static_cast<int>(icons.size() - 1));
    fmt::print("Snake head: {}\n", wstringToOem(icons[snakeHeadIndex]->displayName()));

    // snake[0] shall be the head.
    snake.push_back(make_unique<GameObject>(std::move(icons[snakeHeadIndex]), MoveDirection::Static));

    // Treat all the other icons as food. 
    for (size_t i = 0; i < icons.size(); ++i)
    {
        if (i == snakeHeadIndex)
            continue;

        fmt::print("Food: {}\n", wstringToOem(icons[i]->displayName()));
        food.push_back(make_unique<GameObject>(std::move(icons[i])));
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

        if (boundaryCross)
        {
            snakeObj->boundaryCrossCount++;

            auto& pendEvt = snakeObj->pendingChangeDirEventsQueue;

            // The unusual way of incrementing eventIt here is necessary because
            // the vector being iterating over is modified in the loop.
            for (auto eventIt = pendEvt.begin(); eventIt != pendEvt.end(); )
            {
                if (snakeObj->boundaryCrossCount == eventIt->boundaryCrossCountTarget)
                {
                    snakeObj->changeDirEventsQueue.push_back(*eventIt);
                    eventIt = pendEvt.erase(eventIt);
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
    // Maps the snake head's movement direction to a condition which must
    // be met before icons that make up the snake body change direction.
    static unordered_map<MoveDirection, EventCondition> moveDirToEvtCond{
        {MoveDirection::Right, EventCondition::More_Or_Equal_X},
        {MoveDirection::Left, EventCondition::Less_Or_Equal_X},
        {MoveDirection::Up, EventCondition::Less_Or_Equal_Y},
        {MoveDirection::Down, EventCondition::More_Or_Equal_Y},
    };

    EventCondition condition = moveDirToEvtCond[curDirection];
    double eventValue = 0.0;

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

    addChangeDirectionEvent(GameObjectMoveEvent(newDirection, condition, eventValue, snake[0]->boundaryCrossCount));
}

bool DesktopSnake::handleKey(long virtualKey)
{
    if (!GetAsyncKeyState(virtualKey))
        return false;

    static unordered_map<long, MoveDirection> vkKeyToMoveDir{
        {VK_UP, MoveDirection::Up},
        {VK_DOWN, MoveDirection::Down},
        {VK_LEFT, MoveDirection::Left},
        {VK_RIGHT, MoveDirection::Right},
    };

    if (vkKeyToMoveDir.count(virtualKey) == 0) 
        return false;

    MoveDirection newDirection = vkKeyToMoveDir[virtualKey];
    MoveDirection curDirection = snake[0]->direction;

    // Prevent the snake going back on itself or adding redundant events.
    if (newDirection == curDirection || newDirection == oppositeDirection(curDirection))
        return false;

    snake[0]->setDirection(newDirection);

    // The icon might be static at the start of the game. In that case it will not have a 
    // body so no need to add any events. 
    if (curDirection == MoveDirection::Static)
        return true;

    // Sanity check to ensure the new direction is perpendicular to the current direction.
    MoveDirection perp = perpendicularClockwiseDirection(curDirection);
    if (newDirection != perp && newDirection != oppositeDirection(perp))
        throw runtime_error("Movement direction change should be perpendicular.");

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
    if (dirChangeWouldCauseCollision())
        return;

    // Only handle one key press at a time.
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

bool DesktopSnake::testAABBIconCollision(const Vec2<int>& a, const Vec2<int>& b)
{
    const double iconScale = 0.75;

    int aLeft = a.x; 
    int aRight = a.x + static_cast<int>(iconSpacing.x * iconScale);
    int aTop = a.y;
    int aBottom = a.y + static_cast<int>(iconSpacing.y * iconScale);

    int bLeft = b.x;
    int bRight = b.x + static_cast<int>(iconSpacing.x * iconScale);
    int bTop = b.y; 
    int bBottom = b.y + static_cast<int>(iconSpacing.y * iconScale);

    if (aLeft < bRight && 
        aRight > bLeft &&
        aTop < bBottom && 
        aBottom > bTop)
    {
        return true;
    }

    return false;
}

DesktopSnake::GameObjectVec::iterator DesktopSnake::testObjCollision(
    const GameObject& obj, 
    DesktopSnake::GameObjectVec& objList,
    size_t startIndex)
{
    if (startIndex >= objList.size())
        return objList.end();

    Vec2<int> objPoint = snake[0]->position;

    for (auto listIt = objList.begin() + startIndex; listIt != objList.end(); ++listIt)
    {
        if ( testAABBIconCollision(objPoint, Vec2<int>((*listIt)->position)) )
        {
            return listIt;
        }
    }

    return objList.end();
}

bool DesktopSnake::addingIconWouldGoOOB()
{
    auto& tail = snake.back();

    switch (tail->direction)
    {
    case MoveDirection::Up:
        return (tail->position.y + static_cast<int64_t>(iconSpacing.y) * 2 >= deskRes.y - 1.0);
    case MoveDirection::Down:
        return (tail->position.y - iconSpacing.y <= 1.0);
    case MoveDirection::Left:
        return (tail->position.x + static_cast<int64_t>(iconSpacing.x) * 2 >= deskRes.x - 1.0);
    case MoveDirection::Right:
        return (tail->position.x - iconSpacing.x <= 1.0);
    default:
        return true;
    }
}

#ifdef ADD_FOOD_ICONS
wstring DesktopSnake::randomFileExtension()
{
    static const vector<wstring> extensions{
        L".exe", L".dll", L".doc", L".docx", L".mp3", 
        L".pdf", L".bmp", L".avi", L".jpg", L".msc"};
    return extensions[randomInt(0, static_cast<int>(extensions.size() - 1))];
}

std::wstring DesktopSnake::findUniqueDesktopFilename()
{
    wstring fileName;
    bool exists;

    // Simple counter which will make up the filename.
    static int lastUniqueFileIndex = 0;

    do
    {
        fileName = std::to_wstring(++lastUniqueFileIndex) + randomFileExtension();
        wstring path = desktopDirPath + L'\\' + fileName;

        exists = (PathFileExists(path.c_str()) ? true : false);
        if (exists)
            fmt::print("'{}' exists already\n", wstringToOem(fileName));

    } while (exists);

    return fileName;
}

void DesktopSnake::addDesktopFoodIcon()
{
    wstring foodFileName = findUniqueDesktopFilename();
    wstring foodFilePath = desktopDirPath + L'\\' + foodFileName;

    ofstream file(foodFilePath.c_str());
    if (!file.is_open())
        throw runtime_error("Failed to create file " + wstringToOem(foodFileName));
    file.close();

    // Refresh the desktop while we keep attempting to get a pointer.
    // Some kind of timeout might be appropriate here. 
    unique_ptr<DesktopIcon> icon;
    do
    {
        dc.refresh();
        icon = dc.iconByName(foodFileName);
        std::this_thread::sleep_for(microseconds(2));
    } while (!icon);

    icon->reposition(
        Vec2<int>(
            randomInt(0, deskRes.x - iconSpacing.x),
            randomInt(0, deskRes.y - iconSpacing.y)
        ));

    food.push_back(make_unique<GameObject>(std::move(icon)));
    filesAdded.push_back(foodFileName);

    fmt::print("New food added: '{}'\n", wstringToOem(foodFileName));
}
#endif

void DesktopSnake::handleCollision()
{
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

void DesktopSnake::consumeFood(DesktopSnake::GameObjectVec::iterator foodIt)
{
    unique_ptr<GameObject> foodObj = std::move(*foodIt);
    food.erase(foodIt);

    // Get movement direction and position of the snake's (old) tail before adding a new tail.
    MoveDirection tailDirection = snake.back()->direction;
    Vec2<double> tailPosition = snake.back()->position;

    snake.push_back(std::move(foodObj));

    unique_ptr<GameObject>& tail = snake.back();

    // Since this object was moved, the constructor wasn't called so the direction 
    // and position needs to be set from here. 
    tail->setDirection(tailDirection);
    tail->position = adjacentIconPositionFromDirection(tailPosition, tailDirection);

    // A new object has been added but its event queue is empty so we need to update
    // it with the events from the icon ahead of it. 
    unique_ptr<GameObject>& secondToLastObj = snake.end()[-2];
    tail->changeDirEventsQueue = secondToLastObj->changeDirEventsQueue;
    tail->pendingChangeDirEventsQueue = secondToLastObj->pendingChangeDirEventsQueue;
    tail->boundaryCrossCount = secondToLastObj->boundaryCrossCount;
}

void DesktopSnake::addChangeDirectionEvent(const GameObjectMoveEvent& event)
{
    if (snake.size() < 2)
        return;

    for (size_t i = 1; i < snake.size(); ++i)
    {
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
static long long countSince(const time_point<high_resolution_clock>& start)
{
    return duration_cast<T>(steady_clock::now() - start).count();
}

void DesktopSnake::step()
{
    static auto updateIconStart = steady_clock::now();
    static auto stepStart = steady_clock::now();
    static auto debugStart = steady_clock::now();

    const long long nanosecondsPerSecond = static_cast<long long>(1e9);

    // This is a slow operation so should be done infrequently compared to game logic updates.
    if (countSince<nanoseconds>(updateIconStart) >= nanosecondsPerSecond * iconUpdateInterval)
    {
        updateIconPositions();
        updateIconStart = steady_clock::now();
    }

    // This can be done much more frequently than icon position updates.
    if (countSince<nanoseconds>(stepStart) >= nanosecondsPerSecond * gameStepInterval)
    {
        handleInput();
        handleBoundaries();
        handleEvents();
        handleCollision();

        for (auto& icon : snake)
            icon->step();

        stepStart = steady_clock::now();
    }

    // Debug timer.
    if (countSince<nanoseconds>(debugStart) >= nanosecondsPerSecond)
    {
        //static int i = 0;
        //fmt::print("blah: {}\n", i++);
        debugStart = steady_clock::now();
    }
}

void DesktopSnake::updateIconPositions()
{
    vector<DesktopIcon*> icons;
    vector<Vec2<int>> points;

    for (auto& snakeObj : snake)
    {
        icons.push_back(snakeObj->icon.get());
        points.push_back(Vec2<int>(snakeObj->position));
    }

    dc.repositionIcons(icons, points);
}
