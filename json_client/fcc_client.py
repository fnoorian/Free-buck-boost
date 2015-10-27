from rpc_base import RPCClientBase


fcc = RPCClientBase("http://127.0.0.1:4002/jsonrpc")

print(fcc.call("mightywatt_readstatus"))
print(fcc.call("charger_readstatus"))
print(fcc.call("discharger_readstatus"))
print(fcc.call("mppt_readstatus"))

#fcc.call("mightywatt_setpower", 2) # set load power to 2 W.


