#!/usr/bin/python2

import BaseHTTPServer, SimpleHTTPServer

import ssl

import argparse

import os



parser = argparse.ArgumentParser()

parser.add_argument('-p', '--port', dest='port', type= int,

    help= "Server Port", default= 8087)

args = parser.parse_args()



httpd = BaseHTTPServer.HTTPServer(('', args.port),

        SimpleHTTPServer.SimpleHTTPRequestHandler)



httpd.socket = ssl.wrap_socket (httpd.socket,

        keyfile= "ca_key.pem",

        certfile= "ca_cert.pem", server_side=True)

print "Serving on 0.0.0.0 port ", args.port

httpd.serve_forever()
