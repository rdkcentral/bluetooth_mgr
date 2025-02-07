import dbus
import dbus.service
import dbus.mainloop.glib
from gi.repository import GLib
import threading
from random import randint
from time import sleep

ROOTPATH = "/"
OPATH = "/org/bluez"
IFACE = "org.bluez"
BUS_NAME = "org.bluez"
HCI_PATH = "/org/bluez/hci0"
CONFIGURATION_PATH = "/test/bluez/config"

COMMAND_NO_PROPERTIES = "None"
SOURCE_UUID = "0000110a-0000-1000-8000-00805f9b34fb"
SINK_UUID = "0000110b-0000-1000-8000-00805f9b34fb"
SBC_BYTE = 0

bus = None
name = None
objectManager = None
testOutput = None
objManagerResponse = {}
pairedDevices = []
agent = None
#environmentDevices will be devices that are not yet paired but will appear after a certain amount of time once a scan is started
environmentDevices = []
DiscoveredDeviceObjects = {}
sourceEndpoint = None
sinkEndpoint = None
scanCleanup = 0
mockInstructionsList = []
currentMediaTransport = None
stateMutex = threading.Lock()

def DEBUG(*args):
    output = " ".join([str(arg) for arg in args])
    try:
        print("\033[3;34mDEBUG:" + output +"\033[0m")
    except:
        ERR("A call to print failed")
def INFO(*args):
    output = " ".join([str(arg) for arg in args])
    try:
        print("\033[32mINFO:" + output +"\033[0m")
    except:
        ERR("A call to print failed")
def ERR(*args):
    output = " ".join([str(arg) for arg in args])
    try:
        print("\033[1;31mERROR:" + output +"\033[0m")
    except:
        ERR("A call to print failed")

def getFailureInstruction(function):
    global mockInstructionsList
    instructionsMatchingFunctions = [i for i in mockInstructionsList if i[0] == function]
    if len(instructionsMatchingFunctions) > 0: 
        item = instructionsMatchingFunctions[0]
    else:
        return None
    mockInstructionsList.remove(item)

    return item[1]

class propertyHandler:
    @dbus.service.method(dbus_interface="org.freedesktop.DBus.Properties", in_signature="s", out_signature="a{sv}")
    def GetAll(self, str):
        DEBUG("Got properties for", str)
        return self.properties
    @dbus.service.signal(dbus_interface='org.freedesktop.DBus.Properties',
                             signature='sa{sv}as')
    def PropertiesChanged(self,interface_name, properties,invalidated_properties):
        INFO("Properties changed:", properties)
    @dbus.service.method(dbus_interface="org.freedesktop.DBus.Properties", in_signature="ssv", out_signature="")
    def Set(self, iface, property, value):
        DEBUG(iface, property, value)
        self.properties[property] = value
        self.PropertiesChanged(iface, {property: value}, [])
        if property == "Volume":
            testOutput.reportCommand("VolumeChange", str(value * 2))
        
        


