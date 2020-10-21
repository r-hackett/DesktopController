import deskctrl

try:
    dc = deskctrl.DesktopController()

    flags = dc.folderFlags()
    
    print("Snap to grid:", flags.snapToGrid)
    print("Auto arrange:", flags.autoArrange)
        
except Exception as e:
    print(e.args)
