import deskctrl
import random as rand
import sys

def random_desktop_point(res, spacing):
    return deskctrl.IntVec2(
        rand.randint(0, res.x - spacing.x - 1), 
        rand.randint(0, res.y - spacing.y - 1))

try:
    dc = deskctrl.DesktopController()
    
    res = dc.desktopResolution()
    icon_spacing = dc.iconSpacing()

    icons = dc.allIcons()
    if len(icons) == 0:
        print("No icons on the desktop.")
        sys.exit()
        
    random_icon = icons[rand.randint(0, len(icons)-1)]
    
    # Set it to a random position anywhere on the desktop.
    random_icon.reposition(random_desktop_point(res, icon_spacing))
    
    # Generate a list of random points in bounds of the desktop.
    random_points = [random_desktop_point(res, icon_spacing) for i in icons] 
    
    # Use DesktopController.repositionIcons to efficiently set the position of multiple icons.
    dc.repositionIcons(icons, random_points)
    
except Exception as e:
    print(e.args)