class ObjManager(dbus.service.Object):
    def __init__(self):
        global bus
        self.services = []
        dbus.service.Object.__init__(self, bus, ROOTPATH)
    
    @dbus.service.method(dbus_interface='org.freedesktop.DBus.ObjectManager', out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        global objManagerResponse
        response = {}
        testOutput.reportCommand("GetManagedObjects",COMMAND_NO_PROPERTIES)
        DEBUG('GetManagedObjects')
        return objManagerResponse
    @dbus.service.signal(
        "org.freedesktop.DBus.ObjectManager", signature='oa{sa{sv}}')
    def InterfacesAdded(self, object_path, properties):
        DEBUG("Adding interface:", object_path)

    @dbus.service.signal(
        "org.freedesktop.DBus.ObjectManager", signature='oas')
    def InterfacesRemoved(self, object_path, interfaces):
        DEBUG("Removing interface:", object_path, interfaces)

class Agent(dbus.service.Object):
    def __init__(self):
        global bus
        global objManagerResponse
        objManagerResponse[dbus.ObjectPath(OPATH)] = {'org.freedesktop.DBus.Introspectable': {}, 'org.bluez.AgentManager1': {}}
        dbus.service.Object.__init__(self, bus, OPATH)
#######
#AGENT Interface
#######

    @dbus.service.method(dbus_interface="org.bluez.AgentManager1",
                         in_signature="os", out_signature="", sender_keyword="sender")
    def RegisterAgent(self, agent_path, capability, sender=None):
        global agent
        INFO("Agent registered at", agent_path, capability)
        agent = (sender, agent_path)
    @dbus.service.method(dbus_interface="org.bluez.AgentManager1",
                         in_signature="o", out_signature="")
    def UnregisterAgent(manager, agent_path):
        INFO("Agent unregistered")
        agent = None
    @dbus.service.method(dbus_interface="org.bluez.AgentManager1",
                         in_signature="o", out_signature="")
    def RequestDefaultAgent(manager, agent_path):
        INFO("Default path registered")

class Adapter(dbus.service.Object, propertyHandler):
    def __init__(self):
        global bus
        #add to objmanager response
        global objManagerResponse
        self.discFilter=None
        dbus.service.Object.__init__(self, bus, HCI_PATH)
        self.properties = {
        "UUIDs": dbus.Array(
            [
                # Reference:
                # http://git.kernel.org/cgit/bluetooth/bluez.git/tree/lib/uuid.h
                # PNP
                "00001200-0000-1000-8000-00805f9b34fb",
                # Generic Access Profile
                "00001800-0000-1000-8000-00805f9b34fb",
                # Generic Attribute Profile
                "00001801-0000-1000-8000-00805f9b34fb",
                # Audio/Video Remote Control Profile (remote)
                "0000110e-0000-1000-8000-00805f9b34fb",
                # Audio/Video Remote Control Profile (target)
                "0000110c-0000-1000-8000-00805f9b34fb",
            ],
        ),
        "Discoverable": dbus.Boolean(False),
        "Discovering": dbus.Boolean(False),
        "Pairable": dbus.Boolean(True),
        "Powered": dbus.Boolean(True),
        "Address": dbus.String("E4:5F:01:79:76:B7"),
        "AddressType": dbus.String("public"),
        "Alias": dbus.String("raspberrypi"),
        "Modalias": dbus.String("usb:v1D6Bp0245d050A"),
        "Name": dbus.String("raspberrypi"),
        "Class": dbus.UInt32(268),  # Computer, Laptop
        "DiscoverableTimeout": dbus.UInt32(180),
        "PairableTimeout": dbus.UInt32(0),
        "Roles": dbus.Array(["central", "peripheral"]),
    }
        objManagerResponse[dbus.ObjectPath(HCI_PATH)] = {'org.freedesktop.DBus.Introspectable': {}, 'org.bluez.Adapter1':self.properties, "org.bluez.help":{}}
        objManagerResponse[dbus.ObjectPath(HCI_PATH)].update({"org.bluez.Media1": {}})
#######
#ADAPTER Interface
#######
    @dbus.service.method(dbus_interface="org.bluez.Adapter1",
                         in_signature="a{sv}", out_signature="")
    def SetDiscoveryFilter(self, discoveryType):
        DEBUG("New discovery type:", discoveryType)
        if len(discoveryType) == 0:
            DEBUG("Resetting Filter")
            self.discFilter = None
        if "Transport" not in discoveryType:
            DEBUG("Resetting filter")
            self.discFilter = None
        elif discoveryType["Transport"] == "bredr":
            DEBUG("Setting filter to classic")
            self.discFilter = "classic"
        elif discoveryType["Transport"] == "le":
            DEBUG("Setting filter to le")
            self.discFilter = "le"
        else:
            DEBUG("Resetting filter")
            self.discFilter = None

    @dbus.service.method(dbus_interface="org.bluez.Adapter1",
                         in_signature="", out_signature="")
    def StartDiscovery(self):
        testOutput.reportCommand("StartDiscovery",COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("StartDiscovery")
        if failureCode:
            if failureCode == "org.bluez.Error.Failed":
                INFO("Scan start failed")
                raise dbus.exceptions.DBusException("Discovery in progress", name="org.bluez.Error.Failed")
        INFO("Starting discovery")
        sleep(0.8)
        self.Set("org.bluez.Adapter1", "Discovering", dbus.Boolean(True))
        tDisc = threading.Thread(target=self.DiscoveryRunningThread, args=(None,))
        tDisc.start()
    
    @dbus.service.method(dbus_interface="org.bluez.Adapter1",
                         in_signature="", out_signature="")
    def StopDiscovery(self):
        testOutput.reportCommand("StopDiscovery",COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("StopDiscovery")
        if failureCode:
            if failureCode == "org.bluez.Error.Failed":
                INFO("Scan start failed")
                raise dbus.exceptions.DBusException("Device already paired", name="org.bluez.Error.Failed")
        INFO("Stopping discovery")
        sleep(0.8)
        self.Set("org.bluez.Adapter1", "Discovering", dbus.Boolean(False))

    @dbus.service.method(dbus_interface="org.bluez.Adapter1",
                         in_signature="o", out_signature="")
    def RemoveDevice(self, objectPath):
        global objectManager
        testOutput.reportCommand("RemoveDevice",COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("RemoveDevice")
        if failureCode:
            if failureCode == "Failed":
                INFO("forcing unpair fail")
                raise dbus.exceptions.DBusException("Could not remove device", name="org.bluez.Error.Failed")
            if failureCode == "Timeout":
                INFO("forcing unpair timeout")
                return
        for i in range(len(pairedDevices)):
            if pairedDevices[i].path == objectPath:
                objectManager.InterfacesRemoved(objectPath, pairedDevices[i].interfaces)
                with stateMutex:
                    d = pairedDevices.pop(i)
                d.deinit()
                del d
                return

    def DiscoveryRunningThread(self, args):
        global environmentDevices
        global objectManager
        global pairedDevices
        global DiscoveredDeviceObjects
        global scanCleanup
        backgroundDevicesCount = len(environmentDevices)
        sleep(float(randint(1,3))/10.0)
        i = 0
        scanCleanup = 0
        while self.properties["Discovering"] == dbus.Boolean(True) and i < backgroundDevicesCount:
            if self.discFilter:
                if self.discFilter == "le" and not environmentDevices[i]["LE"]:
                    i += 1
                    DEBUG("Skipping classic device")
                    continue
                if self.discFilter == "classic" and environmentDevices[i]["LE"]:
                    i += 1
                    DEBUG("Skipping LE device")
                    continue
            #create device object and notify object manager
            sleep(float(randint(1,5))/5.0)
            with stateMutex:
                if not environmentDevices[i]["AudioOut"]:
                    DiscoveredDeviceObjects.update({environmentDevices[i]["path"]: bluetoothDevice(environmentDevices[i]["path"], environmentDevices[i]["props"])})
                else:
                    DiscoveredDeviceObjects.update({environmentDevices[i]["path"]: mediaDevice(environmentDevices[i]["path"], environmentDevices[i]["props"])})
            objectManager.InterfacesAdded(environmentDevices[i]["path"], {"org.bluez.Device1":environmentDevices[i]["props"]})
            DEBUG("Device added")
            i += 1
        sleep(0.5)
        sleep_counts = 0
        while ((self.properties["Discovering"] == dbus.Boolean(True)) or (sleep_counts < 60)) and scanCleanup == 0:
            sleep(0.5)
            sleep_counts += 1
        #remove devices
        DEBUG("Removing previously discovered devices")
        pairedDevPaths = [d.path for d in pairedDevices]
        for devPath, dev in DiscoveredDeviceObjects.items():
            if devPath not in pairedDevPaths:
                objectManager.InterfacesRemoved(devPath,dev.interfaces)
                dev.deinit()
                del dev
        with stateMutex:
            DiscoveredDeviceObjects = {}
        scanCleanup = 0
        
        

#######
#MEDIA Interface
#######
    @dbus.service.method(dbus_interface="org.bluez.Media1",
                         in_signature="oa{sv}", out_signature="", sender_keyword="sender")
    def RegisterEndpoint(self, obj, cap, sender=None):
        global sourceEndpoint
        global sinkEndpoint
        INFO("Endpoint registered: ", sender, obj)
        if (cap["UUID"] == SOURCE_UUID):
            sourceEndpoint = (sender, obj)
        else:
            sinkEndpoint = (sender, obj)
        
    @dbus.service.method(dbus_interface="org.bluez.Media1",
                         in_signature="o", out_signature="")
    def UnregisterEndpoint(self, obj):
        INFO("Endpoint unregistered")

class bluetoothDevice(dbus.service.Object, propertyHandler):
    def __init__(self, path, props):
        INFO("Initing device", path)
        global bus
        #add to objmanager response
        global objManagerResponse
        self.properties = props
        self.path = path
        self.transport = None
        self.deleting = False
        self.interfaces = ['org.freedesktop.DBus.Introspectable', 'org.bluez.Device1', 'org.bluez.MediaControl1', 'org.freedesktop.DBus.Properties']
        dbus.service.Object.__init__(self, bus, path)
        objManagerResponse[dbus.ObjectPath(path)] = {'org.freedesktop.DBus.Introspectable': {}, 'org.bluez.Device1':props, 'org.bluez.MediaControl1': {'Connected': dbus.Boolean(False)}}
    def deinit(self):
        INFO("Deniting device", self.path)
        global objManagerResponse
        dbus.service.Object.remove_from_connection(self)
        if self.path in objManagerResponse:
            objManagerResponse.pop(self.path)
        else:
            ERR("cannot remove device from managed objects")
    @dbus.service.method(dbus_interface="org.bluez.Device1", in_signature="", out_signature="")
    def Pair(self):
        testOutput.reportCommand("Pair", COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("Pair")
        if failureCode:
            if failureCode == "Failed":
                INFO("Forced pairing failure")
                raise dbus.exceptions.DBusException("Authentication rejected", name="org.bluez.Error.AuthenticationRejected")
            elif failureCode == "Timeout":
                INFO("Forced timeout")
                sleep(5)
                return
        global pairedDevices
        sleep(2)
        self.Set("org.bluez.Device1", "Connected", dbus.Boolean(True))
        sleep(1.5)
        self.Set("org.bluez.Device1", "Paired", dbus.Boolean(True))
        self.Set("org.bluez.Device1", "Bonded", dbus.Boolean(True))
        sleep(1)
        self.Set("org.bluez.Device1", "Connected", dbus.Boolean(False))
        with stateMutex:
            pairedDevices.append(self)
    @dbus.service.method(dbus_interface="org.bluez.Device1", in_signature="", out_signature="")
    def Connect(self):
        testOutput.reportCommand("Connect", COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("Connect")
        if failureCode:
            if failureCode == "Failed":
                INFO("Forced connect failure")
                raise dbus.exceptions.DBusException("Authentication rejected", name="org.bluez.Error.AuthenticationRejected")
            elif failureCode == "Timeout":
                    INFO("Forced timeout")
                    sleep(5)
                    return
        self.Set("org.bluez.Device1", "Connected", dbus.Boolean(True))
        self.Set("org.bluez.Device1", "ServicesResolved", dbus.Boolean(True))
    def Autoconnect(self):
        DEBUG("Autoconnecting...")
        global agent
        agentobj = bus.get_object(agent[0], agent[1], False)
        agentiface = dbus.Interface(agentobj, "org.bluez.Agent1")
        self.Set("org.bluez.Device1", "Connected", dbus.Boolean(True))
        if "00001124-0000-1000-8000-00805f9b34fb" in self.properties["UUIDs"]:
            DEBUG("Requesting HID authorisation for", self.path)
            agentiface.AuthorizeService(dbus.ObjectPath(self.path), dbus.String("00001124-0000-1000-8000-00805f9b34fb"), reply_handler=self.autconnectSuccess, error_handler=self.autoconnectError)
        elif "00001812-0000-1000-8000-00805f9b34fb" in self.properties["UUIDs"]:
            DEBUG("Requesting HID authorisation for", self.path)
            agentiface.AuthorizeService(dbus.ObjectPath(self.path), dbus.String("00001124-0000-1000-8000-00805f9b34fb"), reply_handler=self.autconnectSuccess, error_handler=self.autoconnectError)
        elif "0000110b-0000-1000-8000-00805f9b34fb" in self.properties["UUIDs"]:
            DEBUG("Requesting audio sink authorisation for", self.path)
            agentiface.AuthorizeService(dbus.ObjectPath(self.path), dbus.String("0000110b-0000-1000-8000-00805f9b34fb"), reply_handler=self.autconnectSuccess, error_handler=self.autoconnectError)
        else:
            ERR("Can't find a servive to authorise")
    def autconnectSuccess(self):
        testOutput.reportCommand("AutoconnectAccepted", "None")
        self.Connect()

    def autoconnectError(self, error):
        testOutput.reportCommand("AutoconnectRejected", "None")
        INFO("Autoconnect rejected/failed with error:", error)


    @dbus.service.method(dbus_interface="org.bluez.Device1", in_signature="", out_signature="")
    def Disconnect(self):
        testOutput.reportCommand("Disconnect", COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("Disconnect")
        if failureCode:
            if failureCode == "Failed":
                INFO("Forced disconnect failure")
                raise dbus.exceptions.DBusException("Authentication rejected", name="org.bluez.Error.AuthenticationRejected")
            elif failureCode == "Timeout":
                INFO("Forced timeout")
                sleep(5)
                return
        sleep(2)
        self.Set("org.bluez.Device1", "Connected", dbus.Boolean(False))
        self.Set("org.bluez.Device1", "ServicesResolved", dbus.Boolean(False))

class mediaDevice(bluetoothDevice):
    @dbus.service.method(dbus_interface="org.bluez.Device1", in_signature="", out_signature="")
    def Connect(self):
        testOutput.reportCommand("Connect", COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("Connect")
        if failureCode:
            if failureCode == "Failed":
                INFO("Forced connect failure")
                raise dbus.exceptions.DBusException("Authentication rejected", name="org.bluez.Error.AuthenticationRejected")
            elif failureCode == "Timeout":
                INFO("Forced timeout")
                sleep(5)
                return
        global bus
        global sourceEndpoint
        sleep(2)
        self.Set("org.bluez.Device1", "Connected", dbus.Boolean(True))
        testOutput.reportCommand("Pair", COMMAND_NO_PROPERTIES)
        #get path and select configuration
        sleep(0.5)
        
        self.Set("org.bluez.Device1", "ServicesResolved", dbus.Boolean(True))
        tMedia = threading.Thread(target=self.mediaHandleThread, args=(None,))
        tMedia.start()
    @dbus.service.method(dbus_interface="org.bluez.Device1", in_signature="", out_signature="")
    def Disconnect(self):
        global bus
        global sourceEndpoint
        testOutput.reportCommand("Disconnect", COMMAND_NO_PROPERTIES)
        failureCode = getFailureInstruction("Disconnect")
        if failureCode:
            if failureCode == "Failed":
                INFO("Forced disconnect failure")
                raise dbus.exceptions.DBusException("Authentication rejected", name="org.bluez.Error.AuthenticationRejected")
            elif failureCode == "Timeout":
                INFO("Forced timeout")
                sleep(5)
                return
        sleep(2)
        self.Set("org.bluez.Device1", "Connected", dbus.Boolean(False))
        self.Set("org.bluez.Device1", "ServicesResolved", dbus.Boolean(False))
        #get path and select configuration
        sleep(0.5)
        if self.transport:
            dbus.service.Object.remove_from_connection(self.transport)
            del self.transport
            self.transport = None
        self.transport.Remove()
        del self.transport
        self.transport = None
    def deinit(self):
        INFO("Deniting device", self.path)
        global objManagerResponse
        self.deleting = True
        if self.transport:
            self.transport.Remove()
            if self.transport:
                dbus.service.Object.remove_from_connection(self.transport)
                del self.transport
                self.transport = None
                
        dbus.service.Object.remove_from_connection(self)
        if self.path in objManagerResponse:
            objManagerResponse.pop(self.path)
        else:
            ERR("Cannot remove device from managed objects")

    def mediaHandleThread(self, args):
        global sourceEndpoint
        global bus
        sleep(3)
        self.mediaendpointobject = bus.get_object(sourceEndpoint[0], sourceEndpoint[1], False)
        DEBUG("Calling SelectConfiguration")
        self.mediaendpointobjectiface = dbus.Interface(self.mediaendpointobject, "org.bluez.MediaEndpoint1")
        failureCode = getFailureInstruction("SelectConfiguration")
        if failureCode:
            INFO("Failure code is:", failureCode)
            if failureCode == "Invalid":
                INFO("sending invalid configuration")
                self.mediaendpointobjectiface.SelectConfiguration([dbus.Byte(0), dbus.Byte(0), dbus.Byte(0), dbus.Byte(0)],signature="ay", reply_handler=self.selectConfigurationSuccess, error_handler=self.selectConfigurationFailure)
            else:
                self.mediaendpointobjectiface.SelectConfiguration([dbus.Byte(0x11), dbus.Byte(0xff), dbus.Byte(1), dbus.Byte(51)],signature="ay", reply_handler=self.selectConfigurationSuccess, error_handler=self.selectConfigurationFailure)
        else:
            self.mediaendpointobjectiface.SelectConfiguration([dbus.Byte(0x11), dbus.Byte(0xff), dbus.Byte(1), dbus.Byte(51)],signature="ay", reply_handler=self.selectConfigurationSuccess, error_handler=self.selectConfigurationFailure)
        
    def selectConfigurationFailure(self, error):
        INFO("Failed as expected")
        testOutput.reportCommand("SelectConfiguration", "Fail")
        return

    def selectConfigurationSuccess(self, configuration):
        INFO ("Select configuration succeeded")
        self.configuration = configuration
        testOutput.reportCommand("SelectConfiguration", "None")
        self.mediaendpointobjectiface.SetConfiguration(dbus.ObjectPath(self.path + "/fd0"), dbus.Dictionary({dbus.String("UUID"): dbus.String(SOURCE_UUID), dbus.String("Device"): dbus.ObjectPath(self.path), dbus.String("Configuration"):dbus.Array(configuration)}),signature="oa{sv}", reply_handler=self.setConfigurationSuccess, error_handler=self.setConfigurationFailure)
    
    def setConfigurationSuccess(self):
        testOutput.reportCommand("SetConfiguration", "None")
        INFO("Called SetConfiguration")
        sleep(1)
        if not self.deleting:
            self.transport = mediaTransport(self.path, 900, 0, dbus.Array(self.configuration))
        else:
            ERR("device is being deleted")
        sleep(1)
        if not self.deleting:
            self.transport.SetPending()
        else:
            ERR("device is being deleted")
            try:
                if self.transport:
                    dbus.service.Object.remove_from_connection(self.transport)
                    del self.transport
                    self.tranport = None
                    
            except:
                ERR("Transport already removed")

    def setConfigurationFailure(self, error):
        ERR(error)

class mediaTransport(dbus.service.Object, propertyHandler):
    def __init__(self, devicePath, mtu, delay, config):
        global objectManager
        global objManagerResponse
        global currentMediaTransport
        self.path = devicePath + "/fd0"
        self.mtu = mtu
        self.delay = delay
        self.volume = 60
        self.config = config
        self.state = "idle"
        self.file = None
        currentMediaTransport = self
        self.properties = {
            "Device": dbus.ObjectPath(devicePath),
            "UUID": dbus.String(SOURCE_UUID),
            "Codec": dbus.Byte(SBC_BYTE),
            "Configuration": self.config,
            "State": dbus.String("idle"),
            "Delay": dbus.UInt16(delay),
            "Volume": dbus.UInt16(60)
        }
        dbus.service.Object.__init__(self, bus, self.path)
        objManagerResponse[dbus.ObjectPath(self.path)] = {'org.freedesktop.DBus.Introspectable': {}, 'org.bluez.MediaTransport1':self.properties}
        objectManager.InterfacesAdded(self.path, {"org.bluez.MediaTransport1":self.properties})
    @dbus.service.method(dbus_interface="org.bluez.MediaTransport1", in_signature="", out_signature="hqq")
    def Acquire(self):
        testOutput.reportCommand("Acquire", "None")
        if (self.state != "pending"):
            raise(dbus.exceptions.DBusException(name="org.bluez.Error.Failed"))
        else:
            self.file = open("./audioOutFile.txt", "w")
            self.SetActive()
            return dbus.types.UnixFd(self.file), dbus.UInt16(self.mtu), dbus.UInt16(self.mtu)
    def SetPending(self):
        self.state = "pending"
        self.Set("org.bluez.MediaTransport1", "State", "pending")
    def SetActive(self):
        self.state = "active"
        self.Set("org.bluez.MediaTransport1", "State", "active")
        sleep(1)
        failureCode = getFailureInstruction("AVRCPvolume")
        if failureCode:
            INFO("Failure code is:", failureCode)
            if failureCode == "NoSupport":
                INFO("Not supporting AVRCP Volume")
                return
        self.Set("org.bluez.MediaTransport1", "Volume", dbus.UInt16(55))

    @dbus.service.method(dbus_interface="org.bluez.MediaTransport1", in_signature="", out_signature="hqq")
    def Release(self):
        self.SetPending()
        if self.file:
            self.file.close()
    def Remove(self):
        global objManagerResponse
        global objectManager
        global currentMediaTransport
        if self.path in objManagerResponse:
            objManagerResponse.pop(self.path)
            objectManager.InterfacesRemoved(self.path, ["org.freedesktop.DBus.Introspectable", "org.bluez.MediaTransport1"])
        if self.file:
            self.file.close()
        currentMediaTransport = None

class ConfigureEnvironment(dbus.service.Object):
    def __init__(self):
        global bus
        dbus.service.Object.__init__(self, bus, CONFIGURATION_PATH)
    @dbus.service.method(dbus_interface="org.testConfig",
                         in_signature="aa{sv}", out_signature="")
    def SetEnvironmentDevices(self, devices):
        DEBUG("Setting devices in the environment")
        global environmentDevices
        environmentDevices = []
        for dev in devices:
            path = "/org/bluez/hci0/dev_" + "_".join(str(dev["address"]).split(":"))
            environmentDevices.append({"path": path, 
                             "props": {'Address': dev["address"], 
                             'AddressType': dbus.String('public'), 
                             'Name': dev["name"], 
                             'Alias': dev["name"], 
                             'Class': dev["class"],
                             'Appearance': dev["appearance"],
                             'Icon': dev["icon"],
                             'Paired': dbus.Boolean(False), 
                             'Bonded': dbus.Boolean(False), 
                             'Trusted': dbus.Boolean(False), 
                             'Blocked': dbus.Boolean(False), 
                             'LegacyPairing': dbus.Boolean(False), 
                             'Connected': dbus.Boolean(False), 
                             'UUIDs': dev["UUIDs"], 
                             'Modalias': dev["modalias"], 
                             'Adapter': dbus.ObjectPath('/org/bluez/hci0'), 
                             'ServicesResolved': dbus.Boolean(False)}, "LE": dev["is_le"], "AudioOut":"0000110b-0000-1000-8000-00805f9b34fb" in dev["UUIDs"]})
            
    @dbus.service.method(dbus_interface="org.testConfig",
                        in_signature="aa{sv}", out_signature="")
    def SetPairedDevices(self, devices):
        global pairedDevices
        global objManagerResponse
        DEBUG("Setting paired devices")
        while len(pairedDevices) != 0:
            DEBUG("Removing", pairedDevices[0].path)
            objectManager.InterfacesRemoved(pairedDevices[0].path,pairedDevices[0].interfaces)
            pairedDevices[0].deinit()
            with stateMutex:
                dev = pairedDevices.pop(0)
        with stateMutex:
            pairedDevices = []
        for dev in devices:
            DEBUG("Adding device:", dev["address"])
            path = "/org/bluez/hci0/dev_" + "_".join(str(dev["address"]).split(":"))
            try:
                newDev = {"path": path, 
                        "props": {'Address': dev["address"], 
                                'AddressType': dbus.String('public'), 
                                'Name': dev["name"], 
                                'Alias': dev["name"], 
                                'Class': dev["class"],
                                'Appearance': dev["appearance"],
                                'Icon': dev["icon"],
                                'Paired': dbus.Boolean(True), 
                                'Bonded': dbus.Boolean(True), 
                                'Trusted': dbus.Boolean(False), 
                                'Blocked': dbus.Boolean(False), 
                                'LegacyPairing': dbus.Boolean(False), 
                                'Connected': dbus.Boolean(False), 
                                'UUIDs': dev["UUIDs"], 
                                'Modalias': dev["modalias"], 
                                'Adapter': dbus.ObjectPath('/org/bluez/hci0'), 
                                'ServicesResolved': dbus.Boolean(False)},
                            "LE": dev["is_le"], 
                            "AudioOut": "0000110b-0000-1000-8000-00805f9b34fb" in dev["UUIDs"]
                        }
            except:
                ERR("Could not create device")
            if not newDev["AudioOut"]:
                d = bluetoothDevice(newDev["path"], newDev["props"])
            else:
                d = mediaDevice(newDev["path"], newDev["props"])
            with stateMutex:
                pairedDevices.append(d)
    @dbus.service.signal("org.testConfig", signature='ss')
    def reportCommand(self, command, properties):
        INFO("Reporting command received:", command, properties)

    @dbus.service.method(dbus_interface="org.testConfig",
                        in_signature="", out_signature="")
    def Clear(self):
        INFO("Clearing state")
        global scanCleanup
        global mockInstructionsList
        scanCleanup = 1
        mockInstructionsList = []

    @dbus.service.method(dbus_interface="org.testConfig",
                        in_signature="ss", out_signature="")
    def PrimeFunctionFailure(self, function, errType):
        INFO("Setting function:", function, "to fail with type:", errType)
        global mockInstructionsList
        mockInstructionsList.append((function, errType))
    @dbus.service.method(dbus_interface="org.testConfig",
                        in_signature="ss", out_signature="")
    def triggerExternalEvent(self, event, data):
        global pairedDevices
        if event == "VolumeChange":
            global currentMediaTransport
            INFO("Setting volume to:", data)
            currentMediaTransport.Set("org.bluez.MediaTransport1", "Volume", dbus.UInt16(int(data)))
        if event == "Disconnect":
            INFO("Disconnecting from", data)
            for dev in pairedDevices:
                if dev.properties["Address"] == data:
                    dev.Disconnect()
        if event == "Autoconnect":
            INFO("Triggering autoconnect", data)
            for dev in pairedDevices:
                if dev.properties["Address"] == data:
                    t = threading.Thread(target=dev.Autoconnect)
                    t.start()
                    return

if __name__ == "__main__":
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)     
    bus = dbus.SystemBus()
    name = bus.request_name(BUS_NAME)
    agent = Agent()
    adapter = Adapter()
    objectManager = ObjManager()
    testOutput = ConfigureEnvironment()
    loop = GLib.MainLoop()
    loop.run()