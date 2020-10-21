import deskctrl as dc

try:
    desk = dc.DesktopController()

    flags = desk.folderFlags()
    
    print("Snap to grid:", flags.snapToGrid)
    print("Auto arrange:", flags.autoArrange)
        
except Exception as e:
    print(e.args)
