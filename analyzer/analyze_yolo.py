# analyzer_polo.py
import cv2 # type: ignore
import time
import paho.mqtt.client as mqtt

# ... (MQTT setup - broker, on_connect function) ...
#broker = "localhost"  # Replace "localhost" with your MQTT broker address
broker = "192.168.71.132" # Use your PC's IP in Python too for this test

# Set up MQTT client and connect
# Use Callback API version 2 to avoid DeprecationWarning
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2) # <-- Updated
# Define the on_connect function
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to MQTT broker successfully.")
    else:
        print(f"Failed to connect to MQTT broker. Return code: {rc}")

client.on_connect = on_connect
try:
    print(f"Attempting to connect to MQTT broker at {broker}...")
    client.connect(broker, 1883, 60)
    # *** START THE NETWORK LOOP IN A BACKGROUND THREAD ***
    client.loop_start() # <-- Added this line!
    print("MQTT client network loop started.")
except Exception as e:
    print(f"Could not connect to MQTT broker: {e}")
    # If initial connection fails, the background loop_start will keep trying.
    # You might decide to exit here if initial connection is critical,
    # but loop_start makes it resilient to temporary broker unavailability.


# Video capture initialization
video_path = "sample.mp4"  # Replace with the actual path to your video file
cap = cv2.VideoCapture(video_path)
if not cap.isOpened():
    print("Error: Could not open video file.")
    exit(1)

# Define the traffic signal states and initialize the index
signal_states = ["red", "yellow", "green"]
idx = 0

print("Starting video playback and MQTT publishing...")

# Main loop for video processing and MQTT publishing
while cap.isOpened():
    # *** The loop_start() is handling reconnections in the background ***
    # *** You no longer need manual is_connected() checks and reconnect() calls here ***

    ret, frame = cap.read()
    if not ret:
        print("End of video stream or failed to read frame.")
        break

    # Update the traffic signal states based on the current index
    state = signal_states[idx % 3]
    print(f"Sending signal for light1: {state}")

    # Publish the states via MQTT
    # With loop_start(), publish calls are generally safe even if momentarily disconnected.
    # The library will queue them or handle them when the connection is re-established.
    try:
        client.publish("traffic/light1", state)  # Publish to light1 topic
        light2_state = "red" if state == "green" else "green"
        client.publish("traffic/light2", light2_state) # Publish to light2 topic
        # print(f"Published light1: {state}, light2: {light2_state}") # Optional debug
    except Exception as e:
        # This catch might still be useful for other types of publish errors,
        # but network disconnection is handled by loop_start's internal reconnect.
        print(f"Error publishing message: {e}")


    # Show the video frame
    cv2.imshow("Traffic Video", frame)

    # Wait for 5 seconds (5000 milliseconds)
    key = cv2.waitKey(5000) & 0xFF
    if key == ord('q'):
        print("Quitting video playback.")
        break

    idx += 1 # Move to the next state in the cycle

# *** CLEANUP: Stop the background network loop and disconnect ***
print("Releasing video capture and destroying windows...")
cap.release()
cv2.destroyAllWindows()

# Stop the MQTT network loop
client.loop_stop() # <-- Added this line!

# Disconnect MQTT client cleanly
client.disconnect() # <-- Added this line!

print("Script finished.")

# *** Removed the final client.loop_forever() as it's not needed with loop_start() ***
# client.loop_forever()