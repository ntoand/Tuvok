import tuvokview

tv = tuvokview.initialize()
#infile = "data/b3_LK.uvf"
infile = "data/foot_tiffs_2048_3.uvf"
stereo = True
tv.initTuvok(infile, stereo, "") #uvffile, stereo?, jsonfile: "" --> not use

cam = getDefaultCamera()
cam.setPosition(Vector3(0, -1, 2))
cam.lookAt(Vector3(0, 0, 0), Vector3(0, 1, 0))
cam.setEyeSeparation(0.02)

cc = cam.getController()
if(cc != None):
        cc.setSpeed(0.5)


# event
def onEvent():
    e = getEvent()

    # On pointer and right click
    if(e.getServiceType() == ServiceType.Pointer and e.isButtonDown(EventFlags.Right) ) or \
      (e.getServiceType() == ServiceType.Wand and e.isButtonDown(EventFlags.Button5) ) :
      	print 'swap eye'
    	tv.swapEye()

setEventFunction(onEvent)

queueCommand(":freefly")

