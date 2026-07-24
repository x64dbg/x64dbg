import idapro
import ida_kernwin

status = idapro.open_database("/Applications/IDA Professional 9.2.app/Contents/MacOS/idapyswitch", True)
print(f"open_database status: {status}")
ea = ida_kernwin.get_screen_ea()
print(f"get_screen_ea: {hex(ea)}")
