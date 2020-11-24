import deskctrl
        
def print_icon_details(index, icon):
    pos = icon.position()
    name = deskctrl.wstringToOem(icon.displayName())
    print("\t{}) '{}' at position {},{}".format(index, name, pos.x, pos.y))
    
def icon_callback(icon):
    global i
    print_icon_details(i, icon)
    i += 1
    
try:
    dc = deskctrl.DesktopController()

    i = 1
    print("Enumerating desktop icons:")
    dc.enumerateIcons(icon_callback)
        
    i = 1
    print("\nRetrieving all desktop icon objects:")
    icons = dc.allIcons()
    for icon in icons:
        print_icon_details(i, icon)
        i += 1
        
    recycle_bin = dc.iconByName("Recycle Bin")
    if recycle_bin is not None:
        print("Found Recycle Bin on desktop")
        
except Exception as e:
    print(e.args)
