from werkzeug.wrappers import Request, Response
from werkzeug.serving import run_simple

# this use package json-rpc (not jsonrpc!)
from jsonrpc import JSONRPCResponseManager, dispatcher

from drivers.boost_driver import FCCBoostDriver
from drivers.buck_driver import FCCBuckDriver, FCCMPPTDriver
from drivers.mighty_driver import MightyWattDriver


@dispatcher.add_method
def get_version():
    version = ["fcc_json_server", 1]
    return version


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

    mightywatt = MightyWattDriver(u'8533434373835120D1C2')
    charger = FCCBoostDriver(u'75439333635351719221')
    discharger = FCCBuckDriver(u'75439333635351712071')
    mppt = FCCMPPTDriver(u'75439333635351918140')
    
    #run_simple('localhost', 4000, application)
    run_simple('0.0.0.0', 4002, application)
