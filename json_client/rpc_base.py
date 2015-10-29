import requests
import json

import sys
import pdb
import logging

def enable_stdout_logging():
    
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)

    ch = logging.StreamHandler(sys.stdout)
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    root.addHandler(ch)

class JSONRPCClientException(Exception):
    def __init__(self, code, message):
        self.code = code
        self.message = message

    def __str__(self):
        return "RPC Error {0}: {1}".format(self.code, self.message)

class JSONRPCClientBase:
    def __init__(self, url):
        self.url = url
        self.headers = {'content-type': 'application/json'}

    def build_params(self, args, kwargs):
        """Build list of parameters from the given arguements"""
        params = []
        if len(args) > 0:
            params = list(args)
            if len(kwargs) > 0:
                ValueError("Can't use named and unnamed args in the same function")

        if len(kwargs) > 0:
            params = dict(kwargs)

        return params

    def callp(self, method, params, rpc_id=None):
        """RPC call"""

        # build payload
        payload = {
            "method": method,
            "params": params,
            "jsonrpc": "2.0",
            "id": rpc_id,
        }

        logging.debug("URL: {0}".format(self.url))
        logging.debug("Payload: ")
        logging.debug(payload)

        # send response
        response = requests.post(self.url, data=json.dumps(payload),
                                 headers=self.headers,
                                 proxies={"http":""})

        logging.debug("Response: ")
        logging.debug(response.content)

        # convert string to json        
        json_response = response.json()
        logging.debug(response.json())

        # check and raise RPC errors
        possible_error = json_response.get("error") 

        if possible_error is not None:
            raise RPCClientException(possible_error["code"], possible_error["message"])

        # return result
        return json_response["result"]

    def call(self, _method, *args, _rpc_id=None, **kwargs):
        return self.callp(_method,
                          self.build_params(args, kwargs),
                          _rpc_id)

class JSONRPCClientDispatcher:

    def __init__(self, url):
        self._rpc = JSONRPCClientBase(url)

    def __getattribute__(self, name):
        if name.startswith('_'):
            return object.__getattribute__(self, name)
        else:
            def dynamic_function(*args, **kwargs):
                params = self._rpc.build_params(args, kwargs)
                return self._rpc.callp(name, params)

            return dynamic_function

def rpc_test():
    #enable_stdout_logging()
    r0 = JSONRPCClientBase('http://10.65.196.249:4000/jsonrpc')
    print(r0.call('get_version'))
    print(r0.call('forecast_demand', start_datetime = "2015-03-14", N = 10, method = "prescient"))


    r = JSONRPCClientDispatcher('http://10.65.196.249:4000/jsonrpc')
    print(r.get_version())
    print(r.forecast_demand("2015-03-14", 10, "prescient"))
    print(r.forecast_demand(start_datetime = "2015-03-14", N = 10, method = "prescient"))
    try:
        r.forecast_demand(start_datetime = "2015-03-14", N = 10, methodx = "prescient")
    except RPCClientException as e:
        print(e)
