import spotipy
from spotipy.oauth2 import SpotifyOAuth
import time
import requests
import io
import sounddevice as sd
import librosa
import numpy as np

import serial
import time


hop_length = 512  #standard hop length for librosa
current_color = [0, 0, 255]  #initialize with blue (R=0, G=0, B=255) - used this to make sure program was running..


#Spotify credentials
SPOTIPY_CLIENT_ID = "cec0d9569694467d864d7f8266ab4ae6"
SPOTIPY_CLIENT_SECRET = "03c6e6156189477dab6040226ecc7db0"
SPOTIPY_REDIRECT_URI = "http://localhost:8888/callback"

#Spotify authorization
scope = "user-top-read"
sp = spotipy.Spotify(auth_manager=SpotifyOAuth(client_id=SPOTIPY_CLIENT_ID,
                                               client_secret=SPOTIPY_CLIENT_SECRET,
                                               redirect_uri=SPOTIPY_REDIRECT_URI,
                                               scope=scope))

#Demo songs list
#To get a song ID, just go on spotify web, pick a song, and grab the long texy after the /track/ part in the url
#ex: https://open.spotify.com/track/7FlHNJT4TC120CDvFOHzei
#the id would be 7FlHNJT4TC120CDvFOHzei
demo_songs = [
   
    ("3yyWrz4CsVzkWPuxtZflov", "Take me Home Country Roads", "KSHMR"),
    # ("2SHnUyZq0zwmvRIl4WY77G", "Alive", "Krewella"),
    ("7FlHNJT4TC120CDvFOHzei", "Rewrite the Stars", ""),
    ("3LxG9HkMMFP0MZuiw3O2rF", "Good Things Fall Apart", "Illenium"),
    # ("1Hdh6lJUR1WZCiNPVWxLTA", "On Ten", "Arizona Zervas"),
    ("6jQPolTBq89z9AHrMrmYpS", "Super Human", "Slander"),
    ("0qwPUvS4Eofc19Uw2IDodG", "Harmony - Radio Edit", "Nicky Romero"),

]



#initialize Arduino serial connection
try:
    #change com9 to whatever port your is
    #38400
    arduino = serial.Serial('COM9', 38400)
    time.sleep(2)
except serial.SerialException as e:
    print(f"Error connecting to Arduino: {e}")
    arduino = None

#fetch top 5 most-played songs
def get_top_songs(limit=5):
    results = sp.current_user_top_tracks(limit=limit, time_range='short_term')
    top_songs = [(track['id'], track['name'], track['artists'][0]['name']) for track in results['items']]
    return top_songs

#get detailed audio features for each song
#the tempo and time signature is basically all we are interested about - the lasers use this information for the beat
def get_song_features(song_id):
    features = sp.audio_features([song_id])[0]
    
    if features is None:
        print(f"Audio features unavailable for song ID: {song_id}")
        return None

    return {
        "danceability": features["danceability"],
        "energy": features["energy"],
        "tempo": features["tempo"],
        "beats": features["time_signature"]
    }



