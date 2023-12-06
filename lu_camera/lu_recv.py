import zmq
from typing import Any, Dict, cast
import cv2
import numpy

# Socket to talk to server
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.setsockopt(zmq.SUBSCRIBE, b"")
# Socket to talk to server

socket.connect ("tcp://localhost:5556" )

while True:
     
     md = cast(Dict[str, Any], socket.recv_json())
     #print(md)
     msg = socket.recv()
     img = numpy.frombuffer(msg, dtype=md['dtype'])
     img = img.reshape(tuple(md['shape']))
     cv2.imshow('recv', img)
     #print(img)
     
     key = cv2.waitKey(1)
     
     if key == 27:
          break

cv2.destroyAllWindows()