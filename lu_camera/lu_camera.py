
import cv2
import zmq

context = zmq.Context()

#  Socket to talk to server
print("ZMQ PUB Mode: Binding Port 5555")
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:5555")

cam = cv2.VideoCapture(0)
print("OpenCV Initialization Completed")

while True:
	_, frame = cam.read()

	cv2.imshow('source', frame)

	key = cv2.waitKey(1)

	#retval, buf = cv2.imencode('.webp',
    #                           frame,
    #                           [cv2.IMWRITE_WEBP_QUALITY, 100])
	#frameWidth = int(cam.get(cv2.CAP_PROP_FRAME_WIDTH))	# 영상의 넓이(가로) 프레임
	#frameHeight = int(cam.get(cv2.CAP_PROP_FRAME_HEIGHT))	# 영상의 높이(세로) 프레임
	md = dict(dtype=str(frame.dtype), shape=frame.shape,)
	#print(md)

	socket.send_json(md, zmq.SNDMORE)
	socket.send(frame)
	#print(frame)
	
	if key == 27:
		break

cam.release()
key = cv2.waitKey(1)
cv2.destroyAllWindows()