'''
Librosa was the main method of music recognition. Basically, it works by allowing us to split the music up
into different bands. We use numpy to generate a 1d array which represents the amplitude of the current song

Spectral flux was the main method of detecing changes in music. It measures the power (energy of the song) and how it changes from frame
to frame. Its divided into 3 parts: the low, mid, and high frequences of the song

I originally had it so the arduino would react to each band seperately, but that was way to much data.
Combining them would into 1 band would be better as it means less data, but gives an overall measure of how the music energy is changing..
I guess if we wanted to be super accurate, then yea, using the seperate bands would be ideal. 

For the beat tracking, we use the ilbrosa beat_track function which is their algorithm to track beats.
It converts the beat frame indices into time segments. In the trigger_laser() function,
we can use this time segment along with a 0xFF bit shift which will represent the low and high parts 
of the duration of the beat (ie: how long the laser will flash for)

The frequency band analysis is very similar to the flux method. 
We divide the spectrum into 3 parts and use math (which is from some very smart audio engineers..) and then we just sum them
together to get an overall energy representation of the given song in that current frame.

'''
def detect_music_events(y, sr):

    #spectral flux with frequency bands
    spec = np.abs(librosa.stft(y))
    flux_low = np.sum(np.diff(spec[:len(spec)//3], axis=1), axis=0)
    flux_mid = np.sum(np.diff(spec[len(spec)//3:2*len(spec)//3], axis=1), axis=0)
    flux_high = np.sum(np.diff(spec[2*len(spec)//3:], axis=1), axis=0)

    #combine all fluxes to send less data
    flux_total = flux_low + flux_mid + flux_high
    flux_total = np.concatenate(([0], flux_total)) 

    #beat tracking
    beat_frames = librosa.beat.beat_track(y=y, sr=sr)
    beat_times = librosa.frames_to_time(beat_frames, sr=sr)

    #frequency band energies for LED control
    band_edges = [0, len(spec)//3, 2*len(spec)//3, len(spec)]
    frequency_bands = [spec[band_edges[i]:band_edges[i+1]] for i in range(3)]
    band_energies = [np.sum(band, axis=0) for band in frequency_bands]
    
    #normalize (add) energies
    band_energies = [librosa.util.normalize(energy) for energy in band_energies]

    return flux_total, beat_times, band_energies



#send trigger laser function
def trigger_laser(duration_ms):
    if arduino is not None:
        try:
            #send command byte followed by 16-bit duration
            duration_high = (duration_ms >> 8) & 0xFF
            duration_low = duration_ms & 0xFF
            arduino.write(b'L' + bytes([duration_high, duration_low]))
        except serial.SerialException as e:
            print(f"Error sending laser command: {e}")


def pump_water():
     if arduino is not None:
        try:
            arduino.write(b'C')
        except serial.SerialException as e:
            print(f"Error sending flash command: {e}")


'''
This function is responsible for controllong the rgb strip. It updates the LED's at high speed to make it go along with the music
melody. Fist, we will calculate the intensity of the energy by grabbing the current frames band energy, it takes the maximum energy. 
We use this for LED brightness. To be honest.. the rgb lights update so fast that you dont even notice the brightness of the led strip changing..

I set the intensity to 0.3 to make it change alot, but not at every singular change in music energy. That made the lights look like they
were spazzing out. 

After that, I needed a random hue transition that would make the colors flash in a random way.
'''
def update_leds(current_frame, band_energies, flux, flux_threshold):
    #calculate intensity from frequency bands
    intensity = max(band_energies[0][current_frame], band_energies[1][current_frame], band_energies[2][current_frame])

    #use spectral flux to detect subtle changes in music
    if flux[current_frame] > flux_threshold or intensity > 0.3:  

        #hue transition with random function to make it give random hue variants
        hue = (np.sin(current_frame * 0.1) + 1) * 180  
        hue += np.random.randint(-60, 60)  


        #tried to make the brightness more noticeable.. 
        saturation = max(0.5, np.random.uniform(0.7, 1.0))  
        value = max(0.6, intensity)  

        #convert HSV to RGB values using formulas available online
        h = hue / 360
        i = int(h * 6)
        f = h * 6 - i
        p = value * (1 - saturation)
        q = value * (1 - f * saturation)
        t = value * (1 - (1 - f) * saturation)

        if i % 6 == 0:
            r, g, b = value, t, p
        elif i % 6 == 1:
            r, g, b = q, value, p
        elif i % 6 == 2:
            r, g, b = p, value, t
        elif i % 6 == 3:
            r, g, b = p, q, value
        elif i % 6 == 4:
            r, g, b = t, p, value
        else:
            r, g, b = value, p, q

        #rgb values are between 0-255 
        r = int(r * 255)
        g = int(g * 255)
        b = int(b * 255)

        #send to arduino
        trigger_rgb(r, g, b)
        
#send RGB values to Arduino
def trigger_rgb(r, g, b):
    global current_color
    if arduino is not None:
        try:
            #clamp RGB values between 0 and 255
            #otherwise, arduino would throw error saying rgb values are not between 0-255
            r = max(0, min(255, r))
            g = max(0, min(255, g))
            b = max(0, min(255, b))

            #update current_color when setting new RGB values incase pump plays so it can restore color after 
            current_color = [r, g, b] 
            
            #send the RGB values as bytes
            arduino.write(b'R' + bytes([r, g, b]))
        except serial.SerialException as e:
            print(f"Error sending RGB command: {e}")

def play_song_preview(song_id, tempo):
    track = sp.track(song_id)
    preview_url = track['preview_url']
    
    if preview_url:
        try:
            #fetch audio preview
            response = requests.get(preview_url)
            response.raise_for_status()
            audio_data = io.BytesIO(response.content)
            
            #load audio data using librosa
            y, sr = librosa.load(audio_data, sr=None)
            
            #detect music events
            flux, beat_times, band_energies = detect_music_events(y, sr)
            
            #set the threshhold for significant changes
            #the major_flux_threshold was what I played around with alot..
            #setting it to 3.5 was sort of a sweet spot for slower songs however if the song was high energy
            #then the pumps would go off almost every time..
            #4 seems to be the most ideal threshold multipler 
            flux_threshold = np.mean(flux) + 2 * np.std(flux)
            major_flux_threshold = np.mean(flux) + 4 * np.std(flux)
            
            #calculate beat interval based on tempo
            beat_interval = 60 / tempo
            laser_on_ratio = 0.25
            
            #start playback with sounddevice
            sd.play(y, sr)
            start_time = time.time()
            
            #initialize indices for tracking beats and flux events
            beat_index = 0
            flux_index = 0
            last_pump_time = 0
            pump_cooldown = 1.5
            
            #while song is playing, send arduino commands
            while sd.get_stream().active:
                current_time = time.time() - start_time
                current_frame = int(current_time * sr / hop_length)  
                
                #beat detection for laser activation
                #need to make sure we only send the trigger_laser command once the previous laser_trigger
                #has been sent. this avoids duplicate laser flashes
                if beat_index < len(beat_times) and current_time >= beat_times[beat_index]:
                    next_beat_time = beat_times[beat_index + 1] if beat_index + 1 < len(beat_times) else beat_times[beat_index] + beat_interval
                    laser_duration = int((next_beat_time - beat_times[beat_index]) * laser_on_ratio * 1000)
                    trigger_laser(laser_duration)
                    beat_index += 1
                
                #update LEDs based on frequency band energies
                if current_frame < len(band_energies[0]):
                    update_leds(current_frame, band_energies, flux, flux_threshold)
                
                #use spectral flux and the major_flux_threshold for pump activation
                #I sorta hardcoded 1.5 seconds in between pump activations because once the pump_water command is sent
                #I have the water flow for 1 second. So I need some time for the water to flow up and down before sending the same command
                if flux_index < len(flux) and current_time >= librosa.frames_to_time(flux_index, sr=sr):
                    if flux[flux_index] > major_flux_threshold:
                        if (current_time - last_pump_time) >= pump_cooldown:
                            pump_water()
                            last_pump_time = current_time
                    flux_index += 1
                
                #super small delay to ensure commands have been sent
                time.sleep(0.01)  
            
            sd.wait() 
        
        except requests.RequestException as e:
            print(f"Failed to download preview: {e}")
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
    
    else:
        print(f"No preview available for '{track['name']}'")

def main():
    try: 
        #uncomment one of the options below to switch between demo songs or your top 5 songs
        # top 5 songs will be based off of your api credentials provided at the top of the file 
        

        # Option 1: use the hardcoded demo songs
        songs = demo_songs
        print("Using demo songs for this session.")
        
        # Option 2: Fetch top 5 most-played songs
        # songs = get_top_songs()
        # print("Fetching top 5 songs...")

        #print out all the song names and artist info
        print("\nSongs:")
        for index, (song_id, song_name, artist_name) in enumerate(songs, start=1):
            print(f"{index}. {song_name} by {artist_name}")

        for song_id, song_name, artist_name in songs:
            print(f"\nNow playing: {song_name} by {artist_name}")
            
            #get song features and check if they are even available through spotify
            features = get_song_features(song_id)
            if features is None:
                print(f"Skipping '{song_name}' due to missing audio features.")
                continue

            print(f"Song features: {features}")

            #play the song preview and send arduino commands
            play_song_preview(song_id, features["tempo"])

            time.sleep(1)  #leave a short delay before next song to make sure all commands have been sent during the current song

    finally: 
        if arduino is not None:
            
            arduino.close()
            print("Arduino connection closed")

if __name__ == "__main__":
    main()