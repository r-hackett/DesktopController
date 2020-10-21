import deskctrl as dc
        
def print_icon_details(index, icon):
    pos = icon.position()
    name = dc.wstringToOem(icon.displayName())
    print("\t{}) '{}' at position {},{}".format(index, name, pos.x, pos.y))
    
def icon_callback(icon):
    global i
    print_icon_details(i, icon)
    i += 1
    
try:
    desk = dc.DesktopController()

    i = 1
    print("Enumerating desktop icons:")
    desk.enumerateIcons(icon_callback)
        
    i = 1
    print("\nRetrieving all desktop icon objects:")
    icons = desk.allIcons()
    for icon in icons:
        print_icon_details(i, icon)
        i += 1
        
    recycle_bin = desk.iconByName("Recycle Bin")
    if recycle_bin is not None:
        print("Found Recycle Bin on desktop")
        
except Exception as e:
    print(e.args)
