from werkzeug.wrappers import Request, Response
from werkzeug.serving import run_simple

# this use package json-rpc (not jsonrpc!)
from jsonrpc import JSONRPCResponseManager, dispatcher

from boost_driver import FCCBoostDriver 
from buck_driver import FCCBuckDriver, FCCMPPTDriver 
from mighty_driver import MightyWattDriver 

@dispatcher.add_method
def get_version():
    return ("fcc_json_server", 1)

@Request.application
def application(request):
    dispatcher["mightywatt_readstatus"] = mightywatt.read_status
    dispatcher["mightywatt_setpower"] = mightywatt.set_power
    dispatcher["charger_readstatus"] = charger.read_status
    dispatcher["discharger_readstatus"] = discharger.read_status
    dispatcher["mppt_readstatus"] = mppt.read_status

    response = JSONRPCResponseManager.handle(
        request.data, dispatcher)
    return Response(response.json, mimetype='application/json')

if __name__ == '__main__':

    mightywatt = MightyWattDriver()
    charger = FCCBoostDriver()
    discharger = FCCBuckDriver()
    mppt = FCCMPPTDriver()

    mightywatt.open()
    charger.open()
    discharger.open()
    mppt.open()
    
    #run_simple('localhost', 4000, application)
    run_simple('0.0.0.0', 4002, application)
