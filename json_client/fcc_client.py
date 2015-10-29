from rpc_base import JSONRPCClientBase


fcc = JSONRPCClientDispatcher("http://127.0.0.1:4002/jsonrpc")

print(fcc.mightywatt_readstatus())
print(fcc.charger_readstatus())
print(fcc.discharger_readstatus())
print(fcc.mppt_readstatus())

#fcc.mightywatt_setpower(2) # set load power to 2 W.


