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

class RPCClientBase:
    def __init__(self, url):
        self.url = url
        self.headers = {'content-type': 'application/json'}

    def call(self, method, *kargs, id=None):
        # build payload
        payload = {
            "method": method,
            "params": list(kargs),
            "jsonrpc": "2.0",
            "id": id,
        }

        logging.debug("URL: {0}".format(self.url))
        logging.debug("Payload: ")
        logging.debug(payload)

        # send response
        response = requests.post(self.url, data=json.dumps(payload), headers=self.headers,
                                 proxies={"http":""})

        logging.debug("Response: ")
        logging.debug(response.content)
        logging.debug(response.json())

        return response.json()["result"]

