import tuvokview

tv = tuvokview.initialize()
tv.initTuvok()

cam = getDefaultCamera()
cam.setPosition(Vector3(0, -1, 2))
cam.lookAt(Vector3(0, 0, 0), Vector3(0, 1, 0))